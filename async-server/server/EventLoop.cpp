#include "EventLoop.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include "io_uring_handle.hpp"

/**
 * 封装的malloc
 **/
static void *malloc_throws(size_t size) {
  void *buf = malloc(size);
  if (!buf) {
    fprintf(stderr, "Fatal error: unable to allocate memory.\n");
    exit(1);
  }
  return buf;
}
/**
 * 字符串全部小写
 **/
static void strtolower(char *str) {
  for (; *str; ++str) *str = (char)tolower(*str);
}
/**
 * 获取http一行的内容
 **/
static int get_line(const char *src, char *dest, int dest_sz) {
  int space_cnt = 0;
  for (int i = 0; i < dest_sz; i++) {
    dest[i] = src[i];
    if (src[i] == ' ') ++space_cnt;
    if (src[i] == '\r' && src[i + 1] == '\n') {
      dest[i] = '\0';
      return space_cnt != 2;
    }
  }
  return -1;
}

static int handle_splice_request(int client_socket, int file_no, unsigned file_count,
                                 struct io_uring *ring) {
  return add_splice_request(client_socket, file_no, 0, file_count, ring);
}

static int handle_splice_send_request(struct request *req,
                                      struct io_uring *ring) {
  return add_splice_send_request(req, ring);
}

/**
 * 对请求封装返回内容，异步写入socket
 **/
static void send_static_string_content(const char *str, int client_socket,
                                       struct io_uring *ring) {
  struct request *req =
      (struct request *)malloc_throws(sizeof(*req) + sizeof(struct iovec));
  unsigned long slen = strlen(str);
  req->iovec_count = 1;
  req->client_socket = client_socket;
  req->iov[0].iov_base = malloc_throws(slen);
  req->iov[0].iov_len = slen;
  memcpy(req->iov[0].iov_base, str, slen);
  add_write_request(req, ring);
}

static void handle_unimplemented_method(int client_socket,
                                        struct io_uring *ring) {
  constexpr const char *response_unimplement =
      "HTTP/1.0 400 Bad Request\r\n"
      "Content-type: text/html\r\n"
      "\r\n"
      "<html>"
      "<head>"
      "<title>ZeroHTTPd: Unimplemented</title>"
      "</head>"
      "<body>"
      "<h1>Bad Request (Unimplemented)</h1>"
      "<p>Your client sent a request ZeroHTTPd did not understand and it is "
      "probably not your fault.</p>"
      "</body>"
      "</html>\r\n";
  send_static_string_content(response_unimplement, client_socket, ring);
}

static void handle_http_404(int client_socket, struct io_uring *ring) {
  constexpr const char *response_404 =
      "HTTP/1.0 404 Not Found\r\n"
      "Content-type: text/html\r\n"
      "\r\n"
      "<html>"
      "<head>"
      "<title>ZeroHTTPd: Not Found</title>"
      "</head>"
      "<body>"
      "<h1>Not Found (404)</h1>"
      "<p>Your client is asking for an object that was not found on this "
      "server.</p>"
      "</body>"
      "</html>\r\n";
  send_static_string_content(response_404, client_socket, ring);
}

/**
 * 仅测试
 **/
static void handle_http_100(const char *message, int client_socket,
                            struct io_uring *ring) {
  constexpr const char *response_100 =
      "HTTP/1.0 100 Request Success\r\n"
      "Content-type: text/html\r\n"
      "\r\n"
      "<html>"
      "<head>"
      "<title>%s</title>"
      "</head>"
      "<body>"
      "<h1>Not Found (404)</h1>"
      "<p>Your client is asking for an object that was not found on this "
      "server.</p>"
      "</body>"
      "</html>\r\n";
  char response[1024];
  sprintf(response, response_100, message);
  send_static_string_content(response, client_socket, ring);
}

/**
 * 完成get请求
 **/
void handle_get_method(char *path, int client_socket, struct io_uring *ring) {
  char final_path[1024];
  strcpy(final_path, "./src");
  int len = strlen(path);
  if (!len) handle_http_404(client_socket, ring);
  strcat(final_path, "/");
  if(path[0] == '/') ++path;

  strcat(final_path, path);
#ifdef CSHR_DEBUG
  printf("client wants file: %s\n", path);
  printf("full path: %s\n", final_path);
#endif
  struct stat path_stat;
  if (stat(final_path, &path_stat) == -1) {
    handle_http_404(client_socket, ring);
  } else {
    int fd = open(final_path, O_RDONLY);
    if (fd < 0) {
      handle_http_100(final_path, client_socket, ring);
    } else {
      unsigned count = path_stat.st_size;
      // 阻塞发送200头部
      constexpr const char *response_ok =
      "HTTP/1.0 200 Request OK\r\n"
      "Content-Length: %d\r\n"
      "Content-type: raw\r\n"
      "Content-Disposition: attachment;filename=\"%s\"\r\n"
      "\r\n";
      char buffer[1024];
      sprintf(buffer, response_ok, count, path);
      if(write(client_socket, buffer, strlen(buffer)) < 0) {
        fprintf(stderr, "sync write http head fail.\n");
        close(client_socket);
      }
#ifdef CSHR_SYNC
      // sendfile是同步的
      off_t off = 0;
      sendfile(client_socket, fd, &off, count);
      close(fd);
#else
      handle_splice_request(client_socket, fd, count, ring);
#endif
    }
  }
}

/**
 * 解析http请求
 * 目前只有get的实现
 **/
static void handle_http_method(char *method_buffer, int client_socket,
                               struct io_uring *ring) {
  char *method, *path, *saveptr;
  method = strtok_r(method_buffer, " ", &saveptr);
  strtolower(method);
  path = strtok_r(NULL, " ", &saveptr);
  if (strcmp(method, "get") == 0) {
    int len = strlen(path);
    constexpr int validlen = 7; // "/?path="
    if(len < validlen) {
      handle_http_404(client_socket, ring);
      return;
    }
    path += validlen;
    handle_get_method(path, client_socket, ring);
  } else {
    handle_unimplemented_method(client_socket, ring);
  }
}

static int handle_client_request(struct request *req, struct io_uring *ring) {
  char http_request[1024];
#ifdef CSHR_DEBUG
  printf("The request line is: %s", (const char *)req->iov[0].iov_base);
#endif
  if (get_line((const char *)req->iov[0].iov_base, http_request,
               sizeof(http_request))) {
    fprintf(stderr, "Malformed request\n");
    handle_http_404(req->client_socket, ring);
  } else {
    handle_http_method(http_request, req->client_socket, ring);
  }
  return 0;
}

EventLoop::EventLoop(int queue_depth) {
  io_uring_queue_init(queue_depth, &ring, 0);
}

void EventLoop::loop(int server_socket) {
  struct io_uring_cqe *cqe;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  add_accept_request(server_socket, &client_addr, &client_addr_len, &ring);
  while (!quit) {
    int ret = io_uring_wait_cqe(&ring, &cqe);
    struct request *req = (struct request *)cqe->user_data;
    if (ret < 0) {
      perror("io_uring_wait_cqe");
      exit(1);
    }
    if (cqe->res < 0) {
      perror("Async request failed.");
      exit(1);
    }
    switch (req->event_type) {
      case EVENT_TYPE_ACCEPT:
#ifdef CSHR_DEBUG
        printf("EVENT_TYPE_ACCEPT\n");
        printf("a guest coming~\n");
#endif
        add_accept_request(server_socket, &client_addr, &client_addr_len,
                           &ring);
        add_read_request(cqe->res, &ring);
        free(req);
        break;
      case EVENT_TYPE_READ:
#ifdef CSHR_DEBUG
        printf("EVENT_TYPE_READ\n");
#endif
        if (!cqe->res) {
          fprintf(stderr, "Empty request, close Connection!\n");
          close(req->client_socket);
        } else {
          handle_client_request(req, &ring);
        }
        free(req->iov[0].iov_base);
        free(req);
        break;
      case EVENT_TYPE_SPLICE:
#ifdef CSHR_DEBUG
        printf("EVENT_TYPE_SPLICE\n");
#endif
        if (!cqe->res) {
          fprintf(stderr, "Fail to read the whole file.\n");
          shutdown(req->client_socket, SHUT_WR);
          int *fd = (int *)req->iov[0].iov_base;
          for (int i = 0; i < req->iov[0].iov_len; ++i) close(*(fd++));
        } else {
          req->event_type = EVENT_TYPE_SPLICE_SEND;
          handle_splice_send_request(req, &ring);
        }
        break;
      case EVENT_TYPE_SPLICE_SEND:
#ifdef CSHR_DEBUG
        printf("EVENT_TYPE_SPLICE_SEND\n");
#endif
        close(req->client_socket);
        for (int i = 0, *fd = (int *)req->iov[0].iov_base; i < 3; ++i)
          close(*(fd++));
        for (int i = 0; i < req->iovec_count; i++) free(req->iov[i].iov_base);
        free(req);
        break;
      case EVENT_TYPE_WRITE:
#ifdef CSHR_DEBUG
        printf("EVENT_TYPE_WRITE\n");
#endif
        close(req->client_socket);
        for (int i = 0; i < req->iovec_count; i++) free(req->iov[i].iov_base);
        free(req);
        break;
      default:
        fprintf(stderr, "Invalid Event Type!\n");
        exit(-1);
    }
    io_uring_cqe_seen(&ring, cqe);
  }
}
