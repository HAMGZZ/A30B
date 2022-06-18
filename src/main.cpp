/*
 * Lewis Hamilton 2022
 * Using the Arduino core for the RP2040 is a cheap (time wise) and fast way
 * to get the product working. In the future I might have time to write it
 * properly using the RP2040 SDK. This would involve me writing things like a 
 * GPS NMEA library and the like. Something which I do not have time to do right
 * now...
 */


/*
MIT License

Copyright (c) 2022 Lewis Hamilton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <Wire.h>

#include "defines.hpp"
#include "Logger/Logger.hpp"
#include "si5351.h"
#include "TinyGPS++.h"
#include "MemoryModel/memory.hpp"
#include "Shell/Shell.h"
#include "Tools/tools.hpp"
#include "ax25/ax25.hpp"

Si5351 rf;
TinyGPSPlus gps;
Memory memory;
Shell shell;
AX25 ax25;

void cmdSet(Shell &shell, int argc, const ShellArguments &argv)
{
    if(argc > 2)
    {
        if (strcmp(argv[1], "zero") == 0)
        {
            if(Tools::IsNumber(argv[2]))
            {
                //Serial.printf("Setting 0 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                //rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        else if (strcmp(argv[1], "one") == 0)
        {
            if(Tools::IsNumber(argv[2]))
            {
                //Serial.printf("Setting 1 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                //rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        else if((strcmp(argv[1], "callsign") == 0))
        {
            memory.SetCallsign(argv[2]);
        }

        else
        {
            Serial.printf("Unknown set command: %s \n\r", argv[1]);
        }
    }
    else
    {
        Serial.printf("Error, no set verb.\n\r");
    }
}


void cmdStatus(Shell &shell, int argc, const ShellArguments &argv)
{
    long currentTime = millis();
    Serial.printf("A30B Version %0.1f -- Lewis Hamilton VK2GZZ June 2022\n\r", VERSION);
    Serial.printf("STATUS>> \n\r");
    Serial.printf("-\tCallsign:\t %s\r\n", memory.callsign);
    Serial.printf("-\tUpTime:\t\t %lus > %lum\r\n", (long)(currentTime/1000), (long)(currentTime/60000));
    Serial.printf("-\tLONG:\t\t %lf\r\n", -33.233);
    Serial.printf("-\tLAT:\t\t %lf\r\n", 151.234);
    Serial.printf("-\tICON #: \t %lu\r\n");
}

void cmdTest(Shell &shell, int argc, const ShellArguments &argv)
{
    if(argc > 2)
    {
        if (strcmp(argv[1], "crc") == 0)
        {
            char input[1024] = {0};
            strcpy(input, argv[2]);
            Serial.printf("INPUT: \t\t %s\r\n", input);
            Serial.printf("ORIGINAL:\t ");
            for (int i = 0; i < strlen(input); i++)
            {
                Tools::PrintBinary(&input[i], 8);
                Serial.printf("| ");
            }
            Serial.printf("\r\nFLIPPED:\t ");
            Tools::BitFlip(input, strlen(input));
            for (int i = 0; i < strlen(input); i++)
            {
                Tools::PrintBinary(&input[i], 8);
                Serial.printf("| ");
            }
            unsigned long begin = micros();
            uint16_t result = ax25.checksum(input, strlen(input));
            unsigned long end = micros();
            unsigned long timespend = end - begin;
            Serial.printf("\r\nRESULT: \t 0x%04x\r\n", result);
            Serial.printf("RESULT: \t ");
            Tools::PrintBinary(&result, 16);
            Serial.printf("\r\nCALC TIME:\t %lu uS\r\n", timespend);
        }
        else
        {
            Serial.printf("Unknown test command: %s \n\r", argv[1]);
        }
    }
    else
    {
        Serial.printf("Error, no test verb.\n\r");
    }
}


ShellCommand(set,   "set [option] [value] \n\r"
                    "\t-> 'set zero 1012000000' sets the zero mark to 10.120,000,00 MHz\n\r"
                    "\t-> 'set one 1012100000' sets the one mark to 10.121,000,00 MHz\n\r"
                    "\t-> 'set callsign *****' sets the callsign - can be up to 7 chars\n\r"
                    "\t-> 'set icon ***' sets the APRS icon to be transmitted", cmdSet);

ShellCommand(status, "status -> Gives overall status of the system", cmdStatus);

ShellCommand(test,  "test [unit] [options] \n\r"
                    "\t-> 'test crc 12345678' returns the CRC-CCITT result from message '12345678'", cmdTest);

// CORE 0 Responsible for Serial prompt.
void setup() 
{
    Serial.begin();
    rp2040.idleOtherCore();
    delay(5000);
    Serial.print("-- A30B START --");
    Serial.print("\n\r"
                "   ___   ____ ___  ___ \n\r"
                "  / _ | |_  // _ \\/ _ )\n\r"
                " / __ |_/_ </ // / _ |\n\r"
                "/_/ |_/____/\\___/____/ \n\n\r");
    memory.Init();
    rp2040.resumeOtherCore();
    delay(500);
    shell.setPrompt("(A30B) $ ");
    shell.begin(Serial, 20);
}

void loop() 
{
    shell.loop();
    //HEARTBEAT
}


// CORE 1 Responsible for GPS and Shifting data.
void setup1()
{
    delay(10); // WAIT FOR CORE 1 to START
    Logger log("IOSETUP", INFO);
    bool rf_conn;
    log.Send(INFO, "SETTING I2C PINS > ", I2C_SDA, I2C_SCL);
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    log.Send(INFO, "CONNECTING TO RF CHIP");
    rf_conn = rf.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    //if(!rf_conn)
    //{
    //   log.Send(FATAL, "COULD NOT CONNECT TO RF CHIP");
    //}
    log.Send(INFO, "CONNECTED TO RF CHIP");
    log.Send(INFO, "SETTING UP RF");
    // rf.set_freq(FREQ_0, SI5351_CLK0);
    // rf.set_freq(FREQ_1, SI5351_CLK1);
    pinMode(TX_EN, OUTPUT);
    pinMode(D_OUT, OUTPUT);
    Serial1.setTX(UART_TX);
    Serial1.setRX(UART_RX);  
    Serial1.begin(GPS_BAUD);
    ax25.begin(memory.callsign, 300, 000);

}

void loop1()
{
    Logger log("IOLOOP", WARNING);
    for(;;)
    {
        while(Serial1.available() > 0)
            gps.encode(Serial1.read());

        
    }
}
