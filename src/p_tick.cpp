/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "Precompiled.hpp"
#include "globaldata.hpp"

#include "z_zone.hpp"
#include "p_local.hpp"

#include "doomstat.hpp"



//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



// Both the head and tail of the thinker list.


//
// P_InitThinkers
//
void P_InitThinkers (void)
{
    Globals::g->thinkercap.prev = Globals::g->thinkercap.next  = &Globals::g->thinkercap;
}




//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker (thinker_t* thinker)
{
    Globals::g->thinkercap.prev->next = thinker;
    thinker->next = &Globals::g->thinkercap;
    thinker->prev = Globals::g->thinkercap.prev;
    Globals::g->thinkercap.prev = thinker;
}



//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
void P_RemoveThinker (thinker_t* thinker)
{
  // FIXME: NOP.
  thinker->function.acv = (actionf_v)(-1);
}



//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//
void P_AllocateThinker (thinker_t*	thinker)
{
}



//
// P_RunThinkers
//
void P_RunThinkers (void)
{
    thinker_t*	currentthinker;

    currentthinker = Globals::g->thinkercap.next;
    while (currentthinker != &Globals::g->thinkercap)
    {
		 if ( currentthinker->function.acv == (actionf_v)(-1) )
		 {
			 // time to remove it
			 currentthinker->next->prev = currentthinker->prev;
			 currentthinker->prev->next = currentthinker->next;
			 Z_Free(currentthinker);
		 }
		 else
		 {
			 if (currentthinker->function.acp1)
				 currentthinker->function.acp1 ((mobj_t*)currentthinker);
		 }
	currentthinker = currentthinker->next;
    }
}



//
// P_Ticker
//
extern unsigned char demoversion;

void P_Ticker (void)
{
    int		i;
    
    // run the tic
    if (Globals::g->paused)
		return;

	// don't think during wipe
	if ( !Globals::g->netgame && (!Globals::g->demoplayback || demoversion == VERSION ) && Globals::g->wipe ) {
		return;
	}

    // pause if in menu and at least one tic has been run
    if ( !Globals::g->netgame
	 && Globals::g->menuactive
	 && !Globals::g->demoplayback
	 && Globals::g->players[Globals::g->consoleplayer].viewz != 1)
    {
	return;
    }


	for (i=0 ; i<MAXPLAYERS ; i++) {
		if (Globals::g->playeringame[i]) {
		    P_PlayerThink (&Globals::g->players[i]);
		}
	}

    P_RunThinkers ();
    P_UpdateSpecials ();
    P_RespawnSpecials ();

    // for par times
    Globals::g->leveltime++;	
}

