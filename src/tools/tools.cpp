#include "tools.hpp"

bool isNumber(const char *number)
{
    for (size_t i = 0; number[i] != 0; i++)
        {
            if(!isdigit(number[i]))
                return false;
        }
    return true;
}