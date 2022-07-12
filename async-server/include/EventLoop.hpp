#include <liburing.h>
#include "IPV4.hpp"
#include <string.h>
#include <functional>

using Request = struct request;
class EventLoop {
 private:
  struct io_uring ring;
  bool quit = false;
 public:
  enum { EVENT_TYPE_ACCEPT = 0, EVENT_TYPE_READ = 1, EVENT_TYPE_WRITE = 2, EVENT_TYPE_SPLICE, EVENT_TYPE_SPLICE_SEND };
 public:
  EventLoop(int queue_depth);
  void loop(int server_socket);
};