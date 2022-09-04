#include "Settings.hpp"

int Settings::Init()
{
    Serial.printf("READING SETTINGS FILE ...[ ]");
    if(Read())
    {
        Serial.printf("\b\bWARNING]\r\n");
        Serial.printf("Error reading file... Using defaults!\r\n");
        strcpy(Callsign, DEFAULT_CALL);
        Icon = DEFAULT_ICON;
        ZeroFreq = DEFAULT_ZERO_FREQ;
        OneFreq = DEFAULT_ONE_FREQ;
        BaudRate = DEFAULT_BAUD;
        Offset = 0;
        TransmissionDelay = DEFAULT_TR;
        ShellColour = 1;
        Serial.printf("=== PLEASE RUN 'save' TO SAVE SETTINGS ===\r\n");
    }
    else
        Serial.printf("\b\bOK]\r\n");    
    return 0;
}

int Settings::Read()
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
    sscanf(in, "%llu,%llu,%lu,%lu,%d,%llu,", &ZeroFreq, &OneFreq, &BaudRate, &TransmissionDelay, &ShellColour, &Offset);
    memset(in, 0, MAXCHAR);
    file.close();
    return 0;
}

int Settings::Write()
{
    fs:File file = LittleFS.open(SETTINGS_PATH, "w");
    file.printf("%s\n%s\n%llu,%llu,%lu,%lu,%d,%llu,\n\r", Callsign, Icon, ZeroFreq, OneFreq, BaudRate, TransmissionDelay, ShellColour, Offset);
    file.close();
    return 0;
}