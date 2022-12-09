package parhash

import (
	"context"

	"github.com/pkg/errors"
	"golang.org/x/sync/semaphore"

	hashpb "fs101ex/pkg/gen/hashsvc"
	parhashpb "fs101ex/pkg/gen/parhashsvc"	
	"google.golang.org/grpc"
	"net"
	"sync"
)


type Config struct {
	ListenAddr   string
	BackendAddrs []string
	Concurrency  int
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
type Server struct {
	conf Config

	sem *semaphore.Weighted

	stop context.CancelFunc
	l    net.Listener
	wg   sync.WaitGroup

}

func New(conf Config) *Server {
	return &Server{
		conf: conf,
		sem:  semaphore.NewWeighted(int64(conf.Concurrency)),
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
	parhashpb.RegisterParallelHashSvcServer(srv , s)

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
}

func (s *Server) ParallelHash(ctx context.Context, req *parhashpb.ParHashReq) (resp *parhashpb.ParHashResp, err error) {

	defer func() { err = errors.Wrapf(err, "ParallelHash()") }()
	// For example, if there are 2 backends and 5 buffers,
	// then the buffers must be assigned to backends in this way:
	//
	//	backend 0: buffers 0, 2, and 4,
	//	backend 1: buffers 1 and 3.	
	// sub-requests must be fanned out evenly


	hashes := make([][]byte, len(req.Data))
	clients := make([]hashpb.HashSvcClient, len(s.conf.BackendAddrs))
	for i, addr := range s.conf.BackendAddrs {
		conn, err := grpc.Dial(addr, grpc.WithInsecure())
		if err != nil {
			return nil, err
		}
		clients[i] = hashpb.NewHashSvcClient(conn)
	}

	var wg sync.WaitGroup

	for i, buf := range req.Data {
		wg.Add(1)
		go func(i int, buf []byte) {
			defer wg.Done()
			s.sem.Acquire(ctx, 1)
			defer s.sem.Release(1)

			// round-robin
			client := clients[i % len(clients)]
			resp, err := client.Hash(ctx, &hashpb.HashReq{Data: buf})
			if err != nil {
				return
			}
			hashes[i] = resp.Hash
		}(i, buf)
	}
	wg.Wait()

	return &parhashpb.ParHashResp{Hashes: hashes}, nil


}