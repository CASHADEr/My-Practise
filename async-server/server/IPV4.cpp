#include "IPV4.hpp"
#include <string.h>
#include <string_view>
#include <arpa/inet.h>
#include <stdio.h>

IPV4::IPV4(const char *ip_address, int port)
{
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = inet_addr(ip_address);
}