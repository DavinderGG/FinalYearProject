#pragma once
//#include <cstdint>
#include "ProtMessage.h"
//#include <iostream>
#include "PingApp.h"
#include "config.h"

struct routingTableEntry {
    bool isValid;
    int hopCount ;
    int sequenceNumber;
    bool validSequenceNumberflag;
    uint8_t nextHop;
    int lifetime;
    int rreqRetries;
    int rreqTimer;
    int packetsBuffered;
    ProtMessage* head;
    ProtMessage* tail;
};

struct RREQrequest {
    uint8_t node;
    int RREQid;
    int time;
};

const int ALLOWED_HELLO_LOSS = 2;
const int HELLO_INTERVAL = 5;
const int RREQ_RETRIES = 2;
const int ACTIVE_ROUTE_TIMEOUT = 10;
const int BROADCAST_ADDRESS = 255;
const int PATH_DISCOVERY_TIME = 20;
const int MAX_BUFFERED_PACKETS = 5;
const int RREQ_HISTORY_SIZE = 5;

class PingApp;

class AODV
{
public:
    AODV(int addr, POINTER_TYPE(PingApp) newApp) : myAddress(addr), app(newApp) {
        rreqSent=0;
        rreqReceived=0;
        rrepSent=0;
        rrepReceived=0;
        helloSent=0;
        helloReceived=0;
        rerrSent=0;
        rerrReceived=0;
        dataSent=0;
        dataReceived=0;
        mySeqNum = 0;
        myRREQid = 0;
        countSeconds=0;
        for (int i = 0;i<MAX_NODES;i++){
            routingTable[i].isValid = false;
            routingTable[i].hopCount = 0;
            routingTable[i].sequenceNumber = 0;
            routingTable[i].validSequenceNumberflag = false;
            routingTable[i].nextHop = 0;
            routingTable[i].lifetime = 0;
            routingTable[i].rreqRetries = 0;
            routingTable[i].rreqTimer = 0;         
            routingTable[i].head = NULL;
            routingTable[i].tail = NULL;
            routingTable[i].packetsBuffered = 0;
        }
    };
    ~AODV();
    void printStats();
    void secondTimerExpired();
    void onReceive(PacketBuffer& buf);
    void appDatagramSend(ProtMessage& msg);

private:
    // statistics
    int rreqSent;
    int rreqReceived;
    int rrepSent;
    int rrepReceived;
    int helloSent;
    int helloReceived;
    int rerrSent;
    int rerrReceived;
    int dataSent;
    int dataReceived;
    int countSeconds;

    routingTableEntry routingTable[MAX_NODES];
    uint8_t myAddress;
    int myRREQid;
    int mySeqNum;
    RREQrequest RREQhistory[RREQ_HISTORY_SIZE];
    POINTER_TYPE(PingApp) app;

    void debugPrintRoutingTable();
    void sendHelloMessage();
    void updateActiveTimeout();
    void sendRERR(uint8_t unreachableNode,  uint8_t dest);
    ProtMessage& receivePacketFromRadio();
    void sendRREQ(uint8_t node);
    void processRREQ(ProtMessage& msg);
    bool isRepeatRREQ(uint8_t node, int id);
    void setRREQTimer(ProtMessage& msg);
    void updateRoutingTable(ProtMessage& msg);
    void addRoutingTableEntry(ProtMessage& msg);
    void forwardDataToApplication(ProtMessage& msg);
    void handleData(ProtMessage& msg);
    void addRoute(uint8_t source, uint8_t myAddress);
    bool hasRoute(uint8_t destination);
    void sendToRadio(ProtMessage& msg);
    void handleRREQ(ProtMessage& msg);
    void forwardToNextHop(ProtMessage& msg);
    void handleRREP(ProtMessage& msg);
    void handleRERR(ProtMessage& msg);
    void updateRREQhistory();
    
};
