#include "Singleton.hpp"
#include <iostream>

namespace cshr
{
 Singleton::Singleton() {
    std::cout << "Singleton Created." << std::endl;
 }  
 SingletonDynamic::SingletonDynamic() {
    std::cout << "SingletonDynamic Created." << std::endl;
 } 
} // namespace cshr
