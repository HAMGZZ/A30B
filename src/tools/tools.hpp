#ifndef TOOLS_HPP
#define TOOLS_HPP
// Just a collection of useful tools...
#include <Arduino.h>
#include "Logger/Logger.hpp"
class Tools
{
    public:
        static bool IsNumber(const char *number)
        {
            for (size_t i = 0; number[i] != 0; i++)
                {
                    if(!isdigit(number[i]))
                        return false;
                }
            return true;
        }

        template<typename T> static void PrintBinary(T t, int len)
        {
            int i = len;
            while(i--)
            {
                Serial.printf("%u", (t[0] >>  i ) & 1u );
                if(i % 4 == 0)
                    Serial.printf(" ");
            }
        }

        template<typename T> static void BitFlip(T t, int len)
        {
            for (int i = 0; i < len; i++)
            {
                t[i] = ((t[i] * 0x0802LU & 0x22110LU) | (t[i] * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
            }
        }
};


#endif