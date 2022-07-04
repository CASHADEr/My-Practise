#include "Proxy.hpp"

#include <iostream>

namespace cshr {
void Target::execute() {
    std::cout << "Target executes success." << std::endl;
}
Target::~Target() {
    std::cout << "Target destroyed." << std::endl;
}
void Proxy::execute() {
  if (m_IsAdmin)
    m_target->execute();
  else
    std::cout << "User(" << m_user << ") has no priviledge!" << std::endl;
}
Proxy::~Proxy() {
    std::cout << "Proxy destroyed." << std::endl;
}
};