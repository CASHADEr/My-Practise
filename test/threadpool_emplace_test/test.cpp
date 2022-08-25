#include "../../threadpool/ThreadPool.hpp"
#include <iostream>
#include <chrono>
int func(int v1, int v2) {
    std::cout << "add" << std::endl;
    return v1 + v2;
}
void test(int v1, int v2) {
    using namespace std::chrono;
    std::this_thread::sleep_for(3s);
    std::cout << "test2" << std::endl;
}

int main() {
    using namespace cshr;
    auto p = ThreadPool<>::GetInstance();
    auto future1 = p->emplace("func", func, 1, 2);
    auto future2 = p->emplace("test", test, 1, 2);
    auto future3 = p->emplace("", func, 33, 55);
    std::cout << "waiting" << std::endl;
    future2.wait(); 
    std::cout << "waiting over" << std::endl;
    auto v = future1.get();
    std::cout << v << std::endl;
    std::cout << future3.get() << std::endl;
}