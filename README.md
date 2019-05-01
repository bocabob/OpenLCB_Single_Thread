# OpenLCB_Single_Thread
OpenLCB stock based on ArduinoIDE

# **** USE with CAUTION - in dev mode ****

This is a refresh of the original Arduino code base developed by Dr. Bob Jacobsen.  It has been modified to make it easier for the developer to match a project's CDI xml to its internal memory structure, by making the two parallel in structure.  In addition, eventid searching uses a sorted table and a binary search. 

# Platforms supported:
* ATMega series, with MCP2515 CAN support; 
* AT90CAN series, with native CAN support; 
* TI Tiva series, with native CAN support; 
* Teensy series, with native CAN support; 
* ESP32, with native CAN supprt.  

Using a specific platform requires downloading of the appropriate compiler support.  
A platform is automagically selected in the processor.h file, allowing the same sketch to be used on multiple platforms.  Platform specific items are included in the processor.h file.  

### At this point, the example sketch OlcbBasicNode compiles in the supported platforms.  

## Changes: 
1. Added support for multiple processors: Arduino, Teensy, Tiva. 
   - Each set of files specific to a CAN-processor is kept in its own directory.   
   - The processor is automatically selected in the processor.h file. 
2. A sorted Index[] is used to speed eventID processing, using a hash of the eventid.  
3. Simplified the definition of CDI/xml for the node by matching a struct{} to the xml structure, see the example below.   

e.g.: This CDI/xml, which self-describes the node to the system:
```xml
    <cdi>
        ...
        <group replication='8'>
        <name>Channels</name>
            <eventid><name>event0</name></eventid>
            <eventid><name>event1</name></eventid>
        </group>
        ...
    </cdi>
```
parallels this program structure:    
```c++
    typedef struct {
        ...
        struct {
            EventID event0;
            EventID event1;
        } channels[8];
        ...
    } MemStruct;
```

In addition, EIDtab is constructed with offsets to each eventid in EEPROM, and their type: Producer, consumer, or both.  
```c++
  // ===== eventid Table =====
  //  Array of the offsets to every eventID in MemStruct/EEPROM/mem, and P/C flags
  //    -- each eventid needs to be identified as a consumer, a producer or both.  
  //    -- PEID = Producer-EID, CEID = Consumer, and PC = Producer/Consumer
  //    -- note matching references to MemStruct.  
       const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        PEID(inputs[0].activation), PEID(inputs[0].inactivation),  // 1st channel - input, ie producer
        PEID(inputs[1].activation), PEID(inputs[1].inactivation),  // 2nd channel - input
        CEID(outputs[0].setEvent),  CEID(outputs[0].resetEvent),   // 3rd channel - output, ie consumer
        CEID(outputs[1].setEvent),  CEID(outputs[1].resetEvent),   // 4th channel - output
      };
```

## Memory Models:
Eventids are read from eeprom into event[].  Their location in EEPROM is held in EIDtab[], see above.  

The eventids are then sorted into eventIndex[], while event[] entries remain in memory order.  Matching a received eventid is by binary search on eventIndex[]/event[].  

     eventIndex[]-+-->event[eid]
                  +-->[EIDtab:offset,flags]-->EEPROM
 

#### In Flash:<br>
EEPROM, or equivalent, is laid out in accordance with Memstruct, which matches the CDI xml, see above.  
    
#### In RAM:
events[] holds a copy of the node's eventids initialized from EEPROM.  
eventIndex[] holds indexes to event[] in ascending sorted order.  
EIDtab[] holds the offsets to the eventids in EEPROM, this is built using Memstruct to calculate the eventid-offsets.  

## More about OpenLCB/LCC - what is it?

OpenLCB/LCC is a set of heirarchical protocols to **let nodes talk to each other**. <br>
For more information on OpenLCB, see: [OpenLCB.org](http://openlcb.org)<br>
For protocol descriptions, see: [Adopted LCC Documents](http://openlcb.org/openlcb-and-lcc-documents/layout-command-control-lcc/)

These protocols consist of: 
 - **Systems/Application Messaging**<br>
    These are the 'workhorse' messages on which most applications will be built, they are useful for sytems-messaging as well, and for building further systems-protocols.  
   - PCE - Produver/Consumer Event Messages
     - These are *unaddressed* EventID messages.
     - They implement *Producer/Consumer Events* (64-bit)
     - EventIDs are globally unique 64-bit numbers.
     - These are one-to-many messages.
   - Datagram Messages
     - These are *addressed* messages containing up to 70-bytes of data in a single message.
     - These are one-to-one messages.
   - Stream Messages
     - These are *addressed* messages carrying unlimited data in multiple messages.
     - These are one-to-one messages.
 - **Systems/Housekeeping**<br>
    These are the 'behind-the-scenes' protocol that enables and ensures the system's integrity. 
   - Link - establishes and maintains the node's link to the network
     - Announces state of Node
     - Announcement of *Intialization Complete*
     - Announcement of *Consumed-* and *Produced-eventIDs*
     - *NodeID reporting* on request.
     - *EventID reporting* on request.
     - On the CAN-implementation, this maintains *alias assignment and maintenance*;
   - SNIP - Simple Node Information Protocol
     - Provides a brief description of the node for *UI Tools* to use.
   - PIP - Protocol Identification Protocol
     - Defines which protocols the node uses, and it is reported as a bit-map.  
   - CDI - Configuration Description Information
     - *Reports the node's CDI/xml* on request.
   - Memory Configuration
     - Reading and writing to the node's memory spaces, including Configuration, RAM and EEPROM spaces.
 - **Additional Protocols**<br>
    These protocols extend the base-system.
   - Teaching -- teaching an eventID from one node to one or more others.  
   - Traction Control -- train control.
 - **Additional Utility-Libraries**<br>
    These libraries implement useful functionality.  
   - BG - Blue/Green -- node health indicators and system buttons.
   - ButtonLed -- implements controlling a button and LED from a single processor pin.

## How the Above Translates to the Codebase
The 'codebase' is a set of libraries and functions that implement the basic protocols of OpenLCB/LCC. <br> 
Each protocol has corresponding **code**, usually in the form of a **class**, and implemented as a pair of *.h and *.cpp  files. <br> 
The codebase tries to hide some of the complexity in #include files.  <br>

However, each protocol needs to have: 
 - **initialization**, and
 - **processing**
    
For example there are some selected lines of code from the OlcbBasicNode example used for **initialization**: 
```c++
  NodeID nodeid(5,1,1,1,3,255);    // This node's default ID; must be valid 
  const char SNII_const_data[] PROGMEM = "\001OpenLCB\000DPHOlcbBasicNode\0001.0\000" OlcbCommonVersion ; 
  uint8_t protocolIdentValue[6] = {0xD7,0x58,0x00,0,0,0};
  ButtonLed* buttons[] = { &pA,&pA,&pB,&pB,&pC,&pC,&pD,&pD };
  PCE pce(&nodal, &txBuffer, pceCallback, restore, &link);
  BG bg(&pce, buttons, patterns, NUM_EVENT, &blue, &gold, &txBuffer);
```
Most of the **processing** is hidden as functions in the #include files.  

## How Does the Application Interact with the Codebase?
The programmer of the Application must: 
 - Decide what and how the new Application works, ie how eventids and other node variables are used to build the Application.  
 - Choose the **NodeID** - this must be from **a range controlled** by the manufacturer - **ie you**.  
 - Write the **CDI/xml** describing the node and its node-variables, including its eventIDs. 
 - Write a **MemStruct{}** that matches the xml description.  
 - Write code to flag each eventID as a **Producer, Consumer, or Both** into EIDtab[].  
 - Write **pceCallback()** to process received eventIDs passed to it, ie eventIDs which are to be consumed, and causing whatever action is required, eg a LED being lit or extinguished.  
 - Write **produceFromInputs()** which scans the node's inputs and, if appropriate, flags an evenItD to be sent.  
 - Write **userConfigWrite()** which is called whenever a UI Tool writes to the node's memory.  This code can then compare the memory address range of the change to the node's variables, and take whatever action is appropriate, e.g. update a servo position.
 - Write additional support and glue code for the Application.  

## Example Applications
The provided examples will give some ideas of how to accomplish sample projects.  They can form the basis of, or  be adapted to, a new Application, or just used for inspiration.  
 - **OlcbBasicNode**<br>
    Implements a simple node which exercises most of the protocols.  It has **two inputs** and **two outputs**.  Each input has two Producer-eventIDs and each output has two Consumer-eventIDs, so **8 eventIDs in total**.  This Application makes use of the ButtonLed library to control **two buttons** and **two LEDs**.  In addition, it implements the BG (Blue-Gold) protocol to allow the **teaching** of eventIDs between this node and others.  
    
## In prgress, but not uploaded
- **OlcbServoPCA8695**<br>
    Implements driving a number of servos from a PCA8695 PWM chip.  It shows how to write a different **pceCallback()**.  It also uses **userConfigWrite()** to allow real-time updating of a servo positions from a **UI Tool**, such as **JMRI** or **Model Railroad System**.  


