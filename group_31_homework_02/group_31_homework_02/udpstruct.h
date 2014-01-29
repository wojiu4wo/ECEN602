#include <stdint.h>
struct RRQ_Packet
{
	uint16_t opcode;
	char *filename;
        uint8_t endOfFile;
        char mode[512];
        uint8_t endOfFile1;
};

struct DATA_Packet
{
	uint16_t opcode;
	uint16_t blockNo;
        char data[512];
};

struct ACK_Packet
{
	uint16_t opcode;
	uint16_t blockNo;
};

struct ERROR_Packet
{
	uint16_t opcode;
	uint16_t errorCode;
        char errorMessage[512];
        char endOfFile;
};

struct client{
	  int clientfd;
	  //struct sockaddr addr_in;
	  int state;     //0 : inactive    1 : waiting for ack_pkt
	  int block_no;
	  FILE* fp; 
};
