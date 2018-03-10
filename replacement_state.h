#ifndef REPL_STATE_H
#define REPL_STATE_H
 
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This file is distributed as part of the Cache Replacement Championship     //
// workshop held in conjunction with ISCA'2010.                               //
//                                                                            //
//                                                                            //
// Everyone is granted permission to copy, modify, and/or re-distribute       //
// this software.                                                             //
//                                                                            //
// Please contact Aamer Jaleel <ajaleel@gmail.com> should you have any        //
// questions                                                                  //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <cassert>
#include "utils.h"
#include "crc_cache_defs.h"
#include <iostream>

using namespace std;

/* My Own Enhancement */
#define FINAL_ENHANCEMENT 1

/* SHiP Signature Defines */
#define SHIP_PC_BITS 18
#define MASK ((1<<SHIP_PC_BITS) - 1)

/* SHiP Counter Defines */
#define COUNTER_BITS 6
#define HIGHEST_COUNT_VALUE ((1<<COUNTER_BITS) - 1)

/* SRRIP Interval Defines */
#define M 3
#define IMMEDIATE_INTERVAL 0 
#define NEAR_IMMEDIATE_INTERVAL 1 
#define FAR_INTERVAL ((1<<M) - 3) 
#define DISTANT_INTERVAL ((1<<M) - 1) 


// Replacement Policies Supported
typedef enum 
{
    CRC_REPL_LRU        = 0,
    CRC_REPL_RANDOM     = 1,
    CRC_REPL_CONTESTANT = 2
} ReplacemntPolicy;

// Replacement State Per Cache Line
typedef struct
{
    UINT32  LRUstackposition;

    // CONTESTANTS: Add extra state per cache line here
    /* SHiP Defines */
    UINT32  SHiP_PC;
    bool outcome;
    /* Enhancement 5 from SHiP++ */
    bool is_pf;
    UINT32 interval;

} LINE_REPLACEMENT_STATE;


// The implementation for the cache replacement policy
class CACHE_REPLACEMENT_STATE
{
public:
    LINE_REPLACEMENT_STATE   **repl;
  private:

    UINT32 numsets;
    UINT32 assoc;
    UINT32 replPolicy;

    COUNTER mytimer;  // tracks # of references to the cache

    // CONTESTANTS:  Add extra state for cache here

  public:
    ostream & PrintStats(ostream &out);

    // The constructor CAN NOT be changed
    CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol );

    INT32 GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType );

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID );

    void   SetReplacementPolicy( UINT32 _pol ) { replPolicy = _pol; } 
    void   IncrementTimer() { mytimer++; } 

    void   UpdateReplacementState( UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
                                   UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit );

    ~CACHE_REPLACEMENT_STATE(void);

  private:
    
    void   InitReplacementState();
    INT32  Get_Random_Victim( UINT32 setIndex );

    INT32  Get_LRU_Victim( UINT32 setIndex );
    INT32  Get_My_Victim( UINT32 setIndex );
    void   UpdateLRU( UINT32 setIndex, INT32 updateWayID );
    void   UpdateMyPolicy(Addr_t PC, UINT32 setIndex, INT32 updateWayID, bool cacheHit, UINT32 accessType);
    /* SHiP Defines */
    unsigned long int SHCT[1<<SHIP_PC_BITS];
	  bool victim_found;
    UINT32 victim_number;
};

#endif
