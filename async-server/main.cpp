#include <assert.h>
#include <string>
#include "TcpServer.hpp"

int main(int argc, const char *argv[])
{
    using namespace std;
#ifdef CSHR_DEBUG
    const char *ip = "127.0.0.1";
    unsigned short int port = 10086;
    int backlog = 100;
#else 
    assert(argc == 4);
    const char *ip = argv[1];
    unsigned short int port = stoi(argv[2]);
    int backlog = stoi(argv[3]);
#endif
    printf("ip:%s\n", ip);
    printf("port:%d\n", port);
    printf("backlog:%d\n", backlog);
    auto server = TcpServer(IPV4(ip, port));
    server.listen(backlog);
    server.start();
}