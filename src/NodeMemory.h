#ifndef NodeMemory_h
#define NodeMemory_h

/**
 * Class for persisting node memory
 * in a non-volative memory, e.g. EEPROM.
 *
 * This class loads from and stores to EEPROM
 * a NodeID and two arrays of Events; it's up to you 
 * to store everything else.
 * 
 * The memory can either be 
 * blank, needing complete initialization or
 * OK at startup. Call setup(...) with a valid NodeID
 * to either load or create EventIDs.
 * 
 * If you change those, call store(...) when done.
 * 
 * When you "reset" the memory, you're 
 * putting _new_ unique EventIDs in place.
 *
 * The first four bytes of memory contain flag values:
 * 0xEE 0x55 0x5E 0xE5 - all memory valid, use as is
 * 0xEE 0x55 0x33 0xCC - Node ID valid, rest must be initialized
 * Any other flag means no memory valid.
 *
 * TODO: Add a "dirty" bit to make store logic easier for external code?
 *
 */
#include <stdint.h>
#include "NodeID.h"
#include "processor.h"

#define RESET_NORMAL_VAL						0xEE55
#define RESET_NEW_EVENTS_VAL				0x33CC
#define RESET_FACTORY_DEFAULTS_VAL	0xFFFF

#define RESET_NORMAL						0x01
#define RESET_NEW_EVENTS				0x02
#define RESET_FACTORY_DEFAULTS	0x04

#define NODE_ID_MARKER_VALID		0x10
#define NODE_ID_CHECKSUM_VALID	0x20
#define NODE_ID_OK (NODE_ID_MARKER_VALID | NODE_ID_CHECKSUM_VALID)

#define NODE_ID_MARKER					'N'

typedef struct
{
	uint8_t 	nodeIdMarker;
	NodeID 		nodeId;
	uint8_t 	nodeIdCheckSum;
} NODE_ID_STORE;

typedef struct
{
	NODE_ID_STORE 	nodeId;
	uint16_t				nextEID;
} NODE_HEADER;

class NodeID;
class Event;
class EventID;

class NodeMemory {
  public:

  /**
   * Define starting address in EEPROM
   */
  NodeMemory(int eepromBaseAddress, uint16_t userConfigSize);  // doesn't do anything
    
  /**
   * Make sure ready to go.  NodeID should have a default
   * value already in case this is the first time.
   *
   * events is address in RAM to load from EEPROM
   * copy N=extraBytes of memory from end of EEPROM to RAM.
   *
   * If the EEPROM is corrupt, all that is reloaded after  
   * N=clearBytes of memory is cleared to e.g. 
   * clear name strings. This count EEPROM address 0 (e.g. _not_ starting
   * at end of event strings; this is the full memory clear)   
   */
  void init(Event* events, uint8_t numEvents);
    
  /** 
   * For debug and test, this forces a reset to factory defaults at next restart
   */
  void forceFactoryReset();
    
  /** 
   * Reload a complete set of new events on next restart.
   */
  void forceNewEventIDs();
    
  uint8_t getNodeID(NodeID *nodeIdBuffer);

  void changeNodeID(NodeID *newNodeId);
  
  // EEPROM.h Compatibility functions that address EEPROM above the Offset specified in userConfigBase
  // that will be called by the cloned EEPROM.h classed defined in OpenLCBMid.h
  uint8_t read( int idx );
	void write( int idx, uint8_t val );
  /*
   * Get a new, forever unique EventID and put in 
   * given EventID location. Does not do a EEPROM store,
   * which must be done separately. unique ID build using
   * this node's nodeID. 
   */
  //void setToNewEventID(NodeID* nodeID, EventID* eventID);
  void setToNewEventID(NodeID* nid, uint16_t eOff);
  void setToNewEventID(NodeID* nid, int n);  //dph

	void print();
    
  private:
		uint8_t			loadAndValidate();	
	  void 				writeNewEventIDs(Event* events, uint8_t numEvents);
		
		uint16_t		eepromBase; 		// address of 1st byte in EEPROM we can use
		uint16_t		userConfigBase; // address of 1st byte in EEPROM for User Config Data
		uint16_t 		userConfigSize; 			// size of the User Config Data
		uint8_t 		nodeHeaderState;
		NODE_HEADER	header;
};

extern NodeMemory nm;

/*
	This code is lifted directly from the EEPROM.h file and adapted to use the NodeMemory backend
	I've simply substituted EE for NMC (NodeMemoryConfig)
	
	============================================================================================= 
  EEPROM.h - EEPROM library
  Original Copyright (c) 2006 David A. Mellis.  All right reserved.
  New version by Christopher Andrews 2015.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/***
    NMCRef class.
    
    This object references an EEPROM cell within the NodeMemory class as it uses different offsets.
    Its purpose is to mimic a typical byte of RAM, however its storage is the EEPROM.
    This class has an overhead of two bytes, similar to storing a pointer to an EEPROM cell.
***/

struct NMCRef{

    NMCRef( const int index )
        : index( index )                 {}
    
    //Access/read members.
    uint8_t operator*() const            { return nm.read( index ); }
    operator uint8_t() const             { return **this; }
    
    //Assignment/write members.
    NMCRef &operator=( const NMCRef &ref ) { return *this = *ref; }
    NMCRef &operator=( uint8_t in )       { return nm.write( index, in ), *this;  }
    NMCRef &operator +=( uint8_t in )     { return *this = **this + in; }
    NMCRef &operator -=( uint8_t in )     { return *this = **this - in; }
    NMCRef &operator *=( uint8_t in )     { return *this = **this * in; }
    NMCRef &operator /=( uint8_t in )     { return *this = **this / in; }
    NMCRef &operator ^=( uint8_t in )     { return *this = **this ^ in; }
    NMCRef &operator %=( uint8_t in )     { return *this = **this % in; }
    NMCRef &operator &=( uint8_t in )     { return *this = **this & in; }
    NMCRef &operator |=( uint8_t in )     { return *this = **this | in; }
    NMCRef &operator <<=( uint8_t in )    { return *this = **this << in; }
    NMCRef &operator >>=( uint8_t in )    { return *this = **this >> in; }
    
    NMCRef &update( uint8_t in )          { return  in != *this ? *this = in : *this; }
    
    /** Prefix increment/decrement **/
    NMCRef& operator++()                  { return *this += 1; }
    NMCRef& operator--()                  { return *this -= 1; }
    
    /** Postfix increment/decrement **/
    uint8_t operator++ (int){ 
        uint8_t ret = **this;
        return ++(*this), ret;
    }

    uint8_t operator-- (int){ 
        uint8_t ret = **this;
        return --(*this), ret;
    }
    
    int index; //Index of current EEPROM cell.
};

/***
    NMCPtr class.
    
    This object is a bidirectional pointer to EEPROM cells represented by NMCRef objects.
    Just like a normal pointer type, this can be dereferenced and repositioned using 
    increment/decrement operators.
***/

struct NMCPtr{

    NMCPtr( const int index )
        : index( index )                {}
        
    operator int() const                { return index; }
    NMCPtr &operator=( int in )          { return index = in, *this; }
    
    //Iterator functionality.
    bool operator!=( const NMCPtr &ptr ) { return index != ptr.index; }
    NMCRef operator*()                   { return index; }
    
    /** Prefix & Postfix increment/decrement **/
    NMCPtr& operator++()                 { return ++index, *this; }
    NMCPtr& operator--()                 { return --index, *this; }
    NMCPtr operator++ (int)              { return index++; }
    NMCPtr operator-- (int)              { return index--; }

    int index; //Index of current EEPROM cell.
};

/***
    EEPROMClass class.
    
    This object represents the entire EEPROM space.
    It wraps the functionality of NMCPtr and NMCRef into a basic interface.
    This class is also 100% backwards compatible with earlier Arduino core releases.
***/

struct NMCPROMClass{

    //Basic user access methods.
    NMCRef operator[]( const int idx )    { return idx; }
    uint8_t read( int idx )              { return NMCRef( idx ); }
    void write( int idx, uint8_t val )   { (NMCRef( idx )) = val; }
    void update( int idx, uint8_t val )  { NMCRef( idx ).update( val ); }

    uint16_t read16( int idx ){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			return (read(idx) << 8) | read(idx + 1);
#else
			return (read(idx + 1) << 8) | read(idx);
#endif
		}
		
    void write16( int idx, uint16_t val ){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			write(idx + 1, val & 0x00FF );
			write(idx, (val >> 8) & 0x00FF );
#else
			write(idx, val & 0x00FF );
			write(idx + 1, (val >> 8) & 0x00FF );
#endif
    
		}
    void update16( int idx, uint16_t val ){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			update(idx + 1, val & 0x00FF );
			update(idx, (val >> 8) & 0x00FF );
#else
			update(idx, val & 0x00FF );
			update(idx + 1, (val >> 8) & 0x00FF );
#endif
		}

    //STL and C++11 iteration capability.
    NMCPtr begin()                        { return 0x00; }
    NMCPtr end()                          { return length(); } //Standards requires this to be the item after the last valid entry. The returned pointer is invalid.
    uint16_t length()                    { return E2END + 1 - sizeof(NODE_HEADER); }
    
    //Functionality to 'get' and 'put' objects to and from EEPROM.
    template< typename T > T &get( int idx, T &t ){
        NMCPtr e = idx;
        uint8_t *ptr = (uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  *ptr++ = *e;
        return t;
    }
    
    template< typename T > const T &put( int idx, const T &t ){
        NMCPtr e = idx;
        const uint8_t *ptr = (const uint8_t*) &t;
        for( int count = sizeof(T) ; count ; --count, ++e )  (*e).update( *ptr++ );
        return t;
    }
};

static NMCPROMClass NODECONFIG;

#endif
