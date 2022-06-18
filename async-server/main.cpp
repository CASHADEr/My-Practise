#include <assert.h>
#include <string>
#include "TcpServer.hpp"

int main(int argc, const char *argv[])
{
    using namespace std;
    assert(argc == 4);
    const char *ip = argv[1];
    printf("ip:%s\n", ip);
    unsigned short int port = stoi(argv[2]);
    printf("port:%d\n", port);
    int backlog = stoi(argv[3]);
    printf("backlog:%d\n", backlog);
    auto server = TcpServer(IPV4(ip, port));
    server.listen(backlog);
    server.start();
}