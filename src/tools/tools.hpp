#ifndef TOOLS_HPP
#define TOOLS_HPP
// Just a collection of useful tools...
#include <Arduino.h>
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

        static void HaltCore()
        {
            for(;;);
        }

        static void HaltAll()
        {
            rp2040.idleOtherCore();
            for(;;);
        }

        static void DumpHex(const void* data, size_t size) {
          // This is not mine - taken from https://gist.github.com/ccbrown/9722406
          char ascii[17];
          size_t i, j;
          ascii[16] = '\0';
          for (i = 0; i < size; ++i) {
          	Serial.printf("%02X ", ((unsigned char*)data)[i]);
          	if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
          		ascii[i % 16] = ((unsigned char*)data)[i];
          	} else {
          		ascii[i % 16] = '.';
          	}
          	if ((i+1) % 8 == 0 || i+1 == size) {
          		Serial.printf(" ");
          		if ((i+1) % 16 == 0) {
          			Serial.printf("|  %s \n\r", ascii);
          		} else if (i+1 == size) {
          			ascii[(i+1) % 16] = '\0';
          			if ((i+1) % 16 <= 8) {
          				Serial.printf(" ");
          			}
          			for (j = (i+1) % 16; j < 16; ++j) {
          				Serial.printf("   ");
          			}
          			Serial.printf("|  %s \n", ascii);
          		}
          	}
          }
        }
};


#endif
