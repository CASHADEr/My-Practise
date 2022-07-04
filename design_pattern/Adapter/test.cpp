#include "Adapter.hpp"
#include <iostream> 

using namespace cshr;
class OldType: public IAdaptee {
  public:
    void oldExecute() override {
        std::cout << "class A protocol" << std::endl;
    }
    ~OldType(){
        std::cout << "class OldType destroyed." << std::endl;
    }
};

// Adatper 面向对象测试
void OOPTest() {
    auto newType = std::make_unique<IAdapter>(std::make_unique<OldType>());
    std::unique_ptr<ITarget> target = std::forward<decltype(newType) &&>(newType);
    target->execute();
}

int main() {
    OOPTest();
}