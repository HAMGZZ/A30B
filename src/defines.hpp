#ifndef DEFINES_HPP
#define DEFINES_HPP

#define VERSION 0.1
#define PROG_NAME "A30B"

#define MAXCHAR 2048
#define I2C_SDA 0
#define I2C_SCL 1
#define UART_TX 28
#define UART_RX 29
#define TX_EN 24
#define D_OUT 25

#define HB_LED 23

#define GPS_BAUD 9600

#define CALLSIGN_SIZE   8 // 7 Bytes for callsign size
#define ICON_SIZE 2
#define COMMENT_SIZE 27

#define SETTINGS_PATH "/Settings.cfg"

#define DEFAULT_CALL "VKXABC"
#define DEFAULT_ICON "000"
#define DEFAULT_ZERO_FREQ   1014920000UL // This 10.147.6 USB with 1600/1800 tones
#define DEFAULT_ONE_FREQ    1014940000UL
#define DEFAULT_TR 600 // 10 mins
#define DEFAULT_BAUD 300

#define SETTINGS_STORED 6




#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
#endif
