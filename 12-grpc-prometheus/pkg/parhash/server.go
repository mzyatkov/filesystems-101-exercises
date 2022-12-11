package parhash

import (
	"context"
	"time"

	"github.com/pkg/errors"
	"github.com/prometheus/client_golang/prometheus"
	"golang.org/x/sync/semaphore"

	parhashpb "fs101ex/pkg/gen/parhashsvc"
	hashpb "fs101ex/pkg/gen/hashsvc"
	"fs101ex/pkg/workgroup"
	"google.golang.org/grpc"


	
	"net"
	"sync"

)

type Config struct {
	ListenAddr   string
	BackendAddrs []string
	Concurrency  int

	Prom prometheus.Registerer
}

// Implement a server that responds to ParallelHash()
// as declared in /proto/parhash.proto.
//
// The implementation of ParallelHash() must not hash the content
// of buffers on its own. Instead, it must send buffers to backends
// to compute hashes. Buffers must be fanned out to backends in the
// round-robin fashion.
//
// For example, suppose that 2 backends are configured and ParallelHash()
// is called to compute hashes of 5 buffers. In this case it may assign
// buffers to backends in this way:
//
//	backend 0: buffers 0, 2, and 4,
//	backend 1: buffers 1 and 3.
//
// Requests to hash individual buffers must be issued concurrently.
// Goroutines that issue them must run within /pkg/workgroup/Wg. The
// concurrency within workgroups must be limited by Server.sem.
//
// WARNING: requests to ParallelHash() may be concurrent, too.
// Make sure that the round-robin fanout works in that case too,
// and evenly distributes the load across backends.
//
// The server must report the following performance counters to Prometheus:
//
//  1. nr_nr_requests: a counter that is incremented every time a call
//     is made to ParallelHash(),
//
//  2. subquery_durations: a histogram that tracks durations of calls
//     to backends.
//     It must have a label `backend`.
//     Each subquery_durations{backed=backend_addr} must be a histogram
//     with 24 exponentially growing buckets ranging from 0.1ms to 10s.
//
// Both performance counters must be placed to Prometheus namespace "parhash".
type Server struct {
	conf Config
	counter prometheus.Counter 
	subquery_durations *prometheus.HistogramVec

	sem *semaphore.Weighted
	stop context.CancelFunc
	l net.Listener
	wg sync.WaitGroup
	mu sync.Mutex
	index_counter int
}

func New(conf Config) *Server {
	return &Server{
		conf: conf,
		sem:  semaphore.NewWeighted(int64(conf.Concurrency)),
		counter: prometheus.NewCounter(prometheus.CounterOpts{
			Namespace: "parhash",
			Name: "nr_requests",
			Help: "Number of requests",
		}),
		subquery_durations: prometheus.NewHistogramVec(prometheus.HistogramOpts{
			Namespace: "parhash",
			Name: "subquery_durations",
			Help: "Subquery durations",
			Buckets: prometheus.ExponentialBuckets(0.1, 10000, 24),
		}, []string{"backend"}),
		index_counter: 0,
	}
}

func (s *Server) Start(ctx context.Context) (err error) {
	defer func() { err = errors.Wrapf(err, "Start()") }()

	ctx, s.stop = context.WithCancel(ctx)

	s.l, err = net.Listen("tcp", s.conf.ListenAddr)
	if err != nil {
		return err
	}

	srv := grpc.NewServer()
	parhashpb.RegisterParallelHashSvcServer(srv, s)

	s.conf.Prom.MustRegister(s.counter)
	s.conf.Prom.MustRegister(s.subquery_durations)

	s.wg.Add(2)
	go func() {
		defer s.wg.Done()

		srv.Serve(s.l)
	}()
	go func() {
		defer s.wg.Done()

		<-ctx.Done()
		s.l.Close()
	}()

	return nil
}

func (s *Server) ListenAddr() string {
	return s.l.Addr().String()
}

func (s *Server) Stop() {
	s.stop()
	s.wg.Wait()
	s.l.Close()
}


func (s *Server) ParallelHash(ctx context.Context, req *parhashpb.ParHashReq) (resp *parhashpb.ParHashResp, err error) {
	defer func() { err = errors.Wrapf(err, "ParallelHash()") }()
	s.counter.Inc()
	clients := make([]hashpb.HashSvcClient, len(s.conf.BackendAddrs))
	connections := make([]*grpc.ClientConn, len(s.conf.BackendAddrs))
	for i, addr := range s.conf.BackendAddrs {	
		connections[i], err = grpc.Dial(addr, grpc.WithInsecure())
		if err != nil {
			return nil, err
		}
		defer connections[i].Close()
		clients[i] = hashpb.NewHashSvcClient(connections[i])
	}
	var wg = workgroup.New(workgroup.Config{Sem : s.sem})
	var hashes = make([][]byte, len(req.Data))
	for i, data := range req.Data {
		i, data := i, data
		wg.Go(ctx, func(ctx context.Context) error {
			var err error
			s.mu.Lock()
			index := s.index_counter
			s.index_counter = (s.index_counter + 1) % len(s.conf.BackendAddrs)
			s.mu.Unlock()
			
			start := time.Now()
			resp, err := clients[index].Hash(ctx, &hashpb.HashReq{Data: data})
			dt := time.Since(start)
			
			if err != nil {
				return err
			}
			s.subquery_durations.WithLabelValues(s.conf.BackendAddrs[index]).Observe(dt.Seconds())
			
			s.mu.Lock()
			hashes[i] = resp.Hash
			s.mu.Unlock()

			return nil
		})
	}
	if err := wg.Wait(); err != nil {
		return nil, err
	}
	return &parhashpb.ParHashResp{Hashes: hashes}, nil

}