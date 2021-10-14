
#include "ProtMessage.h"
//#include <string>


// constructor
ProtMessage::ProtMessage() {}

//destructor
ProtMessage::~ProtMessage() {}

void pack32bitWord(uint8_t* ptr, int i) {
    ptr[0] = i & 0xff;
    ptr[1] = (i >> 8) & 0xff;
    ptr[2] = (i >> 16) & 0xff;
    ptr[3] = (i >> 24) & 0xff;
}

int unpack32bitWord(uint8_t* ptr) {
    return (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
}


bool ProtMessage::pack(PacketBuffer& buf) {
    buf[0] = pktType;
    buf[1] = source;
    buf[2] = destination;
    buf[3] = originator;
    buf[4] = finalDest;
    pack32bitWord(&buf[5], finalDestSeq); // 5,6,7,8, position
    switch (pktType)  {
    case MSG_RREQ:
        buf[9] = hopCount;
        pack32bitWord(&buf[10], origSeq); // 10, 11, 12, 13
        pack32bitWord(&buf[14], RREQid); // 14, 15, 16, 17
        break;
    case MSG_RREP:
    case MSG_HELLO:
        buf[9] = hopCount;
        buf[10] = lifetime;
        pack32bitWord(&buf[11], origSeq); // 11, 12, 13, 14
        break;
    case MSG_RERR:
        buf[9] = unreachableDestAddr;
        pack32bitWord(&buf[10], unreachableDestSeq); // 10, 11, 12, 13
        break;
    case MSG_APP_DATA:
        // pack in the bytes into payload
        for (int i = 0; i < PAYLOAD_SIZE; i++) buf[9 + i] = payload[i]; // 9 onwards
        break;
    default:
        break;
    }

    return(true);
    }

bool ProtMessage::unpack(PacketBuffer& buf) {
    pktType = buf[0];
    source = buf[1];
    destination = buf[2];
    originator = buf[3];
    finalDest = buf[4];
    finalDestSeq = unpack32bitWord(&buf[5]);
    switch (pktType) {
    case MSG_RREQ:
        hopCount = buf[9];
        origSeq = unpack32bitWord(&buf[10]); // 10, 11, 12, 13
        RREQid = unpack32bitWord(&buf[14]); // 14, 15, 16, 17
        break;
    case MSG_RREP:
    case MSG_HELLO:
        hopCount = buf[9];
        lifetime = buf[10];
        origSeq = unpack32bitWord(&buf[11]); // 11, 12, 13, 14
        break;
    case MSG_RERR:
        unreachableDestAddr = buf[9];
        unreachableDestSeq = unpack32bitWord(&buf[10]);
        break;
    case MSG_APP_DATA:
        // unpack the bytes into payload
        for (int i = 0; i < PAYLOAD_SIZE; i++) payload[i] = buf[9 + i] ; // 9 onwards
        break;
    default:
        break;
    }
    return(true);
}

// return a nice string of the message in a nice format
ManagedString ProtMessage::convert_to_str() {
    ManagedString idString("");
    ManagedString sourceString(source);
    ManagedString destString(destination);
    ManagedString origString(originator);
    ManagedString finalDestString(finalDest);
    ManagedString output("");

    switch (pktType) {
    case MSG_APP_DATA:
        output =  "APP_DATA SRC=" + sourceString + " DST=" + destString +
            " Orig=" + origString + " FinalDest=" + finalDestString + 
            " Payload=";
        for (int i = 0; i < PAYLOAD_SIZE; i++) output = output + ManagedString(payload[i]) + " ";
        break;
    case MSG_HELLO:
        output = "HELLO SRC=" + sourceString + " DST=" + destString +
            " Orig=" + origString + " FinalDest=" + finalDestString +
            " HopCount=" + ManagedString(hopCount) +
            " Lifetime=" + ManagedString(lifetime);
        break;
    case MSG_RREQ:
        output = "RREQ SRC=" + sourceString + " DST=" + destString +
            " Orig=" + origString + " FinalDest=" + finalDestString +
            " RREQID=" + ManagedString(RREQid) + " HopCount=" + ManagedString(hopCount) +
            " OrigSeq=" + ManagedString(origSeq) +
            " DestSeq=" + ManagedString(finalDestSeq);

        break;
    case MSG_RREP:
        output = "RREP SRC=" + sourceString + " DST=" + destString +
            " Orig=" + origString + " FinalDest=" + finalDestString +
            " HopCount=" + ManagedString(hopCount) +
            " OrigSeq=" + ManagedString(origSeq) + 
            " Lifetime=" + ManagedString(lifetime);

        break;
    case MSG_RERR:
        output = "RERR SRC=" + sourceString + " DST=" + destString +
            " Orig=" + origString + " FinalDest=" + finalDestString +
            " UnreachDestAddr=" + ManagedString(unreachableDestAddr) +
            " UnreachDestSeq=" + ManagedString(unreachableDestSeq);
        break;
    default:
        break;
    }
    return (output);
}


