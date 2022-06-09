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

Si5351 rf;
TinyGPSPlus gps;
Memory memory;
Shell shell;

void cmdSet(Shell &shell, int argc, const ShellArguments &argv)
{
    if(argc > 2)
    {
        if (strcmp(argv[1], "zero") == 0)
        {
            if(isNumber(argv[2]))
            {
                //Serial.printf("Setting 0 frequency to: %lld\n", strtoull(argv[2], nullptr, 0));
                //rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n", argv[2]);
            }
        }

        else if (strcmp(argv[1], "one") == 0)
        {
            if(isNumber(argv[2]))
            {
                //Serial.printf("Setting 1 frequency to: %lld\n", strtoull(argv[2], nullptr, 0));
                //rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n", argv[2]);
            }
        }

        else if((strcmp(argv[1], "callsign") == 0))
        {
            memory.SetCallsign(argv[2]);
        }

        else
        {
            Serial.printf("Unknown set command: %s \n", argv[1]);
        }
    }
}


void cmdStatus(Shell &shell, int argc, const ShellArguments &argv)
{
    currentTime = millis();
    Serial.printf("A30B Version %lf -- Lewis Hamilton VK2GZZ June 2022\n", VERSION);
    Serial.printf("STATUS>> \n");
    Serial.printf("-\tCallsign:\t%s", memory.callsign);
    Serial.printf("-\tUpTime:\t%lus > %lum", currentTime/1000, currentTime/60000);
    Serial.printf("-\tLONG:\t %lf", -33.233);
    Serial.printf("-\tLAT:\t %lf", 151.234);
}


ShellCommand(set,   "set [option] [value] \n"
                    "-> 'set zero 1012000000' sets the zero mark to 10.120,000,00 MHz\n"
                    "-> 'set one 1012100000' sets the one mark to 10.121,000,00 MHz\n"
                    "-> 'set callsign *****' sets the callsign - can be up to 7 chars\n"
                    "-> 'set icon **' sets the APRS icon to be transmitted", cmdSet);

ShellCommand(status, "status -> Gives overall status of the system", cmdStatus);

// CORE 0 Responsible for Serial prompt.
void setup() 
{
    rp2040.idleOtherCore();
    delay(10000);
    Serial.begin();
    Serial.print("-- A30B START --");
    Serial.print("\n"
                "   ___   ____ ___  ___ \n"
                "  / _ | |_  // _ \\/ _ )\n"
                " / __ |_/_ </ // / _  |\n"
                "/_/ |_/____/\\___/____/ \n\n");
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



    //log.Send(INFO, "STARTING UART PINS > " + to_Str UART_TX, UART_RX);
    Serial1.setTX(UART_TX);
    Serial1.setRX(UART_RX);  
    Serial1.begin(GPS_BAUD);

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
