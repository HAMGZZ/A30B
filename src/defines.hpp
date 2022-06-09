#ifndef DEFINES_HPP
#define DEFINES_HPP

#define VERSION 0.1
#define PROG_NAME "A30B"

#define I2C_SDA 0
#define I2C_SCL 1
#define UART_TX 28
#define UART_RX 29
#define TX_EN 24
#define D_OUT 25

#define HB_LED 23

#define GPS_BAUD 9600
#define FSK_BAUD 300


/* Virtual EEPROM LAYOUT:
 * 0                 16       24       28
 * |-----------------|--------|--------|
 *    17b Callsign     8b frq   8b frq
 * 
 */
#define CALLSIGN_LOC    0
#define CALLSIGN_SIZE   17 // 16 Bytes for callsign size
#define DEFAULT_ZERO    1012000000ULL
#define DEFAULT_ONE     1012100000ULL

#endif
