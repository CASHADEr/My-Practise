#include "TcpServer.hpp"
#include <liburing.h>
#include <functional>

void TcpServer::listen(int backlog, bool enable_reuse_address) {
  int sock = listener;
  int enable = enable_reuse_address;
  int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  assert(res >= 0);
  const struct sockaddr_in *addr_ptr = sock_addr.getIpAddress();
  res = bind(sock, (const struct sockaddr *)addr_ptr, sizeof(struct sockaddr_in));
  if(res < 0) {
    perror("bind fail:");
  }
  res = ::listen(sock, backlog);
  assert(res >= 0);
}
