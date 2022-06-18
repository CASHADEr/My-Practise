#pragma once
#include <netinet/in.h>
class IPV4
{
private:
  using address_type = struct sockaddr_in;
  address_type address;
public:
  socklen_t addr_len = sizeof(address_type);
public:
    IPV4(const char *ip_address, int port);
    const address_type *getIpAddress(){
        return &this->address;
    }
};
