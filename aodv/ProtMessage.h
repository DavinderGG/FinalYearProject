#pragma once


#include "MicroBit.h"

#ifdef PC_SIMULATION

#include <cstdint>
#include <vector>
#include <string>
#include "PacketBuff.h"
#include "ManagedStr.h"
#endif

// Message types
#define MSG_APP_DATA 1
#define MSG_HELLO 4
#define MSG_RREQ 5
#define MSG_RREP 6
#define MSG_RERR 7
#define MSG_APP_PING 100
#define MSG_APP_PING_REPLY 101

#define PAYLOAD_SIZE 12


class ProtMessage
{
public:
    // constructor
    ProtMessage();
    
    //destructor
    ~ProtMessage();

    bool pack(PacketBuffer& buf);

    bool unpack(PacketBuffer& buf);

    // return a nice string of the message in a nice format
    ManagedString convert_to_str();

    uint8_t pktType;
    uint8_t source; // the immediate sender of the packet
    uint8_t destination; // the immediate recipient of the packet
    uint8_t originator; // where the packet originally came from
    uint8_t finalDest; // the ultimate destination of the packet
    int finalDestSeq; // the sequence number of the final destination
    int origSeq ; // originators sequence number
    int hopCount; // number of hops
    int lifetime; // how long the packet can live
    uint8_t payload[PAYLOAD_SIZE]; //  bytes of payload
    int destCount;
    uint8_t unreachableDestAddr;
    int unreachableDestSeq ;
    bool unavailableSeq;
    int RREQid;
    ProtMessage* next;
    ProtMessage* prev;

};





