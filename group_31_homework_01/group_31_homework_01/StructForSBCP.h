#ifndef SBCPStruct_H
#define SBCPStruct_H

//SBCP Header
struct HeaderSBCP{
    unsigned int vrsn : 9;
    unsigned int type : 7;
    int length;
};

//SBCP Attribute
struct AttributeSBCP
{
    int type;
    int length;
    char payload[512];
};

//SBCP Message
struct MessageSBCP
{
    struct HeaderSBCP header;
    struct AttributeSBCP attribute[2];
};

//Structure to link and hold client username and socket descriptor
struct ClientInformation
{
    char username[16];
    int fd;
    int clientCount;
};

#endif
