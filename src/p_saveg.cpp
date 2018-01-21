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

#include "i_system.hpp"
#include "z_zone.hpp"
#include "p_local.hpp"

// State.
#include "doomstat.hpp"
#include "r_state.hpp"



// Pads Globals::g->save_p to a 4-unsigned char boundary
//  so that the load/save works on SGI&Gecko.



//
// P_ArchivePlayers
//
void P_ArchivePlayers (void)
{
    int		i;
    int		j;
    player_t*	dest;
		
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!Globals::g->playeringame[i])
	    continue;
	
	//PADSAVEP();

	dest = (player_t *)Globals::g->save_p;
	memcpy (dest,&Globals::g->players[i],sizeof(player_t));
	Globals::g->save_p += sizeof(player_t);
	for (j=0 ; j<NUMPSPRITES ; j++)
	{
	    if (dest->psprites[j].state)
	    {
		dest->psprites[j].state 
			= (state_t *)(dest->psprites[j].state-Globals::g->states);
	    }
	}
    }
}



//
// P_UnArchivePlayers
//
void P_UnArchivePlayers (void)
{
    int		i;
    int		j;
	
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!Globals::g->playeringame[i])
	    continue;
	
	//PADSAVEP();

	memcpy (&Globals::g->players[i],Globals::g->save_p, sizeof(player_t));
	Globals::g->save_p += sizeof(player_t);
	
	// will be set when unarc thinker
	Globals::g->players[i].mo = NULL;	
	Globals::g->players[i].message = NULL;
	Globals::g->players[i].attacker = NULL;

	for (j=0 ; j<NUMPSPRITES ; j++)
	{
	    if (Globals::g->players[i]. psprites[j].state)
	    {
//        Globals::g->players[i]. psprites[j].state
//            = &Globals::g->states[ (int)Globals::g->players[i].psprites[j].state ];
	    }
	}
    }
}


//
// P_ArchiveWorld
//
void P_ArchiveWorld (void)
{
    int			i;
    int			j;
    sector_t*		sec;
    line_t*		li;
    side_t*		si;
    short*		put;
	
    put = (short *)Globals::g->save_p;
    
    // do Globals::g->sectors
    for (i=0, sec = Globals::g->sectors ; i < Globals::g->numsectors ; i++,sec++)
    {
	*put++ = sec->floorheight >> FRACBITS;
	*put++ = sec->ceilingheight >> FRACBITS;
	*put++ = sec->floorpic;
	*put++ = sec->ceilingpic;
	*put++ = sec->lightlevel;
	*put++ = sec->special;		// needed?
	*put++ = sec->tag;		// needed?
    }

    
    // do Globals::g->lines
    for (i=0, li = Globals::g->lines ; i < Globals::g->numlines ; i++,li++)
    {
	*put++ = li->flags;
	*put++ = li->special;
	*put++ = li->tag;
	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
		continue;
	    
	    si = &Globals::g->sides[li->sidenum[j]];

	    *put++ = si->textureoffset >> FRACBITS;
	    *put++ = si->rowoffset >> FRACBITS;
	    *put++ = si->toptexture;
	    *put++ = si->bottomtexture;
	    *put++ = si->midtexture;	
	}
    }

	// Doom 2 level 30 requires some global pointers, wheee!
	*put++ = Globals::g->braintargeton;
	*put++ = Globals::g->easy;

    Globals::g->save_p = (unsigned char *)put;
}



//
// P_UnArchiveWorld
//
void P_UnArchiveWorld (void)
{
    int			i;
    int			j;
    sector_t*	sec;
    line_t*		li;
    side_t*		si;
    short*		get;
	
    get = (short *)Globals::g->save_p;
    
    // do Globals::g->sectors
    for (i=0, sec = Globals::g->sectors ; i < Globals::g->numsectors ; i++,sec++)
    {
	sec->floorheight = *get++ << FRACBITS;
	sec->ceilingheight = *get++ << FRACBITS;
	sec->floorpic = *get++;
	sec->ceilingpic = *get++;
	sec->lightlevel = *get++;
	sec->special = *get++;		// needed?
	sec->tag = *get++;		// needed?
	sec->specialdata = 0;
	sec->soundtarget = 0;
    }
    
    // do Globals::g->lines
    for (i=0, li = Globals::g->lines ; i < Globals::g->numlines ; i++,li++)
    {
	li->flags = *get++;
	li->special = *get++;
	li->tag = *get++;
	for (j=0 ; j<2 ; j++)
	{
	    if (li->sidenum[j] == -1)
		continue;
	    si = &Globals::g->sides[li->sidenum[j]];
	    si->textureoffset = *get++ << FRACBITS;
	    si->rowoffset = *get++ << FRACBITS;
	    si->toptexture = *get++;
	    si->bottomtexture = *get++;
	    si->midtexture = *get++;
	}
    }

	// Doom 2 level 30 requires some global pointers, wheee!
	Globals::g->braintargeton = *get++;
	Globals::g->easy = *get++;

    Globals::g->save_p = (unsigned char *)get;	
}





//
// Thinkers
//

int GetMOIndex( mobj_t* findme ) {
	thinker_t*	th;
	mobj_t*		mobj;
	int			index = 0;

	for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_MobjThinker) {
			index++;
			mobj = (mobj_t*)th;

			if ( mobj == findme ) {
				return index;
			}
		}
	}

	return 0;
}

mobj_t* GetMO( int index ) {
	thinker_t*	th;
	int			testindex = 0;

	if ( !index ) {
		return NULL;
	}

	for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next)
	{
		if (th->function.acp1 == (actionf_p1)P_MobjThinker) {
			testindex++;

			if ( testindex == index ) {
				return (mobj_t*)th;
			}
		}
	}

	return NULL;
}

//
// P_ArchiveThinkers
//
void P_ArchiveThinkers (void)
{
	thinker_t*		th;
	mobj_t*			mobj;
	ceiling_t*		ceiling;
	vldoor_t*		door;
	floormove_t*	floor;
	plat_t*			plat;
	fireflicker_t*	fire;
	lightflash_t*	flash;
	strobe_t*		strobe;
	glow_t*			glow;

	int i;
	
	// save off the current thinkers
	//I_Printf( "Savegame on leveltime %d\n====================\n", Globals::g->leveltime );

	for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next)
	{
		//mobj_t*	test = (mobj_t*)th;
		//I_Printf( "%3d: %x == function\n", index++, th->function.acp1 );

		if (th->function.acp1 == (actionf_p1)P_MobjThinker)
		{
			*Globals::g->save_p++ = tc_mobj;
//            PADSAVEP();

			mobj = (mobj_t *)Globals::g->save_p;
			memcpy (mobj, th, sizeof(*mobj));
			Globals::g->save_p += sizeof(*mobj);
			mobj->state = (state_t *)(mobj->state - Globals::g->states);

			if (mobj->player)
				mobj->player = (player_t *)((mobj->player-Globals::g->players) + 1);

			// Save out 'target'
			int moIndex = GetMOIndex( mobj->target );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			// Save out 'tracer'
			moIndex = GetMOIndex( mobj->tracer );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->snext );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->sprev );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			// Is this the head of a sector list?
			if ( mobj->subsector->sector->thinglist == (mobj_t*)th ) {
				*Globals::g->save_p++ = 1;
			}
			else {
				*Globals::g->save_p++ = 0;
			}

			moIndex = GetMOIndex( mobj->bnext );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			moIndex = GetMOIndex( mobj->bprev );
			*Globals::g->save_p++ = moIndex >> 8;
			*Globals::g->save_p++ = moIndex;

			// Is this the head of a block list?
			int	blockx = (mobj->x - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
			int	blocky = (mobj->y - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;
			if ( blockx >= 0 && blockx < Globals::g->bmapwidth && blocky >= 0 && blocky < Globals::g->bmapheight 
				&& (mobj_t*)th == Globals::g->blocklinks[blocky*Globals::g->bmapwidth+blockx] ) {

					*Globals::g->save_p++ = 1;
			}
			else {
				*Globals::g->save_p++ = 0;
			}
			continue;
		}

		if (th->function.acv == (actionf_v)NULL)
		{
			for (i = 0; i < MAXCEILINGS;i++)
				if (Globals::g->activeceilings[i] == (ceiling_t *)th)
					break;

			if (i<MAXCEILINGS)
			{
				*Globals::g->save_p++ = tc_ceiling;
//                PADSAVEP();
				ceiling = (ceiling_t *)Globals::g->save_p;
				memcpy (ceiling, th, sizeof(*ceiling));
				Globals::g->save_p += sizeof(*ceiling);
				ceiling->sector = (sector_t *)(ceiling->sector - Globals::g->sectors);
			}
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
		{
			*Globals::g->save_p++ = tc_ceiling;
//            PADSAVEP();
			ceiling = (ceiling_t *)Globals::g->save_p;
			memcpy (ceiling, th, sizeof(*ceiling));
			Globals::g->save_p += sizeof(*ceiling);
			ceiling->sector = (sector_t *)(ceiling->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
		{
			*Globals::g->save_p++ = tc_door;
//            PADSAVEP();
			door = (vldoor_t *)Globals::g->save_p;
			memcpy (door, th, sizeof(*door));
			Globals::g->save_p += sizeof(*door);
			door->sector = (sector_t *)(door->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_MoveFloor)
		{
			*Globals::g->save_p++ = tc_floor;
//            PADSAVEP();
			floor = (floormove_t *)Globals::g->save_p;
			memcpy (floor, th, sizeof(*floor));
			Globals::g->save_p += sizeof(*floor);
			floor->sector = (sector_t *)(floor->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_PlatRaise)
		{
			*Globals::g->save_p++ = tc_plat;
//            PADSAVEP();
			plat = (plat_t *)Globals::g->save_p;
			memcpy (plat, th, sizeof(*plat));
			Globals::g->save_p += sizeof(*plat);
			plat->sector = (sector_t *)(plat->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_FireFlicker)
		{
			*Globals::g->save_p++ = tc_fire;
//            PADSAVEP();
			fire = (fireflicker_t *)Globals::g->save_p;
			memcpy (fire, th, sizeof(*fire));
			Globals::g->save_p += sizeof(*fire);
			fire->sector = (sector_t *)(fire->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_LightFlash)
		{
			*Globals::g->save_p++ = tc_flash;
//            PADSAVEP();
			flash = (lightflash_t *)Globals::g->save_p;
			memcpy (flash, th, sizeof(*flash));
			Globals::g->save_p += sizeof(*flash);
			flash->sector = (sector_t *)(flash->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
		{
			*Globals::g->save_p++ = tc_strobe;
//            PADSAVEP();
			strobe = (strobe_t *)Globals::g->save_p;
			memcpy (strobe, th, sizeof(*strobe));
			Globals::g->save_p += sizeof(*strobe);
			strobe->sector = (sector_t *)(strobe->sector - Globals::g->sectors);
			continue;
		}

		if (th->function.acp1 == (actionf_p1)T_Glow)
		{
			*Globals::g->save_p++ = tc_glow;
//            PADSAVEP();
			glow = (glow_t *)Globals::g->save_p;
			memcpy (glow, th, sizeof(*glow));
			Globals::g->save_p += sizeof(*glow);
			glow->sector = (sector_t *)(glow->sector - Globals::g->sectors);
			continue;
		}
	}

	// add a terminating marker
	*Globals::g->save_p++ = tc_end;

	sector_t* sec;
    short* put = (short *)Globals::g->save_p;
	for (i=0, sec = Globals::g->sectors ; i < Globals::g->numsectors ; i++,sec++) {
		*put++ = (short)GetMOIndex( sec->soundtarget );
	}

	Globals::g->save_p = (unsigned char *)put;

	// add a terminating marker
	*Globals::g->save_p++ = tc_end;
}



//
// P_UnArchiveThinkers
//
void P_UnArchiveThinkers (void)
{
	unsigned char			tclass;
	thinker_t*		currentthinker;
	thinker_t*		next;
	mobj_t*			mobj;
	ceiling_t*		ceiling;
	vldoor_t*		door;
	floormove_t*	floor;
	plat_t*			plat;
	fireflicker_t*	fire;
	lightflash_t*	flash;
	strobe_t*		strobe;
	glow_t*			glow;

	thinker_t*	th;

	int count = 0;
	sector_t* ss = NULL;

	int			mo_index = 0;
	int			mo_targets[1024];
	int			mo_tracers[1024];
	int			mo_snext[1024];
	int			mo_sprev[1024];
	bool		mo_shead[1024];
	int			mo_bnext[1024];
	int			mo_bprev[1024];
	bool		mo_bhead[1024];

	// remove all the current thinkers
	currentthinker = Globals::g->thinkercap.next;
	while (currentthinker != &Globals::g->thinkercap)
	{
		next = currentthinker->next;

		if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
			P_RemoveMobj ((mobj_t *)currentthinker);
		else
			Z_Free(currentthinker);

		currentthinker = next;
	}

	P_InitThinkers ();

	// read in saved thinkers
	while (1)
	{
		tclass = *Globals::g->save_p++;
		switch (tclass)
		{
		case tc_end:

			// clear sector thing lists
			ss = Globals::g->sectors;
			for (int i=0 ; i < Globals::g->numsectors ; i++, ss++) {
				ss->thinglist = NULL;
			}

			// clear blockmap thing lists
			count = sizeof(*Globals::g->blocklinks) * Globals::g->bmapwidth * Globals::g->bmapheight;
			memset (Globals::g->blocklinks, 0, count);

			// Doom 2 level 30 requires some global pointers, wheee!
			Globals::g->numbraintargets = 0;

			// fixup mobj_t pointers now that all thinkers have been restored
			mo_index = 0;
			for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next) {
				if (th->function.acp1 == (actionf_p1)P_MobjThinker) {
					mobj = (mobj_t*)th;

					mobj->target = GetMO( mo_targets[mo_index] );
					mobj->tracer = GetMO( mo_tracers[mo_index] );

					mobj->snext = GetMO( mo_snext[mo_index] );
					mobj->sprev = GetMO( mo_sprev[mo_index] );

					if ( mo_shead[mo_index] ) {
						mobj->subsector->sector->thinglist = mobj;
					}

					mobj->bnext = GetMO( mo_bnext[mo_index] );
					mobj->bprev = GetMO( mo_bprev[mo_index] );

					if ( mo_bhead[mo_index] ) {
						// Is this the head of a block list?
						int	blockx = (mobj->x - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
						int	blocky = (mobj->y - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;
						if ( blockx >= 0 && blockx < Globals::g->bmapwidth && blocky >= 0 && blocky < Globals::g->bmapheight ) {
							Globals::g->blocklinks[blocky*Globals::g->bmapwidth+blockx] = mobj;
						}
					}

					// Doom 2 level 30 requires some global pointers, wheee!
					if ( mobj->type == MT_BOSSTARGET ) {
						Globals::g->braintargets[Globals::g->numbraintargets] = mobj;
						Globals::g->numbraintargets++;
					}

					mo_index++;
				}
			}

			int i;
			sector_t*	sec;
		    short*	get;

			get = (short *)Globals::g->save_p;
			for (i=0, sec = Globals::g->sectors ; i < Globals::g->numsectors ; i++,sec++)
			{
				sec->soundtarget = GetMO( *get++ );
			}
			Globals::g->save_p = (unsigned char *)get;

			tclass = *Globals::g->save_p++;
			if ( tclass != tc_end ) {
				I_Error( "Savegame error after loading sector soundtargets." );
			}

			// print the current thinkers
			//I_Printf( "Loadgame on leveltime %d\n====================\n", Globals::g->leveltime );
			for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next)
			{
				//mobj_t*	test = (mobj_t*)th;
				//I_Printf( "%3d: %x == function\n", index++, th->function.acp1 );
			}

			return; 	// end of list

		case tc_mobj:
//            PADSAVEP();
			mobj = (mobj_t*)DoomLib::Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
			memcpy (mobj, Globals::g->save_p, sizeof(*mobj));
			Globals::g->save_p += sizeof(*mobj);
//            mobj->state = &Globals::g->states[(int)mobj->state];

			mobj->target = NULL;
			mobj->tracer = NULL;

			if (mobj->player)
			{
//                mobj->player = &Globals::g->players[(int)mobj->player-1];
				mobj->player->mo = mobj;
			}

			P_SetThingPosition (mobj);

			mobj->info = &mobjinfo[mobj->type];
			mobj->floorz = mobj->subsector->sector->floorheight;
			mobj->ceilingz = mobj->subsector->sector->ceilingheight;
			mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;

			// Read in 'target' and store for fixup
			int a, b, foundIndex;
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_targets[mo_index] = foundIndex;

			// Read in 'tracer' and store for fixup
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_tracers[mo_index] = foundIndex;

			// Read in 'snext' and store for fixup
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_snext[mo_index] = foundIndex;

			// Read in 'sprev' and store for fixup
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_sprev[mo_index] = foundIndex;

			foundIndex = *Globals::g->save_p++;
			mo_shead[mo_index] = foundIndex == 1;

			// Read in 'bnext' and store for fixup
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_bnext[mo_index] = foundIndex;

			// Read in 'bprev' and store for fixup
			a = *Globals::g->save_p++;
			b = *Globals::g->save_p++;
			foundIndex = (a << 8) + b;
			mo_bprev[mo_index] = foundIndex;

			foundIndex = *Globals::g->save_p++;
			mo_bhead[mo_index] = foundIndex == 1;

			mo_index++;

			P_AddThinker (&mobj->thinker);
			break;

		case tc_ceiling:
//            PADSAVEP();
			ceiling = (ceiling_t*)DoomLib::Z_Malloc(sizeof(*ceiling), PU_LEVEL, NULL);
			memcpy (ceiling, Globals::g->save_p, sizeof(*ceiling));
			Globals::g->save_p += sizeof(*ceiling);
//            ceiling->sector = &Globals::g->sectors[(int)ceiling->sector];
			ceiling->sector->specialdata = ceiling;

			if (ceiling->thinker.function.acp1)
				ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

			P_AddThinker (&ceiling->thinker);
			P_AddActiveCeiling(ceiling);
			break;

		case tc_door:
//            PADSAVEP();
			door = (vldoor_t*)DoomLib::Z_Malloc(sizeof(*door), PU_LEVEL, NULL);
			memcpy (door, Globals::g->save_p, sizeof(*door));
			Globals::g->save_p += sizeof(*door);
//            door->sector = &Globals::g->sectors[(int)door->sector];
			door->sector->specialdata = door;
			door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
			P_AddThinker (&door->thinker);
			break;

		case tc_floor:
//            PADSAVEP();
			floor = (floormove_t*)DoomLib::Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
			memcpy (floor, Globals::g->save_p, sizeof(*floor));
			Globals::g->save_p += sizeof(*floor);
//            floor->sector = &Globals::g->sectors[(int)floor->sector];
			floor->sector->specialdata = floor;
			floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
			P_AddThinker (&floor->thinker);
			break;

		case tc_plat:
//            PADSAVEP();
			plat = (plat_t*)DoomLib::Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
			memcpy (plat, Globals::g->save_p, sizeof(*plat));
			Globals::g->save_p += sizeof(*plat);
//            plat->sector = &Globals::g->sectors[(int)plat->sector];
			plat->sector->specialdata = plat;

			if (plat->thinker.function.acp1)
				plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;

			P_AddThinker (&plat->thinker);
			P_AddActivePlat(plat);
			break;

		case tc_fire:
//            PADSAVEP();
			fire = (fireflicker_t*)DoomLib::Z_Malloc (sizeof(*fire), PU_LEVEL, NULL);
			memcpy (fire, Globals::g->save_p, sizeof(*fire));
			Globals::g->save_p += sizeof(*fire);
//            fire->sector = &Globals::g->sectors[(int)fire->sector];
			fire->thinker.function.acp1 = (actionf_p1)T_FireFlicker;
			P_AddThinker (&fire->thinker);
			break;

		case tc_flash:
//            PADSAVEP();
			flash = (lightflash_t*)DoomLib::Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
			memcpy (flash, Globals::g->save_p, sizeof(*flash));
			Globals::g->save_p += sizeof(*flash);
//            flash->sector = &Globals::g->sectors[(int)flash->sector];
			flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;
			P_AddThinker (&flash->thinker);
			break;

		case tc_strobe:
//            PADSAVEP();
			strobe = (strobe_t*)DoomLib::Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
			memcpy (strobe, Globals::g->save_p, sizeof(*strobe));
			Globals::g->save_p += sizeof(*strobe);
//            strobe->sector = &Globals::g->sectors[(int)strobe->sector];
			strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
			P_AddThinker (&strobe->thinker);
			break;

		case tc_glow:
//            PADSAVEP();
			glow = (glow_t*)DoomLib::Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
			memcpy (glow, Globals::g->save_p, sizeof(*glow));
			Globals::g->save_p += sizeof(*glow);
//            glow->sector = &Globals::g->sectors[(int)glow->sector];
			glow->thinker.function.acp1 = (actionf_p1)T_Glow;
			P_AddThinker (&glow->thinker);
			break;

		default:
			I_Error ("Unknown tclass %i in savegame",tclass);
		}
	}
}


//
// P_ArchiveSpecials
//



//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
void P_ArchiveSpecials (void)
{
    thinker_t*		th;
    ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;
    int			i;
	
    // save off the current thinkers
    for (th = Globals::g->thinkercap.next ; th != &Globals::g->thinkercap ; th=th->next)
    {
	if (th->function.acv == (actionf_v)NULL)
	{
	    for (i = 0; i < MAXCEILINGS;i++)
		if (Globals::g->activeceilings[i] == (ceiling_t *)th)
		    break;
	    
	    if (i<MAXCEILINGS)
	    {
		*Globals::g->save_p++ = tc_ceiling;
//        PADSAVEP();
		ceiling = (ceiling_t *)Globals::g->save_p;
		memcpy (ceiling, th, sizeof(*ceiling));
		Globals::g->save_p += sizeof(*ceiling);
		ceiling->sector = (sector_t *)(ceiling->sector - Globals::g->sectors);
	    }
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
	{
	    *Globals::g->save_p++ = tc_ceiling;
//        PADSAVEP();
	    ceiling = (ceiling_t *)Globals::g->save_p;
	    memcpy (ceiling, th, sizeof(*ceiling));
	    Globals::g->save_p += sizeof(*ceiling);
	    ceiling->sector = (sector_t *)(ceiling->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
	{
	    *Globals::g->save_p++ = tc_door;
//        PADSAVEP();
	    door = (vldoor_t *)Globals::g->save_p;
	    memcpy (door, th, sizeof(*door));
	    Globals::g->save_p += sizeof(*door);
	    door->sector = (sector_t *)(door->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_MoveFloor)
	{
	    *Globals::g->save_p++ = tc_floor;
//        PADSAVEP();
	    floor = (floormove_t *)Globals::g->save_p;
	    memcpy (floor, th, sizeof(*floor));
	    Globals::g->save_p += sizeof(*floor);
	    floor->sector = (sector_t *)(floor->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_PlatRaise)
	{
	    *Globals::g->save_p++ = tc_plat;
//        PADSAVEP();
	    plat = (plat_t *)Globals::g->save_p;
	    memcpy (plat, th, sizeof(*plat));
	    Globals::g->save_p += sizeof(*plat);
	    plat->sector = (sector_t *)(plat->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_LightFlash)
	{
	    *Globals::g->save_p++ = tc_flash;
//        PADSAVEP();
	    flash = (lightflash_t *)Globals::g->save_p;
	    memcpy (flash, th, sizeof(*flash));
	    Globals::g->save_p += sizeof(*flash);
	    flash->sector = (sector_t *)(flash->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
	{
	    *Globals::g->save_p++ = tc_strobe;
//        PADSAVEP();
	    strobe = (strobe_t *)Globals::g->save_p;
	    memcpy (strobe, th, sizeof(*strobe));
	    Globals::g->save_p += sizeof(*strobe);
	    strobe->sector = (sector_t *)(strobe->sector - Globals::g->sectors);
	    continue;
	}
			
	if (th->function.acp1 == (actionf_p1)T_Glow)
	{
	    *Globals::g->save_p++ = tc_glow;
//        PADSAVEP();
	    glow = (glow_t *)Globals::g->save_p;
	    memcpy (glow, th, sizeof(*glow));
	    Globals::g->save_p += sizeof(*glow);
	    glow->sector = (sector_t *)(glow->sector - Globals::g->sectors);
	    continue;
	}
    }
	
    // add a terminating marker
    *Globals::g->save_p++ = tc_endspecials;	

}


//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials (void)
{
    unsigned char		tclass;
    ceiling_t*		ceiling;
    vldoor_t*		door;
    floormove_t*	floor;
    plat_t*		plat;
    lightflash_t*	flash;
    strobe_t*		strobe;
    glow_t*		glow;

    // read in saved thinkers
    while (1)
    {
	tclass = *Globals::g->save_p++;
	switch (tclass)
	{
	  case tc_endspecials:
	    return;	// end of list
			
	  case tc_ceiling:
//        PADSAVEP();
	    ceiling = (ceiling_t*)DoomLib::Z_Malloc(sizeof(*ceiling), PU_LEVEL, NULL);
	    memcpy (ceiling, Globals::g->save_p, sizeof(*ceiling));
	    Globals::g->save_p += sizeof(*ceiling);
//        ceiling->sector = &Globals::g->sectors[(int)ceiling->sector];
	    ceiling->sector->specialdata = ceiling;

	    if (ceiling->thinker.function.acp1)
		ceiling->thinker.function.acp1 = (actionf_p1)T_MoveCeiling;

	    P_AddThinker (&ceiling->thinker);
	    P_AddActiveCeiling(ceiling);
	    break;
				
	  case tc_door:
//        PADSAVEP();
	    door = (vldoor_t*)DoomLib::Z_Malloc(sizeof(*door), PU_LEVEL, NULL);
	    memcpy (door, Globals::g->save_p, sizeof(*door));
	    Globals::g->save_p += sizeof(*door);
//        door->sector = &Globals::g->sectors[(int)door->sector];
	    door->sector->specialdata = door;
	    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
	    P_AddThinker (&door->thinker);
	    break;
				
	  case tc_floor:
//        PADSAVEP();
	    floor = (floormove_t*)DoomLib::Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
	    memcpy (floor, Globals::g->save_p, sizeof(*floor));
	    Globals::g->save_p += sizeof(*floor);
//        floor->sector = &Globals::g->sectors[(int)floor->sector];
	    floor->sector->specialdata = floor;
	    floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
	    P_AddThinker (&floor->thinker);
	    break;
				
	  case tc_plat:
//        PADSAVEP();
	    plat = (plat_t*)DoomLib::Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
	    memcpy (plat, Globals::g->save_p, sizeof(*plat));
	    Globals::g->save_p += sizeof(*plat);
//        plat->sector = &Globals::g->sectors[(int)plat->sector];
	    plat->sector->specialdata = plat;

	    if (plat->thinker.function.acp1)
		plat->thinker.function.acp1 = (actionf_p1)T_PlatRaise;

	    P_AddThinker (&plat->thinker);
	    P_AddActivePlat(plat);
	    break;
				
	  case tc_flash:
//        PADSAVEP();
	    flash = (lightflash_t*)DoomLib::Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
	    memcpy (flash, Globals::g->save_p, sizeof(*flash));
	    Globals::g->save_p += sizeof(*flash);
//        flash->sector = &Globals::g->sectors[(int)flash->sector];
	    flash->thinker.function.acp1 = (actionf_p1)T_LightFlash;
	    P_AddThinker (&flash->thinker);
	    break;
				
	  case tc_strobe:
//        PADSAVEP();
	    strobe = (strobe_t*)DoomLib::Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
	    memcpy (strobe, Globals::g->save_p, sizeof(*strobe));
	    Globals::g->save_p += sizeof(*strobe);
//        strobe->sector = &Globals::g->sectors[(int)strobe->sector];
	    strobe->thinker.function.acp1 = (actionf_p1)T_StrobeFlash;
	    P_AddThinker (&strobe->thinker);
	    break;
				
	  case tc_glow:
//        PADSAVEP();
	    glow = (glow_t*)DoomLib::Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
	    memcpy (glow, Globals::g->save_p, sizeof(*glow));
	    Globals::g->save_p += sizeof(*glow);
//        glow->sector = &Globals::g->sectors[(int)glow->sector];
	    glow->thinker.function.acp1 = (actionf_p1)T_Glow;
	    P_AddThinker (&glow->thinker);
	    break;
				
	  default:
	    I_Error ("P_UnarchiveSpecials:Unknown tclass %i "
		     "in savegame",tclass);
	}
	
    }

}


