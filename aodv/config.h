#pragma once

#include "MicroBit.h"


#ifdef PC_SIM

#include <queue>
#include <mutex>
#include <condition_variable>
#include "PacketBuff.h"
#include "debug.h"

// store useful config and globals


extern std::queue<PacketBuffer*> radio_queue;
extern std::mutex radio_mutex;
extern std::condition_variable radio_cv;
extern MicroBit uBit;
extern int NUM_NODES;


#define MAX_NODES 8 // max number of nodes allowed in this implementation

# define POINT_TYPE(BASE_TYPE) std::shared_ptr<BASE_TYPE>

#else
// Microbit based config
extern MicroBit uBit;
extern int NUM_NODES;
#define POINTER_TYPE(BASE_TYPE) BASE_TYPE *
#define MAX_NODES 8 // max number of nodes allowed in this implementation
#define PACKET_BUFFER_SIZE 32

#define debugPrint(...) {\
    uBit.serial.printf("[NODE %d]", myAddress); \
    uBit.serial.printf(__VA_ARGS__);\
    uBit.serial.printf("\n"); \
    }


#define debugPrintStatus(...) {\
    uBit.serial.printf("[TABL %d]", myAddress); \
    uBit.serial.send(__VA_ARGS__);\
    uBit.serial.printf("\n"); \
    }

#endif
