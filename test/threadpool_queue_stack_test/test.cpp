#include <iostream>
#include <chrono>
#include "../../lockfree/Concurrent.hpp"
#include "../../log/cshrlog.hpp"
#include "../../threadpool/ThreadPool.hpp"

using namespace cshr;
using namespace std;
using namespace chrono_literals;
int max_ops = 1e5;
template <typename _Ty>
void func_produce(_Ty* Q) {
  int cnt = 0;
  for (int i = 0; i != max_ops; ++i) {
    Q->Push(i);
    ++cnt;
  }
  cout << std::this_thread::get_id() << " produce: " << cnt << endl;
}
template <typename _Ty>
void func_consume(_Ty* Q) {
  int cnt = 0;
  for (int i = 0; i != max_ops; ++i) {
    std::unique_ptr<int> sp;
    sp = Q->Pop();
    if(sp) ++cnt;
  }
  cout << std::this_thread::get_id() << " consume: " << cnt << endl;
}
LockFreeListStack<int> ST;
LockFreeListQueue<int> Q;
auto consumer_st = []() { func_consume(&ST); };
auto consumer_q = []() { func_consume(&Q); };
auto producer_st = []() { func_produce(&ST); };
auto producer_q = []() { func_produce(&Q); };
int main() {
  auto p = ThreadPool<>::GetInstance();

  for (int i = 0; i < 4; ++i) {
    p->postTask(producer_q);
    p->postTask(consumer_q);
    p->postTask(producer_st);
    p->postTask(consumer_st);
  }

  std::this_thread::sleep_for(3000ms);
}