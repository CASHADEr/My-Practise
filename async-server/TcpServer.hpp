#pragma once
#include <assert.h>
#include <liburing.h>
#include <unistd.h>
#include "EventLoop.hpp"
#include "IPV4.hpp"

class TcpServer {
 private:
  using IpAddress = IPV4;

 private:
  int listener;
  IpAddress sock_addr;
 public:
  TcpServer(IpAddress ipv4)
      : listener(socket(AF_INET, SOCK_STREAM, 0)), sock_addr(ipv4) {
    assert(listener != -1);
  }
  void listen(int backlog, bool enable_reuse_address = true);
  void start() {
    EventLoop loop(256);
    loop.loop(listener);
  }
  ~TcpServer() {
    close(listener);
  }
};