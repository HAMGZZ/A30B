/*
 * Lewis Hamilton 2022
 * Using the Arduino core for the RP2040 is a cheap (time wise) and fast way
 * to get the product working. In the future I might have time to write it
 * properly using the RP2040 SDK. This would involve me writing things like a
 * GPS NMEA library and the like. Something which I do not have time to do right
 * now...
 * 
 * This main is also too big... all the CLI commands are down the bottom
 * and should be put into another file...
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

void aprsGen(char * array, unsigned long arraySize);

// Blinks HB light.
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
    // Stop CORE 1 from starting while we read settings...
    rp2040.idleOtherCore();
    CORE1LOCK = true;
    CORE1READY = false;

    // Start the serial terminal over USB.
    Serial.begin();

    pinMode(HB_LED, OUTPUT);
    digitalWrite(HB_LED, 1);
    
    // Let's just wait a bit - why the rush?
    delay(2000);
    Serial.printf("-- A30B START --\r\n");
    Serial.printf("V: %0.1f\r\n", VERSION);
    Serial.printf("Type 'licence' at the prompt to show licence.\r\n");
    Serial.printf("\n\r"
                 "   ___   ____ ___  ___ \n\r"
                 "  / _ | |_  // _ \\/ _ )\n\r"
                 " / __ |_/_ </ // / _  |\n\r"
                 "/_/ |_/____/\\___/____/ \n\n\r");

    // Mount the internal file system...
    Serial.printf("MOUNTING FILESYSTEM ...[ ]");
    if (!LittleFS.begin())
    {
        Serial.printf("\b\bFAIL]\r\n");
        Serial.printf("Filesystem is critical to operation - there was an error mounting it!\r\nHALTING!...\r\n");
        Tools::HaltAll();
    }
    Serial.printf("\b\bOK]\r\n");

    // Load the settings...
    settings.Init();

    if (settings.ShellColour == 1)
        shell.setPrompt("\033[1;36m[\033[1;32mA30B\033[1;36m] \033[1;35m$\033[m ");
    else
        shell.setPrompt("[A30B] $ ");

    // Let's the serial buf finish...
    delay(1000);

    // Let's load CORE 1 now...
    Serial.printf("Loading CORE 1 ...\r\n");
    rp2040.resumeOtherCore();
    
    // Wait for CORE 1 to be ready, and wait for buf to be clear
    while(!CORE1READY){}
    delay(2000);
    CORE1LOCK = false;

    shell.begin(Serial, 15);
}

// CORE 0 - Serial shell & program runner
void loop()
{
    shell.loop();
    delay(10); // Release for USB handle
    HeartBeat();
}

// CORE 1 - responsible for tracking and transmission. 
void setup1()
{
    pinMode(TX_EN, OUTPUT);
    pinMode(D_OUT, OUTPUT);
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    Serial1.setTX(UART_TX);
    Serial1.setRX(UART_RX);
    Serial1.begin(GPS_BAUD);

    // Connect to sigen chip...
    Serial.printf("CONNECTING TO Si5356 ...[ ]");
    bool rf_conn;
    rf_conn = rf.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if (!rf_conn)
    {
        Serial.printf("\b\bERROR]\r\nCOULD NOT CONNECT TO RF CHIP!\r\n HALTING!");
        for (;;);
    }
    Serial.printf("\b\bOK]\r\n");

    // Set the frequencies...
    Serial.printf("INITIALISING Si5356 ...[ ]");
    rf.set_freq(settings.ZeroFreq, SI5351_CLK0);
    rf.set_freq(settings.OneFreq, SI5351_CLK1);
    Serial.printf("\b\bOK]\r\n");

    // Get the AX25 gen ready...
    ax25.begin(settings.Callsign, settings.BaudRate, TX_EN, D_OUT);
    delay(2000);

    // We are now ready!
    CORE1READY = true;
}

// CORE 1 loop - reads gps and tx's when ready.
// TODO: IMPLEMENT THIS!
void loop1()
{
    unsigned long prevTime = 0;
    char pack[MAXCHAR] = {0};
    // CORE 1 LOOP LOCK 
    while (!CORE1LOCK)
    {
        while (Serial1.available() > 0)
            gps.encode(Serial1.read());
        delay(50);
        if (millis() - prevTime > Tools::delayTime(gps.speed.kmph()) && gps.sentencesWithFix() > 5)
        {
            prevTime = millis();
            aprsGen(pack, MAXCHAR);
            ax25.buildPacket(pack);
            ax25.shiftOut();
        }
    }
}




/*****************************************************************************
 * APRS PACKET GEN
 *****************************************************************************/

void aprsGen(char * array, unsigned long arraySize)
{
    memset(array, 0, arraySize);
    //Insert all the info into the packet
    sscanf(array,   "!%04d.%02d%c/%05d.%02d%c%c%03d/%03d/A=%06d %s",
                        gps.location.rawLat().deg,
                        gps.location.rawLat().billionths,
                        gps.location.rawLat().negative ? 'S' : 'N',
                        gps.location.rawLng().deg,
                        gps.location.rawLng().billionths,
                        gps.location.rawLng().negative ? 'W' : 'E',
                        settings.Icon,
                        gps.course.deg(),
                        gps.speed.knots(),
                        gps.altitude.feet(),
                        settings.Comment);
}




/*****************************************************************************
 * CLI COMMANDS BELLOW
 *****************************************************************************/

void cmdSet(Shell &shell, int argc, const ShellArguments &argv)
{
    if (argc > 2)
    {
        CORE1LOCK = true;
        rp2040.idleOtherCore();

        // Set the zero frequency.
        if (strcmp(argv[1], "zero") == 0)
        {
            if (Tools::IsNumber(argv[2]))
            {
                Serial.printf("Setting 0 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                settings.ZeroFreq = strtoull(argv[2], nullptr, 0) * 100;
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        // Set the one frequency
        else if (strcmp(argv[1], "one") == 0)
        {
            if (Tools::IsNumber(argv[2]))
            {
                Serial.printf("Setting 1 frequency to: %lld\n\r", strtoull(argv[2], nullptr, 0));
                settings.OneFreq = strtoull(argv[2], nullptr, 0) * 100;
            }
            else
            {
                Serial.printf("Error: Expected number, got: %s\n\r", argv[2]);
            }
        }

        // Set the callsign
        else if (strcmp(argv[1], "callsign") == 0)
        {
            if (strlen(argv[2]) > 7)
                Serial.printf("Warning: Callsign must be less than 7 characters\r\n");
            else
                strcpy(settings.Callsign, argv[2]);
        }

        // Set the APRS icon
        else if (strcmp(argv[1], "icon") == 0)
        {
            if (strlen(argv[2]) > 1)
                Serial.printf("Warning: Symbols are only 1 character!\r\nPlease run 'lssymbol' to list symbols\r\n");
            else
                settings.Icon = argv[2][0];
        }

        else if (strcmp(argv[1], "comment") == 0)
        {
            if (strlen(argv[2]) > 26)
                Serial.printf("Warning: Comment must be less than 26 characters\r\n");
            else
                strcpy(settings.Comment, argv[2]);
        }

        // Set the terminal colour.
        else if (strcmp(argv[1], "colour") == 0)
        {
            if (strcmp(argv[2], "true") == 0)
                settings.ShellColour = 1;
            else if (strcmp(argv[2], "false") == 0)
                settings.ShellColour = 0;
            else
                Serial.printf("Only use true or false.\r\n");
        }

        else
        {
            Serial.printf("Unknown set command: %s \n\r", argv[1]);
        }
        CORE1LOCK = false;
        rp2040.resumeOtherCore();
    }
    else
    {
        Serial.printf("Error, no set verb.\n\r");
    }
}

void cmdStatus(Shell &shell, int argc, const ShellArguments &argv)
{
    CORE1LOCK = true;
    rp2040.idleOtherCore();
    long currentTime = millis();
    Serial.printf("A30B Version %0.1f -- Lewis Hamilton VK2GZZ\n\r", VERSION);
    Serial.printf("== DEV CONFG ==\n\r");
    Serial.printf("-\tCallsign:\t\t %s\r\n", settings.Callsign);
    Serial.printf("-\tICON #:  \t\t %s\r\n", settings.Icon);
    Serial.printf("-\tComment: \t\t %s\t\n", settings.Comment);
    Serial.printf("-\tUpTime:  \t\t %lus > %lum\r\n", (long)(currentTime / 1000), (long)(currentTime / 60000));
    Serial.printf("-\tColour:  \t\t %d\r\n", settings.ShellColour);
    Serial.printf("== GPS STATS ==\r\n");
    Serial.printf("-\tLONG:    \t\t %lf E\r\n", gps.location.lng());
    Serial.printf("-\tLAT:     \t\t %lf N\r\n", gps.location.lat());
    Serial.printf("-\tTIME:    \t\t %lf cs\r\n", gps.time.centisecond());
    Serial.printf("-\tSPEED:   \t\t %lf km/h\r\n", gps.speed.kmph());
    Serial.printf("-\tALT:     \t\t %lf m\r\n", gps.altitude.meters());
    Serial.printf("-\tSAT      \t\t %d fixed\r\n", gps.satellites.value());
    Serial.printf("== X25 STATS ==\r\n");
    Serial.printf("-\tPCK CNT: \t\t %lu\r\n");
    Serial.printf("-\tNXT PCK: \t\t %lu\r\n");
    CORE1LOCK = false;
    rp2040.resumeOtherCore();
}

void cmdTest(Shell &shell, int argc, const ShellArguments &argv)
{
    CORE1LOCK = true;
    rp2040.idleOtherCore();
    if (argc > 2)
    {
        // Test the checksum gen.
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
            // Calc the checksum
            uint16_t result = ax25.checksum(input, strlen(input));
            unsigned long end = micros();
            unsigned long timespend = end - begin;
            Serial.printf("\r\nRESULT: \t 0x%04x\r\n", result);
            Serial.printf("RESULT: \t ");
            Tools::PrintBinary(&result, 16);
            Serial.printf("\r\nCALC TIME:\t %lu uS\r\n", timespend);
        }

        // Test modulation (flip between 0 and 1 TX freqs)
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

        // Test the packet builder.
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

        

        else
        {
            Serial.printf("Unknown test command: %s \n\r", argv[1]);
        }
    }

    // test the baud rate.
    else if(strcmp(argv[1], "baud") == 0)
    {
        float symbolTime = (1/(float)settings.BaudRate) * 1000;
        bool run = true;       
        unsigned long long db = 10000000/settings.BaudRate;
        int out = 0;   
        Serial.printf("Current baud rate: %lu\r\n", settings.BaudRate);
        Serial.printf("The symbol time should be: %f mS\r\n", symbolTime);
        Serial.printf("Transmission time/bit: %llu uS\r\n", db);
        Serial.printf("Press 'q' to quit.\r\n");
        digitalWrite(TX_EN, 1);
        while (run)
        {
            if (Serial.available())
            {
                if (Serial.read() == 'q')
                    run = false;
            }
            Serial.printf("\b%d", out);
            digitalWrite(D_OUT, out);
            out = !out;
            delayMicroseconds(db - 2);
        }
        digitalWrite(TX_EN, 0);
    }

    else if(strcmp(argv[1], "gps") == 0)
    {
        char temp[MAXCHAR] = {0};
        Serial.printf("GPS PACKET:\r\n\r\n");
        aprsGen(temp, MAXCHAR);
        Serial.printf("%s\r\n\r\n", temp);
    }

    else
    {
        Serial.printf("Error, no test verb.\n\r");
    }

    CORE1LOCK = false;
    rp2040.resumeOtherCore();
}

// Calibrate the internal clock and offset.
void cmdCal(Shell &shell, int argc, const ShellArguments &argv)
{
    CORE1LOCK = true;
    rp2040.idleOtherCore();
    bool run = true;
    char a;
    char in[MAXCHAR] = {0};
    long long inFreq = 0;
    long long offset = 0;

    Serial.printf("=== SYSTEM CALIBRATION ===\r\n");
    Serial.printf("THIS WILL OVERWRITE PREVIOUS CALIBRATIONS!\r\n");
    Serial.printf("Setting output of sigen to 10MHz\r\n");
    rf.set_freq(100000000, SI5351_CLK0);
    Serial.printf("Please connect the output to a frequency counter.\r\n");
    Serial.printf("Adjust until the frequency counter reads 10MHz\r\n");
    Serial.printf("Else, press z to enter the frequency manually\r\n\r\n");
    Serial.printf("Press 'q' when done\r\n");
    Serial.printf("   Up:   r   t  y  u  i   o  p\r\n");
    Serial.printf(" Down:   f   g  h  j  k   l  ;\r\n");
    Serial.printf("   Hz: 0.01 0.1 1 10 100 1K 10k\r\n\r\n");

    while (run)
    {
        if (Serial.available())
        {
            a = Serial.read();
            switch (a)
            {
            case 'q':
                Serial.printf("Offset: %ll\r\n");
            case 'r': offset += 1; break;
            case 'f': offset -= 1; break;
            case 't': offset += 10; break;
            case 'g': offset -= 10; break;
            case 'y': offset += 100; break;
            case 'h': offset -= 100; break;
            case 'u': offset += 1000; break;
            case 'j': offset -= 1000; break;
            case 'i': offset += 10000; break;
            case 'k': offset -= 10000; break;
            case 'o': offset += 100000; break;
            case 'l': offset -= 100000; break;
            case 'p': offset += 1000000; break;
            case ';': offset -= 1000000; break;
            default:
                break;
            }
            Serial.printf()
        }
    }

    while (run)
    {
        if (Serial.available())
        {
            a = Serial.read();
            Serial.printf("%c", a);
            if (a == '\r' || a == '\n' || a == ' ')
                run = false;
            else 
                in[strlen(in)] = a;
        }
    }
    inFreq = strtoll(in, nullptr, 0) * 100ULL;
    offset = inFreq - 1000000000ULL;
    Serial.printf("Entered frequency: %llu\r\n", inFreq);
    Serial.printf("Offset: %llu\r\n", offset);
    Serial.printf("Is this offset ok? (y/n): ");
    run = true;
    while (run)
    {
        if (Serial.available())
        {
            if (Serial.read() == 'y' || Serial.read() == 'Y')
            {
                settings.Offset = offset;
                Serial.printf("Saving calibration...\r\n");
                settings.Write();
                Serial.printf("Calibration saved. Please power cycle the device!\r\n");
                delay(2000);
                for(;;);
            }
            else
            {
                Serial.printf("Cancled\r\n");
                run = false;
            }
        }
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
    rp2040.idleOtherCore();
    Serial.printf("Loading configuration...\r\n");
    settings.Read();
    Serial.printf("Configuration loaded.\r\n");
    CORE1LOCK = false;
    rp2040.resumeOtherCore();
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
    if (argc > 1)
    {
        fs:File file = LittleFS.open(argv[1], "a");
        file.printf("%s\r\n", argv[2]);
    }
    else
    {
        Serial.printf("Missing file name.\r\n");
    }
}

void cmdFormat(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("Formatting the file system...");
    Serial.printf(LittleFS.format() ? "OK\r\n" : "ERROR\r\n");
}

void cmdLssmb(Shell &sehll, int argc, const ShellArguments &argv)
{
    Serial.printf(  "S\tDESCRP\t\t\tS\tDESCRP\t\t\tS\tDESCRP\r\n\r\n"
                    "!\tPolice\t\t\t#\tDigi  \t\t\t$\tPhone \r\n"
                    "%%\tDX Cls\t\t\t&\tHF Gtw\t\t\t'\tSPlane\r\n"
                    "(\tSat St\t\t\t*\tSnwMbl\t\t\t+\tRedCrs\r\n"
                    ",\tBoySct\t\t\t-\tHouse \t\t\t.\tX     \r\n"
                    ":\tFire  \t\t\t;\tCamp  \t\t\t<\tMtrClc\r\n"
                    "=\tTrain \t\t\t>\tCar   \t\t\t?\tFilesv\r\n"
                    "A\tAid St\t\t\tB\tBBS   \t\t\tC\tCanoe \r\n"
                    "E\tEyebal\t\t\tG\tGrdSqr\t\t\tH\tHotel \r\n"
                    "I\tTCP/IP\t\t\tK\tSchool\t\t\tM\tMacAPR\r\n"
                    "N\tNTS St\t\t\tO\tBalln \t\t\tP\tPolice\r\n"
                    "R\tRec Ve\t\t\tS\tSpcSht\t\t\tT\tSSTV  \r\n"
                    "U\tBus   \t\t\tV\tATV   \t\t\tW\tNWS St\r\n"
                    "X\tHlcptr\t\t\tY\tYacht \t\t\tZ\tWINAPR\r\n"
                    "[\tJogger\t\t\t\\\tTrngl \t\t\t]\tPBBS  \r\n"
                    "^\tLPlane\t\t\t_\tWthrSt\t\t\t`\tDish  \r\n"
                    "a\tAmbo  \t\t\tb\tbycl  \t\t\td\tFireSt\r\n"
                    "e\tHorse \t\t\tf\tFiretr\t\t\tg\tGlider\r\n"
                    "h\tHospit\t\t\ti\tIOTA  \t\t\tj\tJeep  \r\n"
                    "k\tTruck \t\t\tm\tMicRep\t\t\tn\tNode  \r\n"
                    "o\tEmrgOp\t\t\tp\tRover \t\t\tq\tGrdSqr\r\n"
                    "r\tAnt   \t\t\ts\tShip  \t\t\tt\tTrkStp\r\n"
                    "u\tTruck \t\t\tv\tVan   \t\t\tw\tWater \r\n"
                    "x\tX-APRS\t\t\ty\tYagi\r\n"
                    "There are other symbols available, please lookup the\r\n"
                    "documentation.\r\n"
    );
}

void cmdReady(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("OK$\r\n");
}

void cmdLsSettings(Shell &shell, int argc, const ShellArguments & argv)
{
    Serial.printf(  "%s,%s,%llu,%llu,%ll,%llu$\r\n", 
                    settings.Callsign,
                    settings.Icon,
                    settings.ZeroFreq,
                    settings.OneFreq,
                    settings.Offset,
                    settings.BaudRate);
}

void cmdLic(Shell &shell, int argc, const ShellArguments &argv)
{
    Serial.printf("MIT License\r\n"
                  "\r\n"
                  "Copyright (c) 2022 Lewis Hamilton\r\n"
                  "\r\n"
                  "Permission is hereby granted, free of charge, to any person obtaining a copy\r\n"
                  "of this software and associated documentation files (the \"Software\"), to deal\r\n"
                  "in the Software without restriction, including without limitation the rights\r\n"
                  "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\r\n"
                  "copies of the Software, and to permit persons to whom the Software is\r\n"
                  "furnished to do so, subject to the following conditions:\r\n"
                  "\r\n"
                  "The above copyright notice and this permission notice shall be included in all\r\n"
                  "copies or substantial portions of the Software.\r\n"
                  "\r\n"
                  "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n"
                  "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\r\n"
                  "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\r\n"
                  "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\r\n"
                  "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\r\n"
                  "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\r\n"
                  "SOFTWARE.\r\n");
}


ShellCommand(status, "status -> Gives overall status of the system", cmdStatus);

ShellCommand(set, "set [option] [value] \n\r"
                  "\t-> '** set zero 10149200' sets the zero mark to 10.120,000 MHz\n\r"
                  "\t                         Please restart the device to apply\n\r"
                  "\t-> '** set one 10149400' sets the one mark to 10.121,000 MHz\n\r"
                  "\t                         Please restart the device to apply\n\r"
                  "\t-> 'set callsign *****' sets the callsign - can be up to 7 chars\n\r"
                  "\t-> 'set icon *' sets the APRS icon to be transmitted\r\n"
                  "\t-> 'set colour true/false' sets the shell colour mode",
             cmdSet);

ShellCommand(test, "test [unit] [options] \n\r"
                   "\t-> 'test crc 12345678' returns the CRC-CCITT result from message '12345678'\r\n"
                   "\t-> 'test builder testpacket' returns the build AX25 packet & allows test tx\r\n"
                   "\t-> 'test modulation 10' modulates signal with 1's and 0's being txd for '10' seconds\r\n"
                   "\t-> 'test baud' test baud rate"
                   "\t-> 'test gps' test gps packet gen",
             cmdTest);

ShellCommand(calibrate, "calibrate -> calibrate the local oscilator offset", cmdCal);

ShellCommand(save, "save -> Saves the system configuration", cmdSave);

ShellCommand(load, "load -> Lods the system configuration", cmdLoad);

ShellCommand(read, "read [filename] -> read /settings", cmdRead);

ShellCommand(ls, "ls -> List files", cmdLs);

ShellCommand(mkdir, "mkdir [dirname] -> mkdir /newdir", cmdMkdir);

ShellCommand(rmdir, "rmdir [dirname] -> rmdir /deldir", cmdRmdir);

ShellCommand(put, "put [filename] [data] -> put myFile \"This is what is being placed into file\"", cmdPut);

ShellCommand(format, "format -> Formats the file system - THIS WILL CLEAR CALIBRATION", cmdFormat);

ShellCommand(lssymbol, "lssymbol -> list available symbols", cmdLssmb);

ShellCommand(ready, "ready -> returns ok - ONLY FOR AUTOMATIC CONTROL", cmdReady);

ShellCommand(lssettings, "lssettings -> list settings - ONLY FOR AUTOMATIC CONTROL", cmdLsSettings);

ShellCommand(licence, "licence -> show licence", cmdLic);
