/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/*This program simply has a timer that is between 0 and 5 seconds that 
will time out causing a broadcast ping message to be sent to local microBits,
when they receive the ping they will display a "P" on the LED screen and then
send a ping reply. When the reply is received it will increment the count
and display the count.
*/

#include "MicroBit.h"
#include "aodv.h"
#include "PingApp.h"
#include "ProtMessage.h"
#include <assert.h>

MicroBit    uBit; //This is a library that contains functions for microBit facilities
PingApp *myPingApp; // create a pingApp instance
AODV *myAODV;
int NUM_NODES=4;

//This function is the receiving function that will run when data is received.
void onData(MicroBitEvent)
{
    PacketBuffer buf(PACKET_BUFFER_SIZE);
    buf = uBit.radio.datagram.recv();
    myAODV->onReceive(buf);
}



int main()
{
    int myAddress = 999;
    int mode = 0;
    // Initialise the micro:bit runtime.
    uBit.init();

    uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    uBit.radio.enable();
    
    uBit.serial.send("Hello\n",SYNC_SPINWAIT);
    
    uint32_t serial = microbit_serial_number();
    if (serial == 0x8ac91171){
        myAddress=0;
        mode = 3;
    }
    else if (serial == 0x3d019818){
        myAddress = 1;
        mode = 3;
    }
    else if (serial == 0xfd919cec) {
        myAddress = 2;
        mode = 3;       
    }
    else if (serial == 0x8768e4fe) {
        myAddress = 3;
        mode = 3;       
    }
    
    debugPrint("I am serial number %x and address %d and mode %d", serial, myAddress, mode);
    // display our address
    uBit.display.print(myAddress);
    
    // map unique address to a node address
    myPingApp = new PingApp(myAddress,mode);
    myAODV = new AODV (myAddress, myPingApp);
    myPingApp->setRoutingLayer(myAODV);
    
    assert(myPingApp);
    assert(myAODV);
    
    //infinite loop for 1 second timer
    while(1) {
        // sleep for a second
        uBit.sleep(1000);
        myAODV->secondTimerExpired();
        myPingApp->secondTimerExpired();
        
    }
}

