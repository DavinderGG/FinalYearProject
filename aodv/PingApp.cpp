#include "PingApp.h"

#include "MicroBit.h"

#include "config.h"




PingApp::~PingApp() {
    printStats();
}



void PingApp::printStats() {
        debugPrint("\n\n********************* PING STATS *****************************");
        debugPrint("Node\tPingIn\tPingOut\tRepOut\tRepIn:\tTPut:\tLat\tMax Lat\tPDR");
        for (int i =0;i<NUM_NODES;i++){
                int throughput = 0;
                int latency = 0;
                int max_latency = 0;
                int PDR = 0;
                if ((pingStats[i].repliesReceived > 0) && (pingStats[i].pingsSent > 0) && (countSeconds > 0)) {
                    throughput = (pingStats[i].pingsSent + pingStats[i].pingsReceived + pingStats[i].repliesSent + pingStats[i].repliesReceived) *60 / countSeconds;
                    latency = pingStats[i].latencyTotal / pingStats[i].repliesReceived;
                    max_latency = pingStats[i].maxLatency;
                    PDR = (pingStats[i].repliesReceived * 100) / pingStats[i].pingsSent;
                } 
            debugPrint("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",i,pingStats[i].pingsSent, pingStats[i].pingsReceived, pingStats[i].repliesSent, pingStats[i].repliesReceived,
                throughput, latency, max_latency, PDR);
        }
}

void PingApp::secondTimerExpired() {
    // called every second

    countSeconds++;
    if ((countSeconds % 5) == 0) {
        
        printStats();
    }
    //if (countSeconds < 10) return;
    // if sending pings is disabled by mode, then return
    if (!sendPings) return;
    debugPrint("About to send a ping");
    // roll the dice to see if we should send a ping
    int random = uBit.random(6);
    if (random > 3) return;
    // add small random delay to avoid everything being synchronised to 1 second boundaries
    uBit.sleep(uBit.random(500));
    ProtMessage msg;
    int tempAddress = rand() % NUM_NODES;
    while (tempAddress == myAddress) {
        tempAddress = rand() % NUM_NODES;
    }
    debugPrint("Sending ping...%d",tempAddress);
    msg.source = myAddress;
    msg.originator = myAddress;
    msg.finalDest = tempAddress;
    msg.pktType = MSG_APP_DATA;
    msg.payload[0] = MSG_APP_PING;
    //// add time into the payload as 32 bit number (wraps every 49 days)
    uint32_t now = uBit.systemTime() & 0xffffffff;
    msg.payload[1] = (now & 0xff000000) >> 24;
    msg.payload[2] = (now & 0x00ff0000) >> 16;
    msg.payload[3] = (now & 0x0000ff00) >> 8;
    msg.payload[4] = (now & 0x000000ff);

    myRoutingLayer->appDatagramSend(msg);
    
    pingStats[msg.finalDest].pingsSent++;
    
    debugPrint("Ping Sent to %d with time 0x%x",  msg.finalDest, now);
}

void PingApp::onReceive(ProtMessage& msg) {

    // first byte of payload is Ping Type
    int pktType = msg.payload[0];

    // check if packet is for us
    if (msg.finalDest == myAddress) {
        //    if the event is a ping, it will send a ping reply and display a P
        if (pktType == MSG_APP_PING) {
            pingStats[msg.originator].pingsReceived++;
            if (sendReplies) {
                ProtMessage reply;
                reply.source = myAddress;
                reply.originator = myAddress;
                reply.finalDest = msg.originator;
                reply.pktType = MSG_APP_DATA;
                reply.payload[0] = MSG_APP_PING_REPLY;
                // copy in timestamp
                reply.payload[1] = msg.payload[1];
                reply.payload[2] = msg.payload[2];
                reply.payload[3] = msg.payload[3];
                reply.payload[4] = msg.payload[4];

                debugPrint("Ping Rx from %d, sending reply", reply.finalDest);
                myRoutingLayer->appDatagramSend(reply);
                pingStats[msg.originator].repliesSent++;
                uBit.display.printAsync(msg.originator);
            }
        }

        //if the ping is a reply it will increment the pingCount by 1 and display it
        else if (pktType == MSG_APP_PING_REPLY) {
            pingStats[msg.originator].repliesReceived++;
            // measure round trip time
            uint32_t sendTime = msg.payload[1] << 24 |
                msg.payload[2] << 16 |
                msg.payload[3] << 8 |
                msg.payload[4];
            uint32_t now = uBit.systemTime() & 0xffffffff;
            int delta = now - sendTime;
            debugPrint("Ping Reply Rx from %d with timestamp 0x%x at time 0x%x making round trip time %d ms", msg.originator, sendTime, now, delta);
            if (delta > pingStats[msg.originator].maxLatency) pingStats[msg.originator].maxLatency = delta;
            pingStats[msg.originator].latencyTotal += delta;
        }
        else {
            debugPrint("Unexpected application packet /n");
            //debugPrint("Unexpected application packet %s/n", msg.convert_to_str());

        }
    }
}

void PingApp::setRoutingLayer(POINTER_TYPE(AODV) aodv) {
    myRoutingLayer =  aodv;
}


