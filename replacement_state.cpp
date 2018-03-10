#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <map>
#include <iostream>

using namespace std;

#include "replacement_state.h"

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

/*
** This file implements the cache replacement state. Users can enhance the code
** below to develop their cache replacement ideas.
**
*/


////////////////////////////////////////////////////////////////////////////////
// The replacement state constructor:                                         //
// Inputs: number of sets, associativity, and replacement policy to use       //
// Outputs: None                                                              //
//                                                                            //
// DO NOT CHANGE THE CONSTRUCTOR PROTOTYPE                                    //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
CACHE_REPLACEMENT_STATE::CACHE_REPLACEMENT_STATE( UINT32 _sets, UINT32 _assoc, UINT32 _pol )
{

    numsets    = _sets;
    assoc      = _assoc;
    replPolicy = _pol;

    mytimer    = 0;

    InitReplacementState();
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// The function prints the statistics for the cache                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
ostream & CACHE_REPLACEMENT_STATE::PrintStats(ostream &out)
{

    out<<"=========================================================="<<endl;
    out<<"=========== Replacement Policy Statistics ================"<<endl;
    out<<"=========================================================="<<endl;

    // CONTESTANTS:  Insert your statistics printing here
    
    return out;

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function initializes the replacement policy hardware by creating      //
// storage for the replacement state on a per-line/per-cache basis.           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void CACHE_REPLACEMENT_STATE::InitReplacementState()
{
    // Create the state for sets, then create the state for the ways

    repl  = new LINE_REPLACEMENT_STATE* [ numsets ];

    // ensure that we were able to create replacement state

    assert(repl);

    // Create the state for the sets
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        repl[ setIndex ]  = new LINE_REPLACEMENT_STATE[ assoc ];

        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].LRUstackposition = way;
        }
    }

    if (replPolicy != CRC_REPL_CONTESTANT) return;

    /* Saturating confidence counter */
    memset (SHCT, 0, sizeof (SHCT));

    // Contestants:  ADD INITIALIZATION FOR YOUR HARDWARE HERE
    /* Initialize signatures to all zeroes */
    /* Initialize the counters to all zeros */
    for(UINT32 setIndex=0; setIndex<numsets; setIndex++) 
    {
        for(UINT32 way=0; way<assoc; way++) 
        {
            // initialize stack position (for true LRU)
            repl[ setIndex ][ way ].outcome= false; repl[ setIndex ][ way ].is_pf = false; repl[ setIndex ][ way ].interval = FAR_INTERVAL; repl[ setIndex ][ way ].SHiP_PC = 0x0;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache on every cache miss. The input        //
// argument is the set index. The return value is the physical way            //
// index for the line being replaced.                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::GetVictimInSet( UINT32 tid, UINT32 setIndex, const LINE_STATE *vicSet, UINT32 assoc, Addr_t PC, Addr_t paddr, UINT32 accessType ) {
    // If no invalid lines, then replace based on replacement policy
    if( replPolicy == CRC_REPL_LRU ) 
    {
        return Get_LRU_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        return Get_Random_Victim( setIndex );
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR VICTIM SELECTION FUNCTION HERE
        /* My victim selection based on the SHiP and SRRIP algorithm */
        return Get_My_Victim (setIndex);
    }

    // We should never here here
    assert(0);
    return -1;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function is called by the cache after every cache hit/miss            //
// The arguments are: the set index, the physical way of the cache,           //
// the pointer to the physical line (should contestants need access           //
// to information of the line filled or hit upon), the thread id              //
// of the request, the PC of the request, the accesstype, and finall          //
// whether the line was a cachehit or not (cacheHit=true implies hit)         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
void CACHE_REPLACEMENT_STATE::UpdateReplacementState( 
    UINT32 setIndex, INT32 updateWayID, const LINE_STATE *currLine, 
    UINT32 tid, Addr_t PC, UINT32 accessType, bool cacheHit )
{
	//fprintf (stderr, "ain't I a stinker? %lld\n", get_cycle_count ());
	//fflush (stderr);
    // What replacement policy?
    if( replPolicy == CRC_REPL_LRU ) 
    {
        UpdateLRU( setIndex, updateWayID );
    }
    else if( replPolicy == CRC_REPL_RANDOM )
    {
        // Random replacement requires no replacement state update
    }
    else if( replPolicy == CRC_REPL_CONTESTANT )
    {
        // Contestants:  ADD YOUR UPDATE REPLACEMENT STATE FUNCTION HERE
        // Feel free to use any of the input parameters to make
        // updates to your replacement policy
        /* Calling my SHiP++ update policy */
        UpdateMyPolicy(PC, setIndex, updateWayID, cacheHit, accessType);
        /* My own enhancement to insert the non FAR or more RRPV in the MRU posn */
        if(repl[setIndex][updateWayID].interval < FAR_INTERVAL)
          UpdateLRU( setIndex, updateWayID );
    }
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//////// HELPER FUNCTIONS FOR REPLACEMENT UPDATE AND VICTIM SELECTION //////////
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds the LRU victim in the cache set by returning the       //
// cache block at the bottom of the LRU stack. Top of LRU stack is '0'        //
// while bottom of LRU stack is 'assoc-1'                                     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_LRU_Victim( UINT32 setIndex )
{
	// Get pointer to replacement state of current set

	LINE_REPLACEMENT_STATE *replSet = repl[ setIndex ];
	INT32   lruWay   = 0;

	// Search for victim whose stack position is assoc-1

	for(UINT32 way=0; way<assoc; way++) {
		if (replSet[way].LRUstackposition == (assoc-1)) {
			lruWay = way;
			break;
		}
	}

	// return lru way
	return lruWay;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function finds a random victim in the cache set                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
INT32 CACHE_REPLACEMENT_STATE::Get_Random_Victim( UINT32 setIndex )
{
    INT32 way = (rand() % assoc);
    
    return way;
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
// This function implements the LRU update routine for the traditional        //
// LRU replacement policy. The arguments to the function are the physical     //
// way and set index.                                                         //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

void CACHE_REPLACEMENT_STATE::UpdateLRU( UINT32 setIndex, INT32 updateWayID )
{
	// Determine current LRU stack position
	UINT32 currLRUstackposition = repl[ setIndex ][ updateWayID ].LRUstackposition;

	// Update the stack position of all lines before the current line
	// Update implies incremeting their stack positions by one

	for(UINT32 way=0; way<assoc; way++) {
		if( repl[setIndex][way].LRUstackposition < currLRUstackposition ) {
			repl[setIndex][way].LRUstackposition++;
		}
	}
	// Set the LRU stack position of new line to be zero
	repl[ setIndex ][ updateWayID ].LRUstackposition = 0;
}

INT32 CACHE_REPLACEMENT_STATE::Get_My_Victim( UINT32 setIndex ) {
  /* Initalizing the values */
	  victim_number = assoc - 1;
	  victim_found = false;

    /* My own enhancement to pick the DIST INTERVAL values from the LRU * position */
#ifdef FINAL_ENHANCEMENT
    UINT32 stack_position = 13;
#endif
       
  /* Static Re-reference Interval prediction algo for victim selection */
  do
  {
    for(UINT32 i=0; i<assoc; i++)
    {
#ifdef FINAL_ENHANCEMENT
    	if(repl[setIndex][i].interval == DISTANT_INTERVAL) 
      {
        if(repl[setIndex][i].LRUstackposition >= stack_position)
        {
          stack_position = repl[setIndex][i].LRUstackposition;
          victim_found = true;
          victim_number = i;
        }
    	}
#else
      //interval == RRPV as per the Jaleel et al. paper.
    	if(repl[setIndex][i].interval == DISTANT_INTERVAL) 
      {
        victim_found = true;
        victim_number = i;
        return victim_number;
        break;
      //DEBUG    printf ("sachin inside victim catcher val is %d %d, ctr val = %d\n", victim_number, assoc, counter_value); 
    	}
#endif
    }

    if(victim_found == false)
    {
      /* SRRIP algo update */
      for(UINT32 i=0; i<assoc; i++)
      {
          (repl[setIndex][i].interval<DISTANT_INTERVAL)?(repl[setIndex][i].interval++):repl[setIndex][i].interval;
      //   ((repl[setIndex][i].outcome ==  0) && (repl[setIndex][i].interval ==  FAR_INTERVAL))?(repl[setIndex][i].interval++):repl[setIndex][i].interval;
          ((repl[setIndex][i].outcome ==  0) && (repl[setIndex][i].interval ==  (FAR_INTERVAL - 1)))?((repl[setIndex][i].interval += 2)):repl[setIndex][i].interval;
      }
    }
  }while(!victim_found);
   
 //DEBUG//printf ("sachin victim is %d %d\n", victim_number, assoc); 
	return victim_number;
}

void CACHE_REPLACEMENT_STATE:: UpdateMyPolicy(Addr_t PC, UINT32 setIndex, INT32 updateWayID, bool cacheHit, UINT32 accessType)
{
  /* Here we will be using the SHiP algorithm, with some enhancements mentioned in the ShiP++ */ 
  /* Enhancement 4 from SHiP++, to include different signatures with different accesstypes */
    UINT32 signature =  MASK & ((PC << 5) + ((accessType == ACCESS_PREFETCH) << 4) + ((accessType == ACCESS_WRITEBACK) << 3) + ((accessType == ACCESS_IFETCH) << 2) + ((accessType == ACCESS_LOAD) << 1)  + ((accessType == ACCESS_STORE) << 0)); 

  /* Enhancement 2 from SHiP++, to not update the Reref distances on the first accesses of the prefetches. As these prefetches may be just accessed once and never used again. */
  if(cacheHit && (repl[setIndex][updateWayID].outcome == false))
     {
       /* Enhancement 5 from ShiP++ paper */
       if(repl[setIndex][updateWayID].is_pf)
       {
        repl[setIndex][updateWayID].is_pf = false;
       }
       else
       {
       (SHCT[repl[setIndex][updateWayID].SHiP_PC] < HIGHEST_COUNT_VALUE)?(SHCT[repl[setIndex][updateWayID].SHiP_PC]++):(SHCT[repl[setIndex][updateWayID].SHiP_PC]);
       }
       /* SHiP */
       repl[setIndex][updateWayID].outcome = true;
  }
  else
  {
    //if((repl[setIndex][victim_number].outcome == false) && victim_found)
    if((repl[setIndex][updateWayID].outcome == false))
    {
    (SHCT[repl[setIndex][updateWayID].SHiP_PC] > 0)?(SHCT[repl[setIndex][updateWayID].SHiP_PC]--):(SHCT[repl[setIndex][updateWayID].SHiP_PC]);
    }

    /* Updating the signatures */
    repl[setIndex][updateWayID].SHiP_PC = signature;
    /* Updating the outcomes */
    repl[setIndex][updateWayID].outcome = false;

    /* SHiP++ enhancement to set the prefetch bit if the access is PREFETCH */
    repl[setIndex][updateWayID].is_pf = (accessType == ACCESS_PREFETCH);
    
    /* This is from the SHiP++ paper by Young et al. */
    /* Enhancement 3, all the writebacks will be upated as DISTANT INTERVAL by default */
    if((accessType == ACCESS_WRITEBACK) || (SHCT[signature] == 0))
    {
      repl[setIndex][updateWayID].interval = DISTANT_INTERVAL;
    }
    /* This is from the SHiP++ paper by Young et al. */
    /* Enhancement 1, if the signature contains the HIGHEST_COUNT_VALUE, install it as the IMMEDIATE_INTERVAL */
    else if(SHCT[signature] == HIGHEST_COUNT_VALUE)
    {
      repl[setIndex][updateWayID].interval = IMMEDIATE_INTERVAL;
    }
    /* This is from SHiP, rest of the values will be at FAR Interval */
    else
    {
      repl[setIndex][updateWayID].interval = FAR_INTERVAL;
    }

  }

  //Tried to update the intervals only on second access, Performs worse   ((repl[setIndex][updateWayID].interval > 0) && (((SHCT[repl[setIndex][updateWayID].SHiP_PC]) % 2) == 0))? (repl[setIndex][updateWayID].interval--):repl[setIndex][updateWayID].interval;
  /* Frequent update algorithm as in the SRRIP paper */
  if(cacheHit)
  {
    /* Do this only when the access is not of the Aggressive Prefetch type */
    if(repl[setIndex][updateWayID].is_pf == false)
    {
      /* SRRIP update: Improved performance hence keeping it */
    (repl[setIndex][updateWayID].interval > 0) ? (repl[setIndex][updateWayID].interval--):repl[setIndex][updateWayID].interval;
    }
  }
}

CACHE_REPLACEMENT_STATE::~CACHE_REPLACEMENT_STATE (void) {
}
