
#include <iostream>

#include "puffinEngine/src/PuffinEngine.hpp"

int main(int argc, char* argv[])
{
    PuffinEngine engine;

    char l;
    std::cin >> l;
    std::cout << engine.isUppercase(l) << std::endl;
    
    return 0;
}
