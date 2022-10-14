#include <solution.h>
#include <assert.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <liburing.h>
// #include <liburing/io_uring.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#define QD  4
#define BS (256*1024)

static int infd, outfd;

struct io_data {
    int read;
    off_t first_offset, offset;
    size_t first_len;
    struct iovec iov;
};

static int setup_context(unsigned int entries, struct io_uring *ring)
{
	int ret;

	ret = io_uring_queue_init(entries, ring, 0);
	if (ret < 0) {
		fprintf(stderr, "queue_init: %s\n", strerror(-ret));
		return -1;
	}

	return 0;
}

static int get_file_size(int fd, off_t *size) {
    struct stat st;
    int ret;
    if ((ret = fstat(fd, &st)) < 0 ) {
        fprintf(stderr, "fstat return %d\n", ret);
        return ret;
    }
    *size = st.st_size;
    return 0;
}

static void queue_prep(struct io_uring *ring, struct io_data *data) {
    struct io_uring_sqe *sqe;

    sqe = io_uring_get_sqe(ring);
    assert(sqe);

    if (data->read)
        io_uring_prep_readv(sqe, infd, &data->iov, 1, data->offset);
    else
        io_uring_prep_writev(sqe, outfd, &data->iov, 1, data->offset);

    io_uring_sqe_set_data(sqe, data);
}

static int queue_read(struct io_uring *ring, off_t size, off_t offset) {
    struct io_uring_sqe *sqe;
    struct io_data *data;

    data = malloc(size + sizeof(*data));
    if (!data)
        return 1;

    sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "!sqe error\n");
        free(data);
        return 1;
    }

    data->read = 1;
    data->offset = data->first_offset = offset;

    data->iov.iov_base = data + 1;
    data->iov.iov_len = size;
    data->first_len = size;

    io_uring_prep_readv(sqe, infd, &data->iov,1, offset);
    io_uring_sqe_set_data(sqe, data);
    return 0;
}

static void queue_write(struct io_uring *ring, struct io_data *data) {
    data->read = 0;
    data->offset = data->first_offset;

    data->iov.iov_base = data + 1;
    data->iov.iov_len = data->first_len;

    queue_prep(ring, data);
    io_uring_submit(ring);
}

int copy_file(struct io_uring *ring, off_t insize) {
    unsigned long reads, writes;
    struct io_uring_cqe *cqe;
    off_t write_left, offset;
    int ret;

    write_left = insize;
    writes = reads = offset = 0;

    while (insize || write_left) {
        unsigned long had_reads; 
        int got_comp;

        /* Queue up as many reads as we can */
        had_reads = reads;
        while (insize) {
            off_t this_size = insize;

            if (reads + writes >= QD)
                break;
            if (this_size > BS)
                this_size = BS;
            else if (!this_size) {
                break;
            }
            // printf("queue_read %ld %ld\n", this_size, offset);
            if (queue_read(ring, this_size, offset)) {
                break;
            }

            insize -= this_size;
            offset += this_size;
            reads++;
        }

        if (had_reads != reads) {
            ret = io_uring_submit(ring);
            if (ret < 0) {
                fprintf(stderr, "submit error had_reads!=reads %d\n", ret);
                return ret;
            }
        }

        /* Queue is full at this point. Let's find at least one completion */
        got_comp = 0;
        while (write_left) {
            struct io_data *data;

            if (!got_comp) {
                // printf("io_uring_wait_cqe\n");
                ret = io_uring_wait_cqe(ring, &cqe);
                fprintf(stderr, "!got_comp %d\n", ret);
                got_comp = 1;
            } else {
                ret = io_uring_peek_cqe(ring, &cqe);
                if (ret == -EAGAIN) {
                    cqe = NULL;
                    ret = 0;
                }
            }
            if (ret < 0) {
                fprintf(stderr, "peek_cqe return %d\n", ret);
                return ret;
            }
            if (!cqe)
                break;
            fprintf(stderr, "io_uring_cqe_get_data\n");
            data = io_uring_cqe_get_data(cqe);
            if (cqe->res < 0) {
                if (cqe->res == -EAGAIN) {
                    queue_prep(ring, data);
                    io_uring_cqe_seen(ring, cqe);
                    continue;
                }
                fprintf(stderr, "cqe_get_data error %d\n", cqe->res);
                return -errno;
            } else if (cqe->res != (int)data->iov.iov_len) {
                /* short read/write; adjust and requeue */
                data->iov.iov_base += cqe->res;
                data->iov.iov_len -= cqe->res;
                queue_prep(ring, data);
                io_uring_cqe_seen(ring, cqe);
                continue;
            }

            //  If write, nothing else to do. If read, queue write.
            if (data->read) {
                queue_write(ring, data);
                write_left -= data->first_len;
                reads--;
                writes++;
            } else {
                free(data);
                writes--;
            }
            io_uring_cqe_seen(ring, cqe);
        }
    }

    /* wait out pending writes */
	while (writes) {
		struct io_data *data;

		ret = io_uring_wait_cqe(ring, &cqe);
		if (ret) {
			fprintf(stderr, "wait_cqe=%d\n", ret);
			return 1;
		}
		if (cqe->res < 0) {
			fprintf(stderr, "write res=%d\n", cqe->res);
			return 1;
		}
		data = io_uring_cqe_get_data(cqe);
		free(data);
		writes--;
		io_uring_cqe_seen(ring, cqe);
	}

    return 0;
}

int copy(int in, int out)
{
    infd = in;
    outfd = out;

    struct io_uring ring;
    /**
     * Создаем инстанс io_uring, не используем никаких кастомных опций.
     * Емкость SQ и CQ буфера указываем как 4 вхождения.
     */
    if (setup_context(QD, &ring))
		return 1;

    off_t insize =0;
    if (get_file_size(infd, &insize)) {
        return 1;
    }
	int ret;
    ret = copy_file(&ring, insize);
    fprintf(stderr,"ret= %d\n", ret);
    io_uring_queue_exit(&ring);

	return ret;
}
