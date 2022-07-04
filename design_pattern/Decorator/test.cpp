#include <iostream>

#include "Decorator.hpp"
using namespace cshr;

class Test : public ITarget {
 public:
  void execute() override { std::cout << "Test Execute." << std::endl; }
  ~Test() { std::cout << "Test destroyed." << std::endl; }
};
int main() {
  Decorator dec(new Test());
  dec.bindPreCall([](const std::shared_ptr<ITarget> &ctx) {
    std::cout << "Pre Call." << std::endl;
  });
  dec.bindPostCall([](const std::shared_ptr<ITarget> &ctx) {
    std::cout << "Post Call." << std::endl;
  });
  ITarget &target = dec;
  target.execute();
}