#include "liburing.h"
#define READ_SZ 8192
struct request {
  int event_type;
  int iovec_count;
  int client_socket;
  struct iovec iov[];
};

int add_accept_request(int server_socket, struct sockaddr_in *client_addr,
                       socklen_t *client_addr_len, struct io_uring *ring);
int add_read_request(int client_socket, struct io_uring *ring);
int add_splice_request(int client_socket, int file_no, off_t off, unsigned file_sz, struct io_uring *ring);
int add_splice_send_request(struct request *req, struct io_uring *ring);
int add_write_request(struct request *req, struct io_uring *ring);