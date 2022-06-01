/*
 * Lewis Hamilton 2022
 * Using the Arduino core for the RP2040 is a cheap (time wise) and fast way
 * to get the product working. In the future I might have time to write it
 * properly using the RP2040 SDK. This would involve me writing things like a 
 * GPS NMEA library and the like. Something which I do not have time to do right
 * now...
 */

#include <Arduino.h>
#include <Wire.h>

#include "defines.hpp"
#include "Logger.hpp"
#include "si5351.h"
#include "TinyGPS++.h"

Si5351 rf;
TinyGPSPlus gps;

// CORE 0 Responsible for Serial prompt.
void setup() 
{
    Serial.begin();
    delay(1000);
    Serial.print("A30B - VK2GZZ - 2022");
}

void loop() 
{
    //SHELL
    //HEARTBEAT
}


// CORE 1 Responsible for GPS and Shifting data.
void setup1()
{
    delay(5000); // WAIT FOR CORE 1 to START
    Logger log("IOSETUP", INFO);
    
    
    bool rf_conn;
    log.Send(INFO, "SETTING I2C PINS > ", I2C_SDA, I2C_SCL);
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    log.Send(INFO, "CONNECTING TO RF CHIP");
    rf_conn = rf.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if(!rf_conn)
    {
       log.Send(FATAL, "COULD NOT CONNECT TO RF CHIP");
    }
    log.Send(INFO, "CONNECTED TO RF CHIP");
    log.Send(INFO, "SETTING UP RF");
    // rf.set_freq(FREQ_0, SI5351_CLK0);
    // rf.set_freq(FREQ_1, SI5351_CLK1);
    pinMode(TX_EN, OUTPUT);
    pinMode(D_OUT, OUTPUT);



    log.Send(INFO, "STARTING UART PINS > ", UART_TX, UART_RX);
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
