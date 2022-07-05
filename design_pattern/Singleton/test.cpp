#include <iostream>
#include <memory>
#include "Singleton.hpp"
using namespace cshr;


int main() {
  auto &t1 = Singleton::getInstance();
  auto &t2 = SingletonDynamic::getInstance();
}