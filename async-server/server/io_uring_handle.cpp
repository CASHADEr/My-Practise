#include "io_uring_handle.hpp"

#include <unistd.h>

#include "EventLoop.hpp"

int add_accept_request(int server_socket, struct sockaddr_in *client_addr,
                       socklen_t *client_addr_len, struct io_uring *ring) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  io_uring_prep_accept(sqe, server_socket, (struct sockaddr *)client_addr,
                       client_addr_len, 0);
  struct request *req = (struct request *)malloc(sizeof(*req));
  req->event_type = EventLoop::EVENT_TYPE_ACCEPT;
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(ring);
  return 0;
}

int add_read_request(int client_socket, struct io_uring *ring) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  struct request *req =
      (struct request *)malloc(sizeof(*req) + sizeof(struct iovec));
  req->iov[0].iov_base = malloc(READ_SZ);
  req->iov[0].iov_len = READ_SZ;
  req->event_type = EventLoop::EVENT_TYPE_READ;
  req->client_socket = client_socket;
  memset(req->iov[0].iov_base, 0, READ_SZ);
  io_uring_prep_readv(sqe, client_socket, &req->iov[0], 1, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(ring);
  return 0;
}

int add_splice_request(int client_socket, int file_no, off_t off, unsigned file_sz,
                       struct io_uring *ring) {
  int fd[2];
  if (pipe(fd) < 0) {
    fprintf(stderr, "pipe create fails\n.");
    return -1;
  }
#ifdef CSHR_DEBUG
  printf("open pipe %d, %d\n", fd[0], fd[1]);
#endif
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  struct request *req =
      (struct request *)malloc(sizeof(*req) + sizeof(struct iovec[2]));
  req->event_type = EventLoop::EVENT_TYPE_SPLICE;
  req->client_socket = client_socket;
  req->iovec_count = 2;
  req->iov[0].iov_len = 3 * sizeof(int);
  req->iov[0].iov_base = malloc(req->iov[0].iov_len);
  int *fds = (int *)req->iov[0].iov_base;
  *(fds) = fd[1];
  *(fds + 1) = fd[0];
  *(fds + 2) = file_no;
  req->iov[1].iov_len = sizeof(size_t);
  req->iov[1].iov_base = malloc(req->iov[1].iov_len);
  *(unsigned *)req->iov[1].iov_base = file_sz;
#ifdef CSHR_DEBUG
  printf("ori: %d %d %d \n", fd[0], fd[1], file_no);
  printf("fds: ");
  for (int i = 0; i < 3; ++i) printf("%d ", fds[i]);
  printf("\n");
  printf("pipe_in: %d\n", *(int *)(req->iov[0].iov_base));
  printf("file sz is %d\n", file_sz);
#endif
  int pipe_in = *(int *)(req->iov[0].iov_base);
  io_uring_prep_splice(sqe, file_no, off, pipe_in, -1, file_sz, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(ring);
#ifdef CSHR_DEBUG
  printf("splice first submit success\n");
#endif
  return 0;
}

int add_splice_send_request(struct request *req, struct io_uring *ring) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  int *fd = (int *)req->iov[0].iov_base;
  int pipe_out = *(fd + 1);
  int buffer_sz = *(int *)req->iov[1].iov_base;
  io_uring_prep_splice(sqe, pipe_out, -1, req->client_socket, -1, buffer_sz, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(ring);
#ifdef CSHR_DEBUG
  printf("splice second submit success\n");
#endif
  return 0;
}

int add_write_request(struct request *req, struct io_uring *ring) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
  req->event_type = EventLoop::EVENT_TYPE_WRITE;
  io_uring_prep_writev(sqe, req->client_socket, req->iov, req->iovec_count, 0);
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(ring);
  return 0;
}
