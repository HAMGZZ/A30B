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

#include "defines.hpp"
#include "si5351.h"
#include "TinyGPS++.h"
//#include "MemoryModel/FileSystem.hpp"
#include "Shell/Shell.h"
#include "Tools/tools.hpp"
#include "ax25/ax25.hpp"


Si5351 rf;
TinyGPSPlus gps;
Shell shell;
AX25 ax25;





char Callsign[CALLSIGN_SIZE];
char Icon[ICON_SIZE];
unsigned long long CentreFrequency;
unsigned long long FrequencyShift;
unsigned long BaudRate;
unsigned long TransmissionDelay;
int ShellColour;


void HeartBeat()
{
  if(micros() % 43 == 0)
  {
    digitalWrite(HB_LED, 1);
  }
  else
  {
    digitalWrite(HB_LED, 0);
  }

}

int FsRead(bool printResults = false)
{
    fs:File file = LittleFS.open(SETTINGS_PATH, "r");
    if(!file)
      return 1;
    char in[MAXCHAR] = {0};
    int count = SETTINGS_STORED;
    file.readBytesUntil('\n', in, MAXCHAR);
    sscanf(in, "%s", Callsign);
    memset(in, 0, MAXCHAR);
    file.readBytesUntil('\n', in, MAXCHAR);
    sscanf(in, "%s", Icon);
    memset(in, 0, MAXCHAR);
    file.readBytesUntil('\n', in, MAXCHAR);
    sscanf(in, "%llu,%llu,%lu,%lu,%d", &CentreFrequency, &FrequencyShift, &BaudRate, &TransmissionDelay, &ShellColour);
    memset(in, 0, MAXCHAR);
    file.close();
    if(printResults)
    {
      file = LittleFS.open(SETTINGS_PATH, "r");
      file.readBytesUntil('\r', in, MAXCHAR);
      Serial.printf("File: %s\r\nFile contents:\r\n-- SOF --\r\n%s\r\n-- EOF --\r\n", SETTINGS_PATH, in);
      Serial.printf("Hex Dump: \r\n");
      Tools::DumpHex(in, strlen(in));
      Serial.printf("\r\n");
      file.close();
    }
    return 0;
}

void FsSave()
{
    fs:File file = LittleFS.open(SETTINGS_PATH, "w");
    file.printf("%s\n%s\n%llu,%llu,%lu,%lu,%d\n\r", Callsign, Icon, CentreFrequency, FrequencyShift, BaudRate, TransmissionDelay, ShellColour);
    file.close();
}

void FsInit()
{
    Serial.printf("MOUNTING FILESYSTEM ...[ ]");
    if(!LittleFS.begin())
    {
        Serial.printf("\b\bFAIL]\r\n");
        Serial.printf("Filesystem is critical to operation - there was an error mounting it!\r\nHALTING!...\r\n");
        Tools::HaltAll();
    }
    Serial.printf("\b\bOK]\r\n");
    Serial.printf("READING SETTINGS FILE ...[ ]");
    if(FsRead())
    {
        Serial.printf("\b\bWARNING]\r\n");
        Serial.printf("Error reading file... Using defaults!\r\n");
        strcpy(Callsign, DEFAULT_CALL);
        strcpy(Icon, DEFAULT_ICON);
        CentreFrequency = DEFAULT_CTR_FRQ;
        FrequencyShift = DEFAULT_SHIFT;
        BaudRate = DEFAULT_BAUD;
        TransmissionDelay = DEFAULT_TR;
        ShellColour = 1;
        Serial.printf("=== PLEASE RUN 'save' TO SAVE SETTINGS ===\r\n");
    }
    else
        Serial.printf("\b\bOK]\r\n");    
}



void cmdSet(Shell &shell, int argc, const ShellArguments &argv)
{
    if(argc > 2)
    {
        if (strcmp(argv[1], "zero") == 0)
        {
            if(Tools::IsNumber(argv[2]))
            {
                Serial.printf("Setting 0 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
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
                Serial.printf("Setting 1 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                rf.set_freq(strtoull(argv[2], nullptr, 0), SI5351_CLK0);
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        else if((strcmp(argv[1], "callsign") == 0))
        {
            strcpy(Callsign, argv[2]);
        }

        else if((strcmp(argv[1], "icon") == 0))
        {
            strcpy(Icon, argv[2]);
        }
        
        else if((strcmp(argv[1], "colour") == 0))
        {
          if((strcmp(argv[2], "true") == 0))
            ShellColour = 1;
          else if((strcmp(argv[2], "false") == 0))
            ShellColour = 0;
          else
            Serial.printf("Only use true or false.\r\n");
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
    Serial.printf("-\tCallsign:\t %s\r\n", Callsign);
    Serial.printf("-\tUpTime:\t\t %lus > %lum\r\n", (long)(currentTime/1000), (long)(currentTime/60000));
    Serial.printf("-\tLONG:\t\t %lf\r\n", -33.233);
    Serial.printf("-\tLAT:\t\t %lf\r\n", 151.234);
    Serial.printf("-\tICON #: \t %s\r\n", Icon);
    Serial.printf("-\tColour: \t %d\r\n", ShellColour);
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

        else if (strcmp(argv[1], "modulation") == 0)
        {
          unsigned long int del = 2500;
          Serial.printf("Modulator tester");
          int count = (int) strtol(argv[2], nullptr, 0);
          Serial.printf("running for %d s");
          count = (count * 1000) / del;
          Serial.printf("testing modulation -> ");
          digitalWrite(TX_EN, 1);
          for(int i = 0; i < count; i++)
          {
            digitalWrite(D_OUT, i%2);
            Serial.printf("%d", i%2);
            delay(del);
          }
          digitalWrite(TX_EN, 0);
          digitalWrite(D_OUT, 0);
          Serial.printf("Done!\r\n");
        }

        else if (strcmp(argv[1], "builder") == 0)
        {
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
          bool run = true;
          while(run)
          {
            while(Serial.available())
            {
              char input = Serial.read();
              if(input == 'y')
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


void cmdSave(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Saving configuration...\r\n");
    FsSave();
    Serial.printf("Configuration saved.\r\n");
}

void cmdRead(Shell &shell, int argc, const ShellArguments &argv)
{
  Serial.printf("Settings file available to read? : %s\r\n", FsRead() ? "false" : "true");
  FsRead(true);
}

void cmdLs(Shell &shell, int argc, const ShellArguments &argv)
{
  Serial.printf("Listing files\r\n");
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) 
  {
    Serial.printf(" - \t");
    Serial.print(dir.fileName());
    if(dir.fileSize()) 
    {
      File f = dir.openFile("r");
      Serial.printf("\t%d Bytes\r\n", f.size());
    }
  }
}

void cmdFormat(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Formatting the file system...");
    Serial.printf(LittleFS.format()? "OK\r\n":"ERROR\r\n");
}

void cmdCore(Shell &shell, int argc, const ShellArguments &argv)
{
    if(argc > 2)
    {
        if (strcmp(argv[1], "stop") == 0)
        {
            rp2040.idleOtherCore();
        }
        else if(strcmp(argv[1], "start") == 0)
        {
            rp2040.resumeOtherCore();
        }
        else
        {
            Serial.printf("Unknown core command: %s \n\r", argv[1]);
        }
    }
    else
    {
        Serial.printf("Error, no core verb.\n\r");
    }
}


void cmdReset(Shell &shell, int argc, const ShellArguments &argv)
{
    AIRCR_Register = 0x5FA0004;
}


ShellCommand(set,   "set [option] [value] \n\r"
                    "\t-> 'set zero 1012000000' sets the zero mark to 10.120,000,00 MHz\n\r"
                    "\t-> 'set one 1012100000' sets the one mark to 10.121,000,00 MHz\n\r"
                    "\t-> 'set callsign *****' sets the callsign - can be up to 7 chars\n\r"
                    "\t-> 'set icon ***' sets the APRS icon to be transmitted\r\n"
                    "\t-> 'set colour true/false' sets the shell colour mode", cmdSet);

ShellCommand(status, "status -> Gives overall status of the system", cmdStatus);

ShellCommand(test,  "test [unit] [options] \n\r"
                    "\t-> 'test crc 12345678' returns the CRC-CCITT result from message '12345678'\r\n"
                    "\t-> 'test builder testpacket' returns the build AX25 packet & allows test tx\r\n"
                    "\t-> 'test modulation 10' modulates signal with 1's and 0's being txd for '10' seconds", cmdTest);

ShellCommand(save, "save -> Saves the system configuration", cmdSave);

ShellCommand(read, "read settings and return result", cmdRead);

ShellCommand(ls, "List files in FS", cmdLs);

ShellCommand(format, "format -> Formats the file system", cmdFormat);

ShellCommand(core,  "core [option]\n\r"
                    "\t -> 'core stop' Stops the other core"
                    "\t -> 'core start' Starts the other core", cmdCore);

ShellCommand(reset, "reset -> resets the MCU", cmdReset);

// CORE 0 Responsible for Serial prompt.
void setup() 
{
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
    FsInit();
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
    if(!rf_conn)
    {
       Serial.printf("\b\bERROR]\r\nCOULD NOT CONNECT TO RF CHIP!\r\n HALTING!");
       for(;;);
    }
    Serial.printf("\b\bOK]\r\n");
    Serial.printf("INITIALISING Si5356 ...[ ]");
    rf.set_freq(CentreFrequency + (FrequencyShift / 2), SI5351_CLK0);
    rf.set_freq(CentreFrequency - (FrequencyShift / 2), SI5351_CLK1);
    Serial.printf("\b\bOK]\r\n");

    delay(2000);

    if(ShellColour == 1)
        shell.setPrompt("\033[1;36m[\033[1;32mA30B\033[1;36m] \033[1;35m$\033[m ");
    else
        shell.setPrompt("[A30B] $ ");
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
    delay(3000);
    Serial.printf("INITIALISING CORE 1 ...[ ]");
    ax25.begin(Callsign, Icon, 300, TX_EN, D_OUT);
    delay(250);
    Serial.printf("\b\bOK]\r\n");
}

void loop1()
{
    while(Serial1.available() > 0)
        gps.encode(Serial1.read());
    delay(50);
}
