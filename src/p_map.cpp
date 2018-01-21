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

#include <stdlib.h>

#include "m_bbox.hpp"
#include "m_random.hpp"
#include "i_system.hpp"

#include "doomdef.hpp"
#include "p_local.hpp"

#include "s_sound.hpp"

// State.
#include "doomstat.hpp"
#include "r_state.hpp"
// Data.
#include "sounds.hpp"

#include "Main.hpp"


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".


// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls

// keep track of special Globals::g->lines as they are hit,
// but don't process them until the move is proven valid




//
// TELEPORT MOVE
// 

//
// PIT_StompThing
//
qboolean PIT_StompThing (mobj_t* thing)
{
    fixed_t	blockdist;
		
    if (!(thing->flags & MF_SHOOTABLE) )
	return true;
		
    blockdist = thing->radius + Globals::g->tmthing->radius;
    
    if ( abs(thing->x - Globals::g->tmx) >= blockdist
	 || abs(thing->y - Globals::g->tmy) >= blockdist )
    {
	// didn't hit it
	return true;
    }
    
    // don't clip against self
    if (thing == Globals::g->tmthing)
	return true;
    
    // monsters don't stomp things except on boss level
    if ( !Globals::g->tmthing->player && Globals::g->gamemap != 30)
	return false;	
		
    P_DamageMobj (thing, Globals::g->tmthing, Globals::g->tmthing, 10000);
	
    return true;
}


//
// P_TeleportMove
//
qboolean
P_TeleportMove
( mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    
    subsector_t*	newsubsec;
    
    // kill anything occupying the position
    Globals::g->tmthing = thing;
    Globals::g->tmflags = thing->flags;
	
    Globals::g->tmx = x;
    Globals::g->tmy = y;
	
    Globals::g->tmbbox[BOXTOP] = y + Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXBOTTOM] = y - Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXRIGHT] = x + Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXLEFT] = x - Globals::g->tmthing->radius;

    newsubsec = R_PointInSubsector (x,y);
    Globals::g->ceilingline = NULL;
    
    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted Globals::g->lines the step closer together
    // will adjust them.
    Globals::g->tmfloorz = Globals::g->tmdropoffz = newsubsec->sector->floorheight;
    Globals::g->tmceilingz = newsubsec->sector->ceilingheight;
			
    Globals::g->validcount++;
    Globals::g->numspechit = 0;
    
    // stomp on any things contacted
    xl = (Globals::g->tmbbox[BOXLEFT] - Globals::g->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (Globals::g->tmbbox[BOXRIGHT] - Globals::g->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (Globals::g->tmbbox[BOXBOTTOM] - Globals::g->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (Globals::g->tmbbox[BOXTOP] - Globals::g->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
		return false;
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    thing->floorz = Globals::g->tmfloorz;
    thing->ceilingz = Globals::g->tmceilingz;	
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);
	
    return true;
}


//
// MOVEMENT ITERATOR FUNCTIONS
//


//
// PIT_CheckLine
// Adjusts Globals::g->tmfloorz and Globals::g->tmceilingz as Globals::g->lines are contacted
//
qboolean PIT_CheckLine (line_t* ld)
{
    if (Globals::g->tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
	|| Globals::g->tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
	|| Globals::g->tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
	|| Globals::g->tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
	return true;

    if (P_BoxOnLineSide (Globals::g->tmbbox, ld) != -1)
	return true;
		
    // A line has been hit
    
    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special Globals::g->lines that are only 8 pixels apart
    // could be crossed in either order.
    
    if (!ld->backsector)
	return false;		// one sided line
		
    if (!(Globals::g->tmthing->flags & MF_MISSILE) )
    {
	if ( ld->flags & ML_BLOCKING )
	    return false;	// explicitly blocking everything

	if ( !Globals::g->tmthing->player && ld->flags & ML_BLOCKMONSTERS )
	    return false;	// block monsters only
    }

    // set Globals::g->openrange, Globals::g->opentop, Globals::g->openbottom
    P_LineOpening (ld);	
	
    // adjust floor / ceiling heights
    if (Globals::g->opentop < Globals::g->tmceilingz)
    {
	Globals::g->tmceilingz = Globals::g->opentop;
	Globals::g->ceilingline = ld;
    }

    if (Globals::g->openbottom > Globals::g->tmfloorz)
	Globals::g->tmfloorz = Globals::g->openbottom;	

    if (Globals::g->lowfloor < Globals::g->tmdropoffz)
	Globals::g->tmdropoffz = Globals::g->lowfloor;

    // if contacted a special line, add it to the list
    if (ld->special && Globals::g->numspechit < MAXSPECIALCROSS )
    {
	Globals::g->spechit[Globals::g->numspechit] = ld;
	Globals::g->numspechit++;
    }

    return true;
}

//
// PIT_CheckThing
//
qboolean PIT_CheckThing (mobj_t* thing)
{
    fixed_t		blockdist;
    qboolean		solid;
    int			damage;
		
    if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
	return true;
    
    blockdist = thing->radius + Globals::g->tmthing->radius;

    if ( abs(thing->x - Globals::g->tmx) >= blockdist
	 || abs(thing->y - Globals::g->tmy) >= blockdist )
    {
	// didn't hit it
	return true;	
    }
    
    // don't clip against self
    if (thing == Globals::g->tmthing)
	return true;
    
    // check for skulls slamming into things
    if (Globals::g->tmthing->flags & MF_SKULLFLY)
    {
	damage = ((P_Random()%8)+1)*Globals::g->tmthing->info->damage;
	
	P_DamageMobj (thing, Globals::g->tmthing, Globals::g->tmthing, damage);
	
	Globals::g->tmthing->flags &= ~MF_SKULLFLY;
	Globals::g->tmthing->momx = Globals::g->tmthing->momy = Globals::g->tmthing->momz = 0;
	
	P_SetMobjState (Globals::g->tmthing, (statenum_t)Globals::g->tmthing->info->spawnstate);
	
	return false;		// stop moving
    }

    
    // missiles can hit other things
    if (Globals::g->tmthing->flags & MF_MISSILE)
    {
	// see if it went over / under
	if (Globals::g->tmthing->z > thing->z + thing->height)
	    return true;		// overhead
	if (Globals::g->tmthing->z+Globals::g->tmthing->height < thing->z)
	    return true;		// underneath
		
	if (Globals::g->tmthing->target && (
	    Globals::g->tmthing->target->type == thing->type || 
	    (Globals::g->tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
	    (Globals::g->tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
	{
	    // Don't hit same species as originator.
	    if (thing == Globals::g->tmthing->target)
		return true;

	    if (thing->type != MT_PLAYER)
	    {
		// Explode, but do no damage.
		// Let Globals::g->players missile other Globals::g->players.
		return false;
	    }
	}
	
	if (! (thing->flags & MF_SHOOTABLE) )
	{
	    // didn't do any damage
	    return !(thing->flags & MF_SOLID);	
	}
	
	// damage / explode
	damage = ((P_Random()%8)+1)*Globals::g->tmthing->info->damage;
	P_DamageMobj (thing, Globals::g->tmthing, Globals::g->tmthing->target, damage);

	// don't traverse any more
	return false;				
    }
    
    // check for special pickup
    if (thing->flags & MF_SPECIAL)
    {
	solid = thing->flags&MF_SOLID;
	if (Globals::g->tmflags&MF_PICKUP)
	{
	    // can remove thing
	    P_TouchSpecialThing (thing, Globals::g->tmthing);
	}
	return !solid;
    }
	
    return !(thing->flags & MF_SOLID);
}


//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
// 
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  Globals::g->tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
qboolean
P_CheckPosition
( mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;
    int			bx;
    int			by;
    subsector_t*	newsubsec;

    Globals::g->tmthing = thing;
    Globals::g->tmflags = thing->flags;
	
    Globals::g->tmx = x;
    Globals::g->tmy = y;
	
    Globals::g->tmbbox[BOXTOP] = y + Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXBOTTOM] = y - Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXRIGHT] = x + Globals::g->tmthing->radius;
    Globals::g->tmbbox[BOXLEFT] = x - Globals::g->tmthing->radius;

    newsubsec = R_PointInSubsector (x,y);
    Globals::g->ceilingline = NULL;
    
    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted Globals::g->lines the step closer together
    // will adjust them.
    Globals::g->tmfloorz = Globals::g->tmdropoffz = newsubsec->sector->floorheight;
    Globals::g->tmceilingz = newsubsec->sector->ceilingheight;
			
    Globals::g->validcount++;
    Globals::g->numspechit = 0;

    if ( Globals::g->tmflags & MF_NOCLIP )
	return true;
    
    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    xl = (Globals::g->tmbbox[BOXLEFT] - Globals::g->bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (Globals::g->tmbbox[BOXRIGHT] - Globals::g->bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (Globals::g->tmbbox[BOXBOTTOM] - Globals::g->bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (Globals::g->tmbbox[BOXTOP] - Globals::g->bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
		return false;
    
    // check Globals::g->lines
    xl = (Globals::g->tmbbox[BOXLEFT] - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
    xh = (Globals::g->tmbbox[BOXRIGHT] - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
    yl = (Globals::g->tmbbox[BOXBOTTOM] - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;
    yh = (Globals::g->tmbbox[BOXTOP] - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
		return false;

    return true;
}


//
// P_TryMove
// Attempt to move to a new position,
// crossing special Globals::g->lines unless MF_TELEPORT is set.
//
qboolean
P_TryMove
( mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    fixed_t	oldx;
    fixed_t	oldy;
    int		side;
    int		oldside;
    line_t*	ld;

    Globals::g->floatok = false;
    if (!P_CheckPosition (thing, x, y))
	return false;		// solid wall or thing
    
    if ( !(thing->flags & MF_NOCLIP) )
    {
	if (Globals::g->tmceilingz - Globals::g->tmfloorz < thing->height)
	    return false;	// doesn't fit

	Globals::g->floatok = true;
	
	if ( !(thing->flags&MF_TELEPORT) 
	     &&Globals::g->tmceilingz - thing->z < thing->height)
	    return false;	// mobj must lower itself to fit

	if ( !(thing->flags&MF_TELEPORT)
	     && Globals::g->tmfloorz - thing->z > 24*FRACUNIT )
	    return false;	// too big a step up

	if ( !(thing->flags&(MF_DROPOFF|MF_FLOAT))
	     && Globals::g->tmfloorz - Globals::g->tmdropoffz > 24*FRACUNIT )
	    return false;	// don't stand over a dropoff
    }
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = Globals::g->tmfloorz;
    thing->ceilingz = Globals::g->tmceilingz;	
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);
    
    // if any special Globals::g->lines were hit, do the effect
    if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
    {
		while (Globals::g->numspechit--)
		{
			// see if the line was crossed
			ld = Globals::g->spechit[Globals::g->numspechit];
			side = P_PointOnLineSide (thing->x, thing->y, ld);
			oldside = P_PointOnLineSide (oldx, oldy, ld);
			if (side != oldside)
			{
			if (ld->special)
				P_CrossSpecialLine (ld-Globals::g->lines, oldside, thing);
			}
		}
    }

    return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
qboolean P_ThingHeightClip (mobj_t* thing)
{
    qboolean		onfloor;
	
    onfloor = (thing->z == thing->floorz);
	
    P_CheckPosition (thing, thing->x, thing->y);	
    // what about stranding a monster partially off an edge?
	
    thing->floorz = Globals::g->tmfloorz;
    thing->ceilingz = Globals::g->tmceilingz;
	
    if (onfloor)
    {
	// walking monsters rise and fall with the floor
	thing->z = thing->floorz;
    }
    else
    {
	// don't adjust a floating monster unless forced to
	if (thing->z+thing->height > thing->ceilingz)
	    thing->z = thing->ceilingz - thing->height;
    }
	
    if (thing->ceilingz - thing->floorz < thing->height)
	return false;
		
    return true;
}



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//






//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine (line_t* ld)
{
    int			side;

    angle_t		lineangle;
    angle_t		moveangle;
    angle_t		deltaangle;
    
    fixed_t		movelen;
    fixed_t		newlen;
	
	
    if (ld->slopetype == ST_HORIZONTAL)
    {
	Globals::g->tmymove = 0;
	return;
    }
    
    if (ld->slopetype == ST_VERTICAL)
    {
	Globals::g->tmxmove = 0;
	return;
    }
	
    side = P_PointOnLineSide (Globals::g->slidemo->x, Globals::g->slidemo->y, ld);
	
    lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);

    if (side == 1)
	lineangle += ANG180;

    moveangle = R_PointToAngle2 (0,0, Globals::g->tmxmove, Globals::g->tmymove);
    deltaangle = moveangle-lineangle;

    if (deltaangle > ANG180)
	deltaangle += ANG180;
    //	I_Error ("SlideLine: ang>ANG180");

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;
	
    movelen = P_AproxDistance (Globals::g->tmxmove, Globals::g->tmymove);
    newlen = FixedMul (movelen, finecosine[deltaangle]);

    Globals::g->tmxmove = FixedMul (newlen, finecosine[lineangle]);	
    Globals::g->tmymove = FixedMul (newlen, finesine[lineangle]);	
}


//
// PTR_SlideTraverse
//
qboolean PTR_SlideTraverse (intercept_t* in)
{
    line_t*	li;
	
    if (!in->isaline)
	I_Error ("PTR_SlideTraverse: not a line?");
		
    li = in->d.line;
    
    if ( ! (li->flags & ML_TWOSIDED) )
    {
	if (P_PointOnLineSide (Globals::g->slidemo->x, Globals::g->slidemo->y, li))
	{
	    // don't hit the back side
	    return true;		
	}
	goto isblocking;
    }

    // set Globals::g->openrange, Globals::g->opentop, Globals::g->openbottom
    P_LineOpening (li);
    
    if (Globals::g->openrange < Globals::g->slidemo->height)
	goto isblocking;		// doesn't fit
		
    if (Globals::g->opentop - Globals::g->slidemo->z < Globals::g->slidemo->height)
	goto isblocking;		// mobj is too high

    if (Globals::g->openbottom - Globals::g->slidemo->z > 24*FRACUNIT )
	goto isblocking;		// too big a step up

    // this line doesn't block movement
    return true;		
	
    // the line does block movement,
    // see if it is closer than best so far
  isblocking:		
    if (in->frac < Globals::g->bestslidefrac)
    {
	Globals::g->secondslidefrac = Globals::g->bestslidefrac;
	Globals::g->secondslideline = Globals::g->bestslideline;
	Globals::g->bestslidefrac = in->frac;
	Globals::g->bestslideline = li;
    }
	
    return false;	// stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (mobj_t* mo)
{
    fixed_t		leadx;
    fixed_t		leady;
    fixed_t		trailx;
    fixed_t		traily;
    fixed_t		newx;
    fixed_t		newy;
    int			hitcount;
		
    Globals::g->slidemo = mo;
    hitcount = 0;
    
  retry:
    if (++hitcount == 3)
	goto stairstep;		// don't loop forever

    
    // Globals::g->trace along the three leading corners
    if (mo->momx > 0)
    {
	leadx = mo->x + mo->radius;
	trailx = mo->x - mo->radius;
    }
    else
    {
	leadx = mo->x - mo->radius;
	trailx = mo->x + mo->radius;
    }
	
    if (mo->momy > 0)
    {
	leady = mo->y + mo->radius;
	traily = mo->y - mo->radius;
    }
    else
    {
	leady = mo->y - mo->radius;
	traily = mo->y + mo->radius;
    }
		
    Globals::g->bestslidefrac = FRACUNIT+1;
	
    P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    
    // move up to the wall
    if (Globals::g->bestslidefrac == FRACUNIT+1)
    {
	// the move most have hit the middle, so stairstep
      stairstep:
	if (!P_TryMove (mo, mo->x, mo->y + mo->momy))
	    P_TryMove (mo, mo->x + mo->momx, mo->y);
	return;
    }

    // fudge a bit to make sure it doesn't hit
    Globals::g->bestslidefrac -= 0x800;	
    if (Globals::g->bestslidefrac > 0)
    {
	newx = FixedMul (mo->momx, Globals::g->bestslidefrac);
	newy = FixedMul (mo->momy, Globals::g->bestslidefrac);
	
	if (!P_TryMove (mo, mo->x+newx, mo->y+newy))
	    goto stairstep;
    }
    
    // Now continue along the wall.
    // First calculate remainder.
    Globals::g->bestslidefrac = FRACUNIT-(Globals::g->bestslidefrac+0x800);
    
    if (Globals::g->bestslidefrac > FRACUNIT)
	Globals::g->bestslidefrac = FRACUNIT;
    
    if (Globals::g->bestslidefrac <= 0)
	return;
    
    Globals::g->tmxmove = FixedMul (mo->momx, Globals::g->bestslidefrac);
    Globals::g->tmymove = FixedMul (mo->momy, Globals::g->bestslidefrac);

    P_HitSlideLine (Globals::g->bestslideline);	// clip the moves

    mo->momx = Globals::g->tmxmove;
    mo->momy = Globals::g->tmymove;
		
    if (!P_TryMove (mo, mo->x+Globals::g->tmxmove, mo->y+Globals::g->tmymove))
    {
	goto retry;
    }
}


//
// P_LineAttack
//

// Height if not aiming up or down
// ???: use slope for monsters?



// slopes to top and bottom of target


//
// PTR_AimTraverse
// Sets linetaget and Globals::g->aimslope when a target is aimed at.
//
qboolean
PTR_AimTraverse (intercept_t* in)
{
    line_t*		li;
    mobj_t*		th;
    fixed_t		slope;
    fixed_t		thingtopslope;
    fixed_t		thingbottomslope;
    fixed_t		dist;
		
    if (in->isaline)
    {
	li = in->d.line;
	
	if ( !(li->flags & ML_TWOSIDED) )
	    return false;		// stop
	
	// Crosses a two sided line.
	// A two sided line will restrict
	// the possible target ranges.
	P_LineOpening (li);
	
	if (Globals::g->openbottom >= Globals::g->opentop)
	    return false;		// stop
	
	dist = FixedMul (Globals::g->attackrange, in->frac);

	if (li->frontsector->floorheight != li->backsector->floorheight)
	{
	    slope = FixedDiv (Globals::g->openbottom - Globals::g->shootz , dist);
	    if (slope > Globals::g->bottomslope)
		Globals::g->bottomslope = slope;
	}
		
	if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	    slope = FixedDiv (Globals::g->opentop - Globals::g->shootz , dist);
	    if (slope < Globals::g->topslope)
		Globals::g->topslope = slope;
	}
		
	if (Globals::g->topslope <= Globals::g->bottomslope)
	    return false;		// stop
			
	return true;			// shot continues
    }
    
    // shoot a thing
    th = in->d.thing;
    if (th == Globals::g->shootthing)
	return true;			// can't shoot self
    
    if (!(th->flags&MF_SHOOTABLE))
	return true;			// corpse or something

    // check angles to see if the thing can be aimed at
    dist = FixedMul (Globals::g->attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - Globals::g->shootz , dist);

    if (thingtopslope < Globals::g->bottomslope)
	return true;			// shot over the thing

    thingbottomslope = FixedDiv (th->z - Globals::g->shootz, dist);

    if (thingbottomslope > Globals::g->topslope)
	return true;			// shot under the thing
    
    // this thing can be hit!
    if (thingtopslope > Globals::g->topslope)
	thingtopslope = Globals::g->topslope;
    
    if (thingbottomslope < Globals::g->bottomslope)
	thingbottomslope = Globals::g->bottomslope;

    Globals::g->aimslope = (thingtopslope+thingbottomslope)/2;
    Globals::g->linetarget = th;

    return false;			// don't go any farther
}


//
// PTR_ShootTraverse
//
qboolean PTR_ShootTraverse (intercept_t* in)
{
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;
    fixed_t		frac;
    
    line_t*		li;
    
    mobj_t*		th;

    fixed_t		slope;
    fixed_t		dist;
    fixed_t		thingtopslope;
    fixed_t		thingbottomslope;
		
    if (in->isaline)
    {
	li = in->d.line;
	
	if (li->special)
	    P_ShootSpecialLine (Globals::g->shootthing, li);

	if ( !(li->flags & ML_TWOSIDED) )
	    goto hitline;
	
	// crosses a two sided line
	P_LineOpening (li);
		
	dist = FixedMul (Globals::g->attackrange, in->frac);

	if (li->frontsector->floorheight != li->backsector->floorheight)
	{
	    slope = FixedDiv (Globals::g->openbottom - Globals::g->shootz , dist);
	    if (slope > Globals::g->aimslope)
		goto hitline;
	}
		
	if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	    slope = FixedDiv (Globals::g->opentop - Globals::g->shootz , dist);
	    if (slope < Globals::g->aimslope)
		goto hitline;
	}

	// shot continues
	return true;
	
	
	// hit line
      hitline:
	// position a bit closer
	frac = in->frac - FixedDiv (4*FRACUNIT,Globals::g->attackrange);
	x = Globals::g->trace.x + FixedMul (Globals::g->trace.dx, frac);
	y = Globals::g->trace.y + FixedMul (Globals::g->trace.dy, frac);
	z = Globals::g->shootz + FixedMul (Globals::g->aimslope, FixedMul(frac, Globals::g->attackrange));

	if (li->frontsector->ceilingpic == Globals::g->skyflatnum)
	{
	    // don't shoot the sky!
	    if (z > li->frontsector->ceilingheight)
		return false;
	    
	    // it's a sky hack wall
	    if	(li->backsector && li->backsector->ceilingpic == Globals::g->skyflatnum)
		return false;		
	}

	mobj_t * sourceObject = Globals::g->shootthing;
	if( sourceObject ) {

		if( ( sourceObject->player) == &(Globals::g->players[DoomLib::GetPlayer()]) ) {
			
			// Fist Punch.
			if( Globals::g->attackrange == MELEERANGE ) {
			}
		}
	}

	// Spawn bullet puffs.
	P_SpawnPuff (x,y,z);
	
	// don't go any farther
	return false;	
    }
    
    // shoot a thing
    th = in->d.thing;
    if (th == Globals::g->shootthing)
	return true;		// can't shoot self
    
    if (!(th->flags&MF_SHOOTABLE))
	return true;		// corpse or something
		
    // check angles to see if the thing can be aimed at
    dist = FixedMul (Globals::g->attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - Globals::g->shootz , dist);

    if (thingtopslope < Globals::g->aimslope)
	return true;		// shot over the thing

    thingbottomslope = FixedDiv (th->z - Globals::g->shootz, dist);

    if (thingbottomslope > Globals::g->aimslope)
	return true;		// shot under the thing

    
    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv (10*FRACUNIT,Globals::g->attackrange);

    x = Globals::g->trace.x + FixedMul (Globals::g->trace.dx, frac);
    y = Globals::g->trace.y + FixedMul (Globals::g->trace.dy, frac);
    z = Globals::g->shootz + FixedMul (Globals::g->aimslope, FixedMul(frac, Globals::g->attackrange));

	// check for friendly fire.
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if( th  && gameLocal->GetMatchParms().GetGameType() != GAME_TYPE_PVP ) {
		player_t * hitPlayer = th->player;

		if( hitPlayer ) {

			mobj_t * sourceObject = Globals::g->shootthing;

			if( sourceObject ) {
				player_t* sourcePlayer = sourceObject->player;

				if( sourcePlayer != NULL && sourcePlayer != hitPlayer  && !gameLocal->GetMatchParms().AllowFriendlyFire() ) {
					return true;
				}
			}
		}
	}
#endif

	mobj_t * sourceObject = Globals::g->shootthing;
	if( sourceObject ) {

		if( ( sourceObject->player) == &(Globals::g->players[DoomLib::GetPlayer()]) ) {

			// Fist Punch.
			if( Globals::g->attackrange == MELEERANGE ) {
			}
		}
	}


    // Spawn bullet puffs or blod spots,
    // depending on target type.
    if (in->d.thing->flags & MF_NOBLOOD)
	P_SpawnPuff (x,y,z);
    else
	P_SpawnBlood (x,y,z, Globals::g->la_damage);

    if (Globals::g->la_damage)
	P_DamageMobj (th, Globals::g->shootthing, Globals::g->shootthing, Globals::g->la_damage);

    // don't go any farther
    return false;
	
}


//
// P_AimLineAttack
//
fixed_t
P_AimLineAttack
( mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance )
{
    fixed_t	x2;
    fixed_t	y2;
	
    angle >>= ANGLETOFINESHIFT;
    Globals::g->shootthing = t1;
    
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    Globals::g->shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

    // can't shoot outside view angles
    Globals::g->topslope = 100*FRACUNIT/160;	
    Globals::g->bottomslope = -100*FRACUNIT/160;
    
    Globals::g->attackrange = distance;
    Globals::g->linetarget = NULL;
	
    P_PathTraverse ( t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_AimTraverse );
		
    if (Globals::g->linetarget)
	return Globals::g->aimslope;

    return 0;
}
 

//
// P_LineAttack
// If damage == 0, it is just a test Globals::g->trace
// that will leave Globals::g->linetarget set.
//
void
P_LineAttack
( mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance,
  fixed_t	slope,
  int		damage )
{
    fixed_t	x2;
    fixed_t	y2;
	
    angle >>= ANGLETOFINESHIFT;
    Globals::g->shootthing = t1;
    Globals::g->la_damage = damage;
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    Globals::g->shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
    Globals::g->attackrange = distance;
    Globals::g->aimslope = slope;
		
    P_PathTraverse ( t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_ShootTraverse );
}
 


//
// USE LINES
//

qboolean	PTR_UseTraverse (intercept_t* in)
{
    int		side;
	
    if (!in->d.line->special)
    {
	P_LineOpening (in->d.line);
	if (Globals::g->openrange <= 0)
	{
	    S_StartSound (Globals::g->usething, sfx_noway);
	    
	    // can't use through a wall
	    return false;	
	}
	// not a special line, but keep checking
	return true ;		
    }
	
    side = 0;
    if (P_PointOnLineSide (Globals::g->usething->x, Globals::g->usething->y, in->d.line) == 1)
	side = 1;
    
    //	return false;		// don't use back side
	
    P_UseSpecialLine (Globals::g->usething, in->d.line, side);

    // can't use for than one special line in a row
    return false;
}


//
// P_UseLines
// Looks for special Globals::g->lines in front of the player to activate.
//
void P_UseLines (player_t*	player) 
{
    int		angle;
    fixed_t	x1;
    fixed_t	y1;
    fixed_t	x2;
    fixed_t	y2;
	
    Globals::g->usething = player->mo;
		
    angle = player->mo->angle >> ANGLETOFINESHIFT;

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
    y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];
	
    P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
}


//
// RADIUS ATTACK
//


//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
qboolean PIT_RadiusAttack (mobj_t* thing)
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	dist;
	
    if (!(thing->flags & MF_SHOOTABLE) )
	return true;

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG
	|| thing->type == MT_SPIDER)
	return true;	
		
    dx = abs(thing->x - Globals::g->bombspot->x);
    dy = abs(thing->y - Globals::g->bombspot->y);
    
    dist = dx>dy ? dx : dy;
    dist = (dist - thing->radius) >> FRACBITS;

    if (dist < 0)
	dist = 0;

    if (dist >= Globals::g->bombdamage)
	return true;	// out of range

    if ( P_CheckSight (thing, Globals::g->bombspot) )
    {
	// must be in direct path
	P_DamageMobj (thing, Globals::g->bombspot, Globals::g->bombsource, Globals::g->bombdamage - dist);
    }
    
    return true;
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void
P_RadiusAttack
( mobj_t*	spot,
  mobj_t*	source,
  int		damage )
{
    int		x;
    int		y;
    
    int		xl;
    int		xh;
    int		yl;
    int		yh;
    
    fixed_t	dist;
	
    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
    Globals::g->bombspot = spot;
    Globals::g->bombsource = source;
    Globals::g->bombdamage = damage;
	
    for (y=yl ; y<=yh ; y++)
	for (x=xl ; x<=xh ; x++)
	    P_BlockThingsIterator (x, y, PIT_RadiusAttack );
}



//
// SECTOR HEIGHT CHANGING
// After modifying a Globals::g->sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//


//
// PIT_ChangeSector
//
qboolean PIT_ChangeSector (mobj_t*	thing)
{
    mobj_t*	mo;
	
    if (P_ThingHeightClip (thing))
    {
	// keep checking
	return true;
    }
    

    // crunch bodies to giblets
    if (thing->health <= 0)
    {
	P_SetMobjState (thing, S_GIBS);

	thing->flags &= ~MF_SOLID;
	thing->height = 0;
	thing->radius = 0;

	// keep checking
	return true;		
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED)
    {
	P_RemoveMobj (thing);
	
	// keep checking
	return true;		
    }

    if (! (thing->flags & MF_SHOOTABLE) )
    {
	// assume it is bloody gibs or something
	return true;			
    }
    
    Globals::g->nofit = true;

    if (Globals::g->crushchange && !(Globals::g->leveltime&3) )
    {
	P_DamageMobj(thing,NULL,NULL,10);

	// spray blood in a random direction
	mo = P_SpawnMobj (thing->x,
			  thing->y,
			  thing->z + thing->height/2, MT_BLOOD);
	
	mo->momx = (P_Random() - P_Random ())<<12;
	mo->momy = (P_Random() - P_Random ())<<12;
    }

    // keep checking (crush other things)	
    return true;	
}



//
// P_ChangeSector
//
qboolean
P_ChangeSector
( sector_t*	sector,
  qboolean	crunch )
{
    int		x;
    int		y;
	
    Globals::g->nofit = false;
    Globals::g->crushchange = crunch;
	
    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
	for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
	    P_BlockThingsIterator (x, y, PIT_ChangeSector);
	
	
    return Globals::g->nofit;
}


