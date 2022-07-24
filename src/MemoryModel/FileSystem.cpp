#include <Arduino.h>
#include "FileSystem.hpp"


void FileSystem::Init()
{
    //Serial.printf("INITIALISING MEMORY ...[ ]");
    //Serial.printf(fs.format()? "\b\bOK]":"\b\bERROR]\r\n");
    Serial.printf("MOUNTING FILESYSTEM ...[ ]");
    if(!fs.begin())
    {
        Serial.printf("\b\bFAIL]\r\n");
        Serial.printf("Filesystem is critical to operation - there was an error mounting it!\r\nHALTING!...\r\n");
        //Tools::HaltAll();
    }
    Serial.printf("\b\bOK]\r\n");
    Serial.printf("READING SETTINGS FILE ...[ ]");
    if(Read())
    {
        Serial.printf("\b\bWARNING]\r\n");
        Serial.printf("Error reading file... Using defaults!\r\n");
        strcpy(Callsign, DEFAULT_CALL);
        strcpy(Icon, DEFAULT_ICON);
        CentreFrequency = DEFAULT_CTR_FRQ;
        FrequencyShift = DEFAULT_SHIFT;
        BaudRate = DEFAULT_BAUD;
        TransmissionDelay = DEFAULT_TR;
        ShellColour = true;
        Serial.printf("=== PLEASE RUN 'save' TO SAVE SETTINGS ===\r\n");
    }
    else
        Serial.printf("\b\bOK]\r\n");    
}

int FileSystem::Read()
{
    if(!fs.exists(SETTINGS_PATH))
        return 1;
    fs:File file = fs.open(SETTINGS_PATH, "r");
    char in[1000] = {0};
    int count = SETTINGS_STORED;
    file.readBytesUntil('\n', in, 999);
    sscanf(in, "%s,%s,%llu,%llu,%lu,%lu", Callsign, Icon, &CentreFrequency, &FrequencyShift, &BaudRate, &TransmissionDelay);
    file.close();
    return 0;
}

void FileSystem::Save()
{
    fs:File file = fs.open(SETTINGS_PATH, "w+");
    file.printf("%s,%s,%llu,%llu,%lu,%lu\n", Callsign, Icon, CentreFrequency, FrequencyShift, BaudRate, TransmissionDelay);
    file.close();
}
