#include <iostream>
#include <string>

#include "PuffinEngine.hpp"

std::string PuffinEngine::isUppercase(char l)
{
if(l & 0b00100000)
    return "This letter is lowercase!";
else
    return "This letter is uppercase!";
}
