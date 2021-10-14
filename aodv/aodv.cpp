//#include <iostream>
#include "config.h"
#include <assert.h>
#include "aodv.h"
#include "ProtMessage.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Destructor - print statistics
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AODV::~AODV() {
    printStats();
}

void AODV::printStats() {
    debugPrint("AODV stats: rreqSent %d, rreqReceived %d, rrepSent %d, \nrrepReceived %d, helloSent %d, helloReceived %d, dataSent %d, dataReceived %d, countSeconds %d",
        rreqSent, rreqReceived, rrepSent, rrepReceived, helloSent, helloReceived, dataSent, dataReceived, countSeconds);
    int overhead = rreqSent + rreqReceived + rrepSent + rrepReceived + helloSent + helloReceived + rerrSent + rerrReceived;
    if ((dataSent + dataReceived) > 0 && (countSeconds > 0)) {
        debugPrint("AODV stats: Routing_overhead(%%) %d, throughput(pkts/min) %d",
            overhead * 100 / (dataSent + dataReceived),
            (dataSent + dataReceived) * 60 / countSeconds);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   appDatagramSend - used by the app to send a datagram through the network
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::appDatagramSend(ProtMessage& msg) {
    msg.originator = myAddress;
    handleData(msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   secondTimerExpired - the function called once a second to operate the time related aspects of the protocol
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::secondTimerExpired() {
    countSeconds++;
    if ((countSeconds % 20) == 0) {
        printStats();
        debugPrintRoutingTable();
    }
    updateActiveTimeout();
    updateRREQhistory();
    
    //send Hello message to neighbours, then update the active timeout
    if ((countSeconds % HELLO_INTERVAL) == 0) {
        //sendHelloMessage();  
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   forwardDataToApplication - a datagram is received so pass to higher layer app
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::forwardDataToApplication(ProtMessage& msg) {
    // Pass on the payload of the data
    app->onReceive(msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   debugPrintRoutingTable - print out the routing table to debug output
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AODV::debugPrintRoutingTable() {


    ManagedString s("\n****************** ROUTING TABLE ***************\nNode\tValid\tNxtHop\tHopCnt\tSequen\tValSeq\tLife\tBuff\n");
    // now print out the table
    for (int i = 0; i < NUM_NODES; i++) {
        if (i == myAddress) {
            s = s + "  "+ManagedString(i)+ManagedString("\t-\t-\t-\t-\t-\t-\t-\n");
        } 
        else {
            routingTableEntry* e = &routingTable[i];
            char str[1024];
            std::snprintf(str,1024, "  %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                i,e->isValid,e->nextHop,e->hopCount,e->sequenceNumber, e->validSequenceNumberflag,
                e->lifetime, e->packetsBuffered);
            s = s + str;
        }
    }
    // clear the status box

    debugPrintStatus(s);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   sendHelloMessage - forms a Hello message and sends to the radio for broadcast
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::sendHelloMessage() {
    // form a new Hello message, populate fields and then send to radio
    ProtMessage helloMessage;
    mySeqNum++;
    helloMessage.destination = BROADCAST_ADDRESS;
    helloMessage.finalDest = myAddress; // the destination for which the route is supplied is this node sending hello
    helloMessage.finalDestSeq = mySeqNum;
    helloMessage.source = myAddress;
    helloMessage.originator = myAddress;
    helloMessage.origSeq = mySeqNum;
    helloMessage.lifetime = ALLOWED_HELLO_LOSS * HELLO_INTERVAL;
    helloMessage.hopCount = 0;
    helloMessage.pktType = MSG_HELLO;
    sendToRadio(helloMessage);
    debugPrint("Sending hello");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   updateActiveTimeout - go through each node in the table and deal with any out of date routes
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::updateActiveTimeout() {
    // for each node in the table, decrement the ActiveTimeout and if any hit zero, send an RERR
    for (int i = 0; i < MAX_NODES; i++) {
        if (routingTable[i].isValid) {
            routingTable[i].lifetime--;
            if (routingTable[i].lifetime == 0) {
                sendRERR(i, BROADCAST_ADDRESS);
                routingTable[i].isValid = false;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   sendRERR - for a given node, transmit an RERR to signify the route is no longer valid
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::sendRERR(uint8_t unreachableNode,  uint8_t dest) {
    debugPrint("Sending RERR regarding node %d to %d", unreachableNode, dest);
    mySeqNum++;

    //form a new RERR message, populate fields and then send to radio
    ProtMessage RERR;
    RERR.finalDest = dest;
    // check whether broadcast or directed
    if (dest == BROADCAST_ADDRESS) RERR.destination = BROADCAST_ADDRESS;
    else RERR.destination = routingTable[RERR.destination].nextHop;
    RERR.destCount = 1;
    RERR.source = myAddress;
    RERR.originator = myAddress;
    RERR.unreachableDestAddr = unreachableNode;
    RERR.unreachableDestSeq = routingTable[unreachableNode].sequenceNumber;
    RERR.pktType = MSG_RERR;
    sendToRadio(RERR);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   sendRREQ - construct and send an RREQ to discover a route
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::sendRREQ(uint8_t node) {


    //form a rreq, populate fields and then sent to radio
    ProtMessage RREQ;
    RREQ.pktType = MSG_RREQ;
    if (routingTable[node].validSequenceNumberflag) {
        RREQ.finalDestSeq = routingTable[node].sequenceNumber;
        RREQ.unavailableSeq = false;
    }
    else {
        RREQ.unavailableSeq = true;
        RREQ.finalDestSeq = 0;
    }
    RREQ.hopCount = 0;
    RREQ.RREQid = myRREQid;
    myRREQid++;
    mySeqNum++;
    RREQ.origSeq = mySeqNum;
    RREQ.originator = myAddress;
    RREQ.finalDest = node;
    RREQ.source = myAddress;
    RREQ.destination = BROADCAST_ADDRESS;
    sendToRadio(RREQ);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   updateRoutingTable - updates the routing table based on the received message
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::updateRoutingTable(ProtMessage& msg) {
    //check if there exists an entry, if not then create it
    // update destination sequence number
    // set the ActiveTimeout
    // but check if it was destined as broadcast first


    if (msg.source != myAddress) {
        // now update based on the source of the message - this is the immediate sender who can therefore be considered a neighbour
        assert(msg.source < MAX_NODES);
        routingTableEntry* entry = &routingTable[msg.source];
        entry->isValid = true;
        entry->hopCount = 1; // immediate neighbour
        entry->rreqRetries = 0;
        entry->rreqTimer = 0;
        entry->nextHop = msg.source;
        entry->lifetime = ACTIVE_ROUTE_TIMEOUT;
        debugPrint("Updating direct route to SRC %d ", msg.source);
    }

    if (msg.originator != myAddress) {
        // now update based on the originator of the message. In this case, we have a sequence number to deal with
        assert(msg.originator < MAX_NODES);
        routingTableEntry* entry = &routingTable[msg.originator];
        entry->isValid = true;
        // check sequence number before updating route - it is only applicable to RREQ and RREP
        if ((msg.pktType == MSG_RREQ) || (msg.pktType == MSG_RREP)) {
            if (!entry->validSequenceNumberflag || (msg.origSeq > entry->sequenceNumber)) {
                entry->hopCount = msg.hopCount + 1;
                entry->rreqRetries = 0;
                entry->rreqTimer = 0;
                entry->sequenceNumber = msg.origSeq;
                entry->validSequenceNumberflag = true;
                entry->nextHop = msg.source;
                debugPrint("Updating route to ORIG %d via hop %d", msg.originator, msg.source);
                debugPrintRoutingTable();
            }
        }
        entry->lifetime = ACTIVE_ROUTE_TIMEOUT;
    }

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   handleData - process a received datagram 
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void AODV::handleData(ProtMessage& msg) {
    // update the routing table based on this packet 
    updateRoutingTable(msg);
    //check if the datagram is for this node, then check if the destination is in the routing table, if not check the rreq_retries limit. If under the limit send an RREQ
    if (msg.finalDest == myAddress) {
        forwardDataToApplication(msg);
    }
    else
    {
        if (hasRoute(msg.finalDest)) {
            forwardToNextHop(msg);
        }
        else {
            // no route, consider sending RREQ if this node is originator
            if (msg.originator == myAddress) {
                debugPrint("Trying to send to %d but no route, sending RREQ first", msg.finalDest);
                routingTableEntry* entry = &routingTable[msg.finalDest];
                if ((entry->rreqRetries <= RREQ_RETRIES) && (entry->packetsBuffered <= MAX_BUFFERED_PACKETS)) {
                    sendRREQ(msg.finalDest);
                    debugPrint("about to use queue");
                    ProtMessage* newMsg = new ProtMessage;
                    assert(newMsg);
                    *newMsg = msg;
                    if ((entry->head == NULL) && (entry->tail == NULL)){
                            entry->head = newMsg;
                            entry->tail = newMsg;
                            newMsg->next = NULL;
                            newMsg->prev = NULL;
                    }
                    else{
                            entry->tail->next = newMsg;
                            newMsg->prev = entry->tail;
                            newMsg->next = NULL;
                            entry->tail = newMsg;
                    }
                    debugPrint("queue used");
                    entry->packetsBuffered++;
                } // else discard message
                else
                {
                    debugPrint("Discarding message due to RREQ retries or buffer full");
                }
            }
            else {
                // no route to destination so cannot forward this data packet - generate RERR to originator
                sendRERR(msg.finalDest, msg.originator);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   hasRoute - returns true/false dependent on whether a route is known in the routing table
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool AODV::hasRoute(uint8_t node) {
    return routingTable[node].isValid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   sendToRadio - transmit a packet via the radio 
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::sendToRadio(ProtMessage& msg) {
    // need to pack message into PacketBuffer
    // then send to radio

    //update statistics
    switch (msg.pktType) {
    case MSG_APP_DATA:
        dataSent++;
        break;
    case MSG_RREQ:
        rreqSent++;
        break;
    case MSG_RREP:
        rrepSent++;
        break;
    case MSG_RERR:
        rerrSent++;
        break;
    case MSG_HELLO:
        helloSent++;
        break;
    default:
        debugPrint("sendToRadio - unknown packet type %d ", msg.pktType);
    }


    PacketBuffer buff(PACKET_BUFFER_SIZE);
    msg.pack(buff);
    uBit.radio.datagram.send(buff);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   processRREQ -  process a RREQ for this node
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::processRREQ(ProtMessage& msg) {
    // increment my sequence number
    mySeqNum++;
    
    // compose RREP
    ProtMessage reply;
    reply.pktType = MSG_RREP;
    reply.origSeq = mySeqNum;
    reply.source = myAddress;
    reply.originator = myAddress;
    reply.finalDest = msg.originator; // the node that originally requested the RREQ
    reply.hopCount = msg.hopCount; // so the originator knows how far away this node is via this route
    reply.lifetime = ACTIVE_ROUTE_TIMEOUT;
    if (!hasRoute(reply.finalDest)) {
        debugPrint("ERROR - Can't find route to send RREP to %d", reply.finalDest);
    }
    else {
        reply.destination = routingTable[reply.finalDest].nextHop;
        sendToRadio(reply);
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   updateRREQhistory - ages the RREQ history 
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 void AODV::updateRREQhistory() {
    for (int i=0; i < RREQ_HISTORY_SIZE; i++) {
        if (RREQhistory[i].time >0 ) {
            RREQhistory[i].time--;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   isRepeatRREQ 
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool AODV::isRepeatRREQ(uint8_t newNode, int newRREQid) {
    // check if this request was already recorded in the history of requests received by this node
    for (int i=0; i < RREQ_HISTORY_SIZE; i++) {
        if ((RREQhistory[i].node == newNode) && (RREQhistory[i].RREQid == newRREQid)) {
            RREQhistory[i].time = PATH_DISCOVERY_TIME;
            return true;
        }
    }
    
    // not found - so find an empty entry if one exists
    for (int i=0; i < RREQ_HISTORY_SIZE; i++) {
        if (RREQhistory[i].time ==0) {
            RREQhistory[i].node = newNode;
            RREQhistory[i].RREQid = newRREQid;
            RREQhistory[i].time = PATH_DISCOVERY_TIME;
            return false;
        }
    }
    
    // no room to store entry - so discard
    debugPrint("Cannot store RREQ in history so discarding. RREQ from %d, id %d", newNode, newRREQid);
    return false;
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   handleRREQ - process a received RREQ 
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::handleRREQ(ProtMessage& msg) {
    // first check if this is a duplicate request. As RREQs get re-broadcast ie flooded, we may receive the same r
    // RREQ several times from different directions and that can confuse the routing table
    // RREQ uniqueness is determined by Originator address and RREQ ID
    // check if RREQ is originated from me. As RREQs are broadcast, I could get a retransmission of my own RREQ

    if ((msg.originator == myAddress) || (isRepeatRREQ(msg.originator, msg.RREQid)))
    {
        debugPrint("Duplicate RREQ from %d is %d - discarding", msg.originator, msg.RREQid);
        return;
    }

    updateRoutingTable(msg); // create/update the route to the previous hop and originator

    if (msg.finalDest == myAddress) {
        processRREQ(msg);
    }
    else {
        // not for me - so forward on
        forwardToNextHop(msg);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   forwardToNextHop - used to send a message to the next hop in the route
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::forwardToNextHop(ProtMessage& msg) {
    if (msg.pktType == MSG_APP_DATA) {
        // update timers in routing table if it is a data frame
        routingTable[msg.originator].lifetime = ACTIVE_ROUTE_TIMEOUT;
        routingTable[msg.source].lifetime = ACTIVE_ROUTE_TIMEOUT;
        if (msg.finalDest != BROADCAST_ADDRESS) {
            routingTable[msg.finalDest].lifetime = ACTIVE_ROUTE_TIMEOUT;
        }
        if (msg.destination != BROADCAST_ADDRESS) {
            routingTable[msg.destination].lifetime = ACTIVE_ROUTE_TIMEOUT;
        }
    }
    // increase hop count by 1 for RREQ
    if (msg.pktType == MSG_RREQ) msg.hopCount = msg.hopCount + 1;
    // if it is a broadcast, then forward the broadcast - otherwise use the routing information
    if (msg.destination != BROADCAST_ADDRESS) {
        assert(msg.finalDest < MAX_NODES);
        assert(routingTable[msg.finalDest].isValid == true);
        msg.destination = routingTable[msg.finalDest].nextHop;
        debugPrint("Forward packet to finaldest %d, type= %d dest = %d", msg.finalDest,msg.pktType, msg.destination);
    }
    msg.source = myAddress;
    sendToRadio(msg);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   handleRREP - deal with a received RREP message by updating routing table and transmitting any pending datagrams
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::handleRREP(ProtMessage& msg) {
    updateRoutingTable(msg);
    if (msg.finalDest == myAddress) {
        debugPrint("RREP is for me");
        //update routing table
        
        // empty the waitingForRoute queue
        // iterate through queue and for each packet check whether we now have a route
        // if we have a valid route, we can dequeue that packet and send to radio
        // otherwise other packets stay in the waitingForRoute queue
        
        assert(msg.originator<MAX_NODES);
        ProtMessage* head = routingTable[msg.originator].head;

        while (head != NULL){
            sendToRadio(*head);
            debugPrint("Dequeued packet for %d and sending via %d", head->finalDest, head->destination);    
            ProtMessage* old = head;
            head = head->next;
            delete old;    
        }
        routingTable[msg.originator].head = NULL;
        routingTable[msg.originator].tail = NULL;
        routingTable[msg.originator].packetsBuffered = 0;
        
    }
    else if (msg.destination != BROADCAST_ADDRESS)
    {
        debugPrint("Fowarding RREP to next hop");
        // forward the RREP to the next hop
        forwardToNextHop(msg);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   handleRERR - deal with a received RERR by invalidating necessary routes
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AODV::handleRERR(ProtMessage& msg) {
    updateRoutingTable(msg);
    // invalidate the route to the node contained in the RERR
    assert(msg.unreachableDestAddr < MAX_NODES);
    routingTableEntry* entry = &routingTable[msg.unreachableDestAddr];
    entry->sequenceNumber = msg.unreachableDestSeq;
    entry->isValid = false;
    debugPrint("RERR received so invalidating entry for %d", msg.unreachableDestAddr);

}

void AODV::onReceive(PacketBuffer& buf) {
    ProtMessage messageFromRadio;
    messageFromRadio.unpack(buf);

    // check if packet for this node (could be broadcast)
    if (((messageFromRadio.destination != BROADCAST_ADDRESS) && (messageFromRadio.destination != myAddress)) ||
        ((messageFromRadio.destination == myAddress) && (messageFromRadio.source == myAddress))) {
        //debugPrint("Discarding pkt from %d", messageFromRadio.destination);
        return;
    }

    debugPrint("AODV: Message received %d from %d to %d", messageFromRadio.pktType, messageFromRadio.source, messageFromRadio.destination);

    // look at packet type and decide how to handle
    switch (messageFromRadio.pktType) {
    case MSG_APP_DATA:
        dataReceived++;
        handleData(messageFromRadio);
        break;

    case MSG_RREQ:
        rreqReceived++;
        //uBit.display.printAsync("Q");
        handleRREQ(messageFromRadio);
        break;

    case MSG_RREP:
        rrepReceived++;
        //uBit.display.printAsync("P");
        handleRREP(messageFromRadio);
        break;

    case MSG_HELLO:
        helloReceived++;
        //uBit.display.printAsync("H");
        handleRREP(messageFromRadio);
        break;

    case MSG_RERR:
        rerrReceived++;
        //uBit.display.printAsync("E");
        handleRERR(messageFromRadio);
        break;

    default:
        debugPrint("Unknown packet type %d - discarding", messageFromRadio.pktType);
    }
}


