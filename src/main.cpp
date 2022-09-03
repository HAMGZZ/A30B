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
#include <LittleFS.h>
#include <FreeRTOS.h>

#include "Settings/Settings.hpp"
#include "defines.hpp"
#include "si5351.h"
#include "TinyGPS++.h"
#include "Shell/Shell.h"
#include "Tools/tools.hpp"
#include "ax25/ax25.hpp"

Si5351 rf;
TinyGPSPlus gps;
Shell shell;
AX25 ax25;
Settings settings;


bool CORE1LOCK;
bool CORE1LOAD;
bool CORE1READY;

void HeartBeat()
{
    if (micros() % 43 == 0)
        digitalWrite(HB_LED, 1);
    else
        digitalWrite(HB_LED, 0);
}

// CORE 0 Responsible for Serial prompt.
void setup()
{
    CORE1LOCK = true;
    CORE1LOAD = false;
    CORE1READY = false;
    Serial.begin();
    pinMode(HB_LED, OUTPUT);
    digitalWrite(HB_LED, 1);
    delay(2000);
    Serial.print("-- A30B START --");
    Serial.print("\n\r"
                 "   ___   ____ ___  ___ \n\r"
                 "  / _ | |_  // _ \\/ _ )\n\r"
                 " / __ |_/_ </ // / _  |\n\r"
                 "/_/ |_/____/\\___/____/ \n\n\r");

    Serial.printf("MOUNTING FILESYSTEM ...[ ]");
    if (!LittleFS.begin())
    {
        Serial.printf("\b\bFAIL]\r\n");
        Serial.printf("Filesystem is critical to operation - there was an error mounting it!\r\nHALTING!...\r\n");
        Tools::HaltAll();
    }
    Serial.printf("\b\bOK]\r\n");
    settings.Init();

    Serial.printf("Loading CORE 1...");
    CORE1LOAD = true;
    
    if (settings.ShellColour == 1)
        shell.setPrompt("\033[1;36m[\033[1;32mA30B\033[1;36m] \033[1;35m$\033[m ");
    else
        shell.setPrompt("[A30B] $ ");
    
    while(!CORE1READY){}

    CORE1LOCK = false;
    shell.begin(Serial);
}

void loop()
{
    shell.loop();
    delay(10); // Release for USB handle
    HeartBeat();
}

void setup1()
{
    delay(10);
    while(!CORE1LOAD){}

    pinMode(TX_EN, OUTPUT);
    pinMode(D_OUT, OUTPUT);

    Serial1.setTX(UART_TX);
    Serial1.setRX(UART_RX);
    Serial1.begin(GPS_BAUD);

    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);

    Serial.printf("CONNECTING TO Si5356 ...[ ]");

    bool rf_conn;
    rf_conn = rf.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if (!rf_conn)
    {
        Serial.printf("\b\bERROR]\r\nCOULD NOT CONNECT TO RF CHIP!\r\n HALTING!");
        for (;;);
    }
    Serial.printf("\b\bOK]\r\n");

    Serial.printf("INITIALISING Si5356 ...[ ]");
    rf.set_freq(settings.ZeroFreq, SI5351_CLK0);
    rf.set_freq(settings.OneFreq, SI5351_CLK1);
    Serial.printf("\b\bOK]\r\n");
    ax25.begin(settings.Callsign, settings.Icon, settings.BaudRate, TX_EN, D_OUT);
    CORE1READY = true;
}

void loop1()
{
    // CORE 1 LOOP LOCK 
    while (!CORE1LOCK)
    {
        while (Serial1.available() > 0)
            gps.encode(Serial1.read());
        delay(50);
    }
}

/*****************************************************************************
 * CLI COMMANDS BELLOW
 *****************************************************************************/

void cmdSet(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 2)
    {
        CORE1LOCK = true;
        if (strcmp(argv[1], "zero") == 0)
        {
            if (Tools::IsNumber(argv[2]))
            {
                Serial.printf("Setting 0 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                settings.ZeroFreq = strtoull(argv[2], nullptr, 0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        else if (strcmp(argv[1], "one") == 0)
        {
            if (Tools::IsNumber(argv[2]))
            {
                Serial.printf("Setting 1 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                settings.OneFreq = strtoull(argv[2], nullptr, 0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        else if ((strcmp(argv[1], "callsign") == 0))
        {
            strcpy(settings.Callsign, argv[2]);
        }

        else if ((strcmp(argv[1], "icon") == 0))
        {
            strcpy(settings.Icon, argv[2]);
        }

        else if ((strcmp(argv[1], "colour") == 0))
        {
            if ((strcmp(argv[2], "true") == 0))
                settings.ShellColour = 1;
            else if ((strcmp(argv[2], "false") == 0))
                settings.ShellColour = 0;
            else
                Serial.printf("Only use true or false.\r\n");
        }

        else
        {
            Serial.printf("Unknown set command: %s \n\r", argv[1]);
        }
        CORE1LOCK = false;
    }
    else
    {
        Serial.printf("Error, no set verb.\n\r");
    }
}

void cmdStatus(Shell &shell, int argc, const ShellArguments &argv)
{
    CORE1LOCK = true;
    long currentTime = millis();
    Serial.printf("A30B Version %0.1f -- Lewis Hamilton VK2GZZ June 2022\n\r", VERSION);
    Serial.printf("STATUS>> \n\r");
    Serial.printf("-\tCallsign:\t %s\r\n", settings.Callsign);
    Serial.printf("-\tUpTime:\t\t %lus > %lum\r\n", (long)(currentTime / 1000), (long)(currentTime / 60000));
    Serial.printf("-\tLONG:\t\t %lf\r\n", -33.233);
    Serial.printf("-\tLAT:\t\t %lf\r\n", 151.234);
    Serial.printf("-\tICON #: \t %s\r\n", settings.Icon);
    Serial.printf("-\tColour: \t %d\r\n", settings.ShellColour);
    CORE1LOCK = false;
}

void cmdTest(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 2)
    {
        Serial.printf("STOPPING CORE1\r\n");
        CORE1LOCK = true;

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

        else if (strcmp(argv[1], "modulation") == 0)
        {
            unsigned long int del = 2500;
            Serial.printf("Modulator tester");
            int count = (int)strtol(argv[2], nullptr, 0);
            Serial.printf("running for %d s");
            count = (count * 1000) / del;
            Serial.printf("testing modulation -> ");
            digitalWrite(TX_EN, 1);
            for (int i = 0; i < count; i++)
            {
                digitalWrite(D_OUT, i % 2);
                Serial.printf("%d", i % 2);
                delay(del);
            }
            digitalWrite(TX_EN, 0);
            digitalWrite(D_OUT, 0);
            Serial.printf("Done!\r\n");
        }

        else if (strcmp(argv[1], "builder") == 0)
        {
            bool run = true;

            Serial.printf("Input: %s\r\n", argv[2]);
            unsigned long begin = micros();
            // Run it once without printing debug to get timing
            ax25.buildPacket(argv[2], false);
            unsigned long end = micros();
            unsigned long timespend = end - begin;
            ax25.buildPacket(argv[2], true);
            Serial.printf("Built packet  :\r\n");
            Tools::DumpHex(ax25.packet, strlen(ax25.packet));
            Serial.printf("\r\nTime spent: %lu uS\r\n", timespend);
            Serial.printf("Do you want to transmit (y/n)? ");
            while (run)
            {
                while (Serial.available())
                {
                    char input = Serial.read();
                    if (input == 'y')
                    {
                        Serial.printf("%c\r\nShifting out!\r\n", input);
                        ax25.shiftOut();
                        run = false;
                    }
                    else
                    {
                        Serial.printf("%c\r\n", input);
                        run = false;
                    }
                }
                delay(10);
            }
        }

        else if(strcmp(argv[1], "baud") == 0)
        {
            float symbolTime = (1/(float)settings.BaudRate) * 1000;
            bool run = true;       
            unsigned long long db = 100000/settings.BaudRate; 
            int out = 0;   
            Serial.printf("Current baud rate: %lu\r\n", settings.BaudRate);
            Serial.printf("The symbol time should be: %f mS\r\n", symbolTime);
            Serial.printf("Press 'q' to quit.\r\n");
            digitalWrite(TX_EN, 1);
            while (run)
            {
                if (Serial.available())
                {
                    if (Serial.read() == 'q')
                        run = false;
                }
                digitalWrite(D_OUT, out);
                out = !out;
                delayMicroseconds(db - 2);
            }
            digitalWrite(TX_EN, 0);
        }

        else
        {
            Serial.printf("Unknown test command: %s \n\r", argv[1]);
        }
        CORE1LOCK = false;
    }
    else
    {
        Serial.printf("Error, no test verb.\n\r");
    }
}

void cmdSave(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Saving configuration...\r\n");
    settings.Write();
    Serial.printf("Configuration saved.\r\n");
}

void cmdLoad(Shell &shell, int argc, const ShellArguments &argv)
{
    CORE1LOCK = true;
    Serial.printf("Loading configuration...\r\n");
    settings.Read();
    Serial.printf("Configuration loaded.\r\n");
    CORE1LOCK = false;
}

void cmdRead(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 1)
    {
        fs:File file = LittleFS.open(argv[1], "r");
        if(!file)
        {
            Serial.printf("File %s not found!\r\n");
            return;
        }

        char in[MAXCHAR] = {0};
        file.readBytesUntil('\r', in, MAXCHAR);
        Serial.printf("File: %s\r\n-- SOF --\r\n%s\r\n-- EOF --\r\n", argv[1], in);
        Serial.printf("Hex Dump: \r\n");
        Tools::DumpHex(in, strlen(in));
        Serial.printf("\r\n");
        file.close();
    }
    else
        Serial.printf("Missing file name\r\n");
}

void cmdLs(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Listing files\r\n");
    Dir dir = LittleFS.openDir("/");
    while (dir.next())
    {
        Serial.printf(" - \t");
        Serial.print(dir.fileName());
        if (dir.fileSize())
        {
            File f = dir.openFile("r");
            Serial.printf("\t%d Bytes\r\n", f.size());
        }
    }
}

void cmdMkdir(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 1)
    {
        if(!LittleFS.mkdir(argv[1]))
        {
            Serial.printf("Path %s is invalid\r\n");
        }
    }
}

void cmdRmdir(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 1)
    {
        if(!LittleFS.rmdir(argv[1]))
        {
            Serial.printf("Path %s is invalid\r\n");
        }
    }
}

void cmdPut(Shell &shell, int argc, const ShellArguments &argv)
{
}

void cmdFormat(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Formatting the file system...");
    Serial.printf(LittleFS.format() ? "OK\r\n" : "ERROR\r\n");
}

void cmdCalibrate(Shell &shell, int argc, const ShellArguments &argv)
{
}


ShellCommand(status, "status -> Gives overall status of the system", cmdStatus);

ShellCommand(set, "set [option] [value] \n\r"
                  "\t-> '** set zero 1012000000' sets the zero mark to 10.120,000,00 MHz\n\r"
                  "\t                         Please restart the device to apply\n\r"
                  "\t-> '** set one 1012100000' sets the one mark to 10.121,000,00 MHz\n\r"
                  "\t                         Please restart the device to apply\n\r"
                  "\t-> 'set callsign *****' sets the callsign - can be up to 7 chars\n\r"
                  "\t-> 'set icon ***' sets the APRS icon to be transmitted\r\n"
                  "\t-> 'set colour true/false' sets the shell colour mode",
             cmdSet);

ShellCommand(test, "test [unit] [options] \n\r"
                   "\t-> 'test crc 12345678' returns the CRC-CCITT result from message '12345678'\r\n"
                   "\t-> 'test builder testpacket' returns the build AX25 packet & allows test tx\r\n"
                   "\t-> 'test modulation 10' modulates signal with 1's and 0's being txd for '10' seconds\r\n"
                   "\t=> 'test baud' test baud rate.",
             cmdTest);

ShellCommand(save, "save -> Saves the system configuration", cmdSave);

ShellCommand(load, "load -> Lods the system configuration", cmdLoad);

ShellCommand(read, "read [filename] -> read /settings", cmdRead);

ShellCommand(ls, "ls -> List files", cmdLs);

ShellCommand(mkdir, "mkdir [dirname] -> mkdir /newdir", cmdMkdir);

ShellCommand(rmdir, "rmdir [dirname] -> rmdir /deldir", cmdRmdir);

ShellCommand(put, "put [filename] [data] -> put myFile \"This is what is being placed into file\"", cmdPut);

ShellCommand(format, "format -> Formats the file system", cmdFormat);
