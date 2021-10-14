#pragma once

#include "ProtMessage.h"
#include "aodv.h"
#include "config.h"

struct PingStatsRecord{
    int pingsReceived;
    int pingsSent;
    int repliesReceived;
    int repliesSent;
    int maxLatency;
    int latencyTotal; 
};
class AODV;

class PingApp
{
public:
    PingApp(int addr, int appMode) : myAddress(addr), myMode(appMode),
        sendPings(appMode & 1), sendReplies(appMode & 2),
        myRoutingLayer(NULL) {
    countSeconds = 0;
    for (int i = 0;i<MAX_NODES;i++){
            pingStats[i].pingsReceived = 0;
            pingStats[i].pingsSent = 0;
            pingStats[i].repliesReceived = 0;
            pingStats[i].repliesSent = 0;
            pingStats[i].maxLatency = 0;
            pingStats[i].latencyTotal = 0;
        
        }
    }
    ~PingApp();
    void secondTimerExpired();
    void printStats();
    void onReceive(ProtMessage& msg);
    void setRoutingLayer(POINTER_TYPE(AODV) aodv);

private:
    // statistics
    PingStatsRecord pingStats[MAX_NODES];
    int countSeconds;

    int myAddress;
    int myMode;
    POINTER_TYPE(AODV) myRoutingLayer;
    bool sendPings;
    bool sendReplies;


};


// mode of the ping app
// bit 0 means send pings
// bit 1 means send replies
const int MODE_SILENT = 0;
const int MODE_PING_ONLY = 1;
const int MODE_REPLY_ONLY = 2;
const int MODE_PING_REPLY = 3;



