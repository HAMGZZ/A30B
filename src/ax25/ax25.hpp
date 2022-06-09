#ifndef AX25_HPP
#define AX25_HPP

#define FLAG 0xE7
#define CONTROL_FIELD 0x03
#define PROTOCOL_ID 0xF0

class AX25
{
    private:
        char sourceAddress[8] = {0}; //7 + \n
        char packet[332] = {0}; // Entire packet
        
        bool checkDestinationAddress(char * destinationAddress);
        bool checkInformation(char * information);
    public:
        AX25(char * sourceAddress);
        buildPacket(char * destinationAddress, char * digipeaterAddress, char * information);
        ~AX25();
};

#endif
