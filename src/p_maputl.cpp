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

#include "doomdef.hpp"
#include "p_local.hpp"


// State.
#include "r_state.hpp"

//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t
P_AproxDistance
( fixed_t	dx,
  fixed_t	dy )
{
    dx = abs(dx);
    dy = abs(dy);
    if (dx < dy)
	return dx+dy-(dx>>1);
    return dx+dy-(dy>>1);
}


//
// P_PointOnLineSide
// Returns 0 or 1
//
int
P_PointOnLineSide
( fixed_t	x,
  fixed_t	y,
  line_t*	line )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;
	
    if (!line->dx)
    {
	if (x <= line->v1->x)
	    return line->dy > 0;
	
	return line->dy < 0;
    }
    if (!line->dy)
    {
	if (y <= line->v1->y)
	    return line->dx < 0;
	
	return line->dx > 0;
    }
	
    dx = (x - line->v1->x);
    dy = (y - line->v1->y);
	
    left = FixedMul ( line->dy>>FRACBITS , dx );
    right = FixedMul ( dy , line->dx>>FRACBITS );
	
    if (right < left)
	return 0;		// front side
    return 1;			// back side
}



//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int
P_BoxOnLineSide
( fixed_t*	tmbox,
  line_t*	ld )
{
    int		p1 = 0;
    int		p2 = 0;
	
    switch (ld->slopetype)
    {
      case ST_HORIZONTAL:
	p1 = tmbox[BOXTOP] > ld->v1->y;
	p2 = tmbox[BOXBOTTOM] > ld->v1->y;
	if (ld->dx < 0)
	{
	    p1 ^= 1;
	    p2 ^= 1;
	}
	break;
	
      case ST_VERTICAL:
	p1 = tmbox[BOXRIGHT] < ld->v1->x;
	p2 = tmbox[BOXLEFT] < ld->v1->x;
	if (ld->dy < 0)
	{
	    p1 ^= 1;
	    p2 ^= 1;
	}
	break;
	
      case ST_POSITIVE:
	p1 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXTOP], ld);
	p2 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
	break;
	
      case ST_NEGATIVE:
	p1 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
	p2 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
	break;
    }

    if (p1 == p2)
	return p1;
    return -1;
}


//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
int
P_PointOnDivlineSide
( fixed_t	x,
  fixed_t	y,
  divline_t*	line )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;
	
    if (!line->dx)
    {
	if (x <= line->x)
	    return line->dy > 0;
	
	return line->dy < 0;
    }
    if (!line->dy)
    {
	if (y <= line->y)
	    return line->dx < 0;

	return line->dx > 0;
    }
	
    dx = (x - line->x);
    dy = (y - line->y);
	
    // try to quickly decide by looking at sign bits
    if ( (line->dy ^ line->dx ^ dx ^ dy)&0x80000000 )
    {
	if ( (line->dy ^ dx) & 0x80000000 )
	    return 1;		// (left is negative)
	return 0;
    }
	
    left = FixedMul ( line->dy>>8, dx>>8 );
    right = FixedMul ( dy>>8 , line->dx>>8 );
	
    if (right < left)
	return 0;		// front side
    return 1;			// back side
}



//
// P_MakeDivline
//
void
P_MakeDivline
( line_t*	li,
  divline_t*	dl )
{
    dl->x = li->v1->x;
    dl->y = li->v1->y;
    dl->dx = li->dx;
    dl->dy = li->dy;
}



//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
fixed_t
P_InterceptVector
( divline_t*	v2,
  divline_t*	v1 )
{
#if 1
    fixed_t	frac;
    fixed_t	num;
    fixed_t	den;
	
    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

    if (den == 0)
	return 0;
    //	I_Error ("P_InterceptVector: parallel");
    
    num =
	FixedMul ( (v1->x - v2->x)>>8 ,v1->dy )
	+FixedMul ( (v2->y - v1->y)>>8, v1->dx );

    frac = FixedDiv (num , den);

    return frac;
#else	// UNUSED, float debug.
    float	frac;
    float	num;
    float	den;
    float	v1x;
    float	v1y;
    float	v1dx;
    float	v1dy;
    float	v2x;
    float	v2y;
    float	v2dx;
    float	v2dy;

    v1x = (float)v1->x/FRACUNIT;
    v1y = (float)v1->y/FRACUNIT;
    v1dx = (float)v1->dx/FRACUNIT;
    v1dy = (float)v1->dy/FRACUNIT;
    v2x = (float)v2->x/FRACUNIT;
    v2y = (float)v2->y/FRACUNIT;
    v2dx = (float)v2->dx/FRACUNIT;
    v2dy = (float)v2->dy/FRACUNIT;
	
    den = v1dy*v2dx - v1dx*v2dy;

    if (den == 0)
	return 0;	// parallel
    
    num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
    frac = num / den;

    return frac*FRACUNIT;
#endif
}


//
// P_LineOpening
// Sets Globals::g->opentop and Globals::g->openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//


void P_LineOpening (line_t* maputil_linedef)
{
    sector_t*	front;
    sector_t*	back;
	
    if (maputil_linedef->sidenum[1] == -1)
    {
	// single sided line
	Globals::g->openrange = 0;
	return;
    }
	 
    front = maputil_linedef->frontsector;
    back = maputil_linedef->backsector;
	
    if (front->ceilingheight < back->ceilingheight)
	Globals::g->opentop = front->ceilingheight;
    else
	Globals::g->opentop = back->ceilingheight;

    if (front->floorheight > back->floorheight)
    {
	Globals::g->openbottom = front->floorheight;
	Globals::g->lowfloor = back->floorheight;
    }
    else
    {
	Globals::g->openbottom = back->floorheight;
	Globals::g->lowfloor = front->floorheight;
    }
	
    Globals::g->openrange = Globals::g->opentop - Globals::g->openbottom;
}


//
// THING POSITION SETTING
//


//
// P_UnsetThingPosition
// Unlinks a thing from block map and Globals::g->sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition (mobj_t* thing)
{
    int		blockx;
    int		blocky;

    if ( ! (thing->flags & MF_NOSECTOR) )
    {
	// inert things don't need to be in blockmap?
	// unlink from subsector
	if (thing->snext)
	    thing->snext->sprev = thing->sprev;

	if (thing->sprev)
	    thing->sprev->snext = thing->snext;
	else
	    thing->subsector->sector->thinglist = thing->snext;
    }
	
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
	// inert things don't need to be in Globals::g->blockmap
	// unlink from block map
	if (thing->bnext)
	    thing->bnext->bprev = thing->bprev;
	
	if (thing->bprev)
	    thing->bprev->bnext = thing->bnext;
	else
	{
	    blockx = (thing->x - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
	    blocky = (thing->y - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;

	    if (blockx>=0 && blockx < Globals::g->bmapwidth
		&& blocky>=0 && blocky < Globals::g->bmapheight)
	    {
		Globals::g->blocklinks[blocky*Globals::g->bmapwidth+blockx] = thing->bnext;
	    }
	}
    }
}


//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void
P_SetThingPosition (mobj_t* thing)
{
    subsector_t*	ss;
    sector_t*		sec;
    int			blockx;
    int			blocky;
    mobj_t**		link;

    
    // link into subsector
    ss = R_PointInSubsector (thing->x,thing->y);
    thing->subsector = ss;
    
    if ( ! (thing->flags & MF_NOSECTOR) )
    {
	// invisible things don't go into the sector links
	sec = ss->sector;
	
	thing->sprev = NULL;
	thing->snext = sec->thinglist;

	if (sec->thinglist)
	    sec->thinglist->sprev = thing;

	sec->thinglist = thing;
    }

    
    // link into Globals::g->blockmap
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
	// inert things don't need to be in Globals::g->blockmap		
	blockx = (thing->x - Globals::g->bmaporgx)>>MAPBLOCKSHIFT;
	blocky = (thing->y - Globals::g->bmaporgy)>>MAPBLOCKSHIFT;

	if (blockx>=0
	    && blockx < Globals::g->bmapwidth
	    && blocky>=0
	    && blocky < Globals::g->bmapheight)
	{
	    link = &Globals::g->blocklinks[blocky*Globals::g->bmapwidth+blockx];
	    thing->bprev = NULL;
	    thing->bnext = *link;
	    if (*link)
		(*link)->bprev = thing;

	    *link = thing;
	}
	else
	{
	    // thing is off the map
	    thing->bnext = thing->bprev = NULL;
	}
    }
}



//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The Globals::g->validcount flags are used to avoid checking Globals::g->lines
// that are marked in multiple mapblocks,
// so increment Globals::g->validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
qboolean
P_BlockLinesIterator
( int			x,
  int			y,
  qboolean(*func)(line_t*) )
{
    int			offset;
    short*		list;
    line_t*		ld;
	
    if (x<0
	|| y<0
	|| x>=Globals::g->bmapwidth
	|| y>=Globals::g->bmapheight)
    {
	return true;
    }
    
    offset = y*Globals::g->bmapwidth+x;
	
    offset = *(Globals::g->blockmap+offset);

    for ( list = Globals::g->blockmaplump+offset ; *list != -1 ; list++)
    {
	ld = &Globals::g->lines[*list];

	if (ld->validcount == Globals::g->validcount)
	    continue; 	// line has already been checked

	ld->validcount = Globals::g->validcount;

	if ( !func(ld) )
	    return false;
    }
    return true;	// everything was checked
}


//
// P_BlockThingsIterator
//
qboolean
P_BlockThingsIterator
( int			x,
  int			y,
  qboolean(*func)(mobj_t*) )
{
    mobj_t*		mobj;
	
    if ( x<0
	 || y<0
	 || x>=Globals::g->bmapwidth
	 || y>=Globals::g->bmapheight)
    {
	return true;
    }
    

    for (mobj = Globals::g->blocklinks[y*Globals::g->bmapwidth+x] ;
	 mobj ;
	 mobj = mobj->bnext)
    {
	if (!func( mobj ) )
	    return false;
    }
    return true;
}



//
// INTERCEPT ROUTINES
//


//
// PIT_AddLineIntercepts.
// Looks for Globals::g->lines in the given block
// that intercept the given Globals::g->trace
// to add to the Globals::g->intercepts list.
//
// A line is crossed if its endpoints
// are on opposite Globals::g->sides of the Globals::g->trace.
// Returns true if Globals::g->earlyout and a solid line hit.
//
qboolean
PIT_AddLineIntercepts (line_t* ld)
{
    int			s1;
    int			s2;
    fixed_t		frac;
    divline_t		dl;
	
    // avoid precision problems with two routines
    if ( Globals::g->trace.dx > FRACUNIT*16
	 || Globals::g->trace.dy > FRACUNIT*16
	 || Globals::g->trace.dx < -FRACUNIT*16
	 || Globals::g->trace.dy < -FRACUNIT*16)
    {
	s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &Globals::g->trace);
	s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &Globals::g->trace);
    }
    else
    {
	s1 = P_PointOnLineSide (Globals::g->trace.x, Globals::g->trace.y, ld);
	s2 = P_PointOnLineSide (Globals::g->trace.x+Globals::g->trace.dx, Globals::g->trace.y+Globals::g->trace.dy, ld);
    }
    
    if (s1 == s2)
	return true;	// line isn't crossed
    
    // hit the line
    P_MakeDivline (ld, &dl);
    frac = P_InterceptVector (&Globals::g->trace, &dl);

    if (frac < 0)
	return true;	// behind source
	
    // try to early out the check
    if (Globals::g->earlyout
	&& frac < FRACUNIT
	&& !ld->backsector)
    {
	return false;	// stop checking
    }
    
	
    Globals::g->intercept_p->frac = frac;
    Globals::g->intercept_p->isaline = true;
    Globals::g->intercept_p->d.line = ld;
    Globals::g->intercept_p++;

    return true;	// continue
}



//
// PIT_AddThingIntercepts
//
qboolean PIT_AddThingIntercepts (mobj_t* thing)
{
    fixed_t		x1;
    fixed_t		y1;
    fixed_t		x2;
    fixed_t		y2;
    
    int			s1;
    int			s2;
    
    qboolean		tracepositive;

    divline_t		dl;
    
    fixed_t		frac;
	
    tracepositive = (Globals::g->trace.dx ^ Globals::g->trace.dy)>0;
		
    // check a corner to corner crossection for hit
    if (tracepositive)
    {
	x1 = thing->x - thing->radius;
	y1 = thing->y + thing->radius;
		
	x2 = thing->x + thing->radius;
	y2 = thing->y - thing->radius;			
    }
    else
    {
	x1 = thing->x - thing->radius;
	y1 = thing->y - thing->radius;
		
	x2 = thing->x + thing->radius;
	y2 = thing->y + thing->radius;			
    }
    
    s1 = P_PointOnDivlineSide (x1, y1, &Globals::g->trace);
    s2 = P_PointOnDivlineSide (x2, y2, &Globals::g->trace);

    if (s1 == s2)
	return true;		// line isn't crossed
	
    dl.x = x1;
    dl.y = y1;
    dl.dx = x2-x1;
    dl.dy = y2-y1;
    
    frac = P_InterceptVector (&Globals::g->trace, &dl);

    if (frac < 0)
	return true;		// behind source

    Globals::g->intercept_p->frac = frac;
    Globals::g->intercept_p->isaline = false;
    Globals::g->intercept_p->d.thing = thing;
    Globals::g->intercept_p++;

    return true;		// keep going
}


//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all Globals::g->lines.
// 
qboolean
P_TraverseIntercepts
( traverser_t	func,
  fixed_t	maxfrac )
{
    int			count;
    fixed_t		dist;
    intercept_t*	scan;
    intercept_t*	in;
	
    count = Globals::g->intercept_p - Globals::g->intercepts;
    
    in = 0;			// shut up compiler warning
	
    while (count--)
    {
        dist = std::numeric_limits<int>::max();
	for (scan = Globals::g->intercepts ; scan < Globals::g->intercept_p ; scan++)
	{
	    if (scan->frac < dist)
	    {
		dist = scan->frac;
		in = scan;
	    }
	}
	
	if (dist > maxfrac)
	    return true;	// checked everything in range		

#if 0  // UNUSED
    {
	// don't check these yet, there may be others inserted
	in = scan = Globals::g->intercepts;
	for ( scan = Globals::g->intercepts ; scan<Globals::g->intercept_p ; scan++)
	    if (scan->frac > maxfrac)
		*in++ = *scan;
	Globals::g->intercept_p = in;
	return false;
    }
#endif

        if ( !func (in) )
	    return false;	// don't bother going farther

        in->frac = std::numeric_limits<int>::max();
    }
	
    return true;		// everything was traversed
}




//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all Globals::g->lines.
//
qboolean
P_PathTraverse
( fixed_t		x1,
  fixed_t		y1,
  fixed_t		x2,
  fixed_t		y2,
  int			flags,
  qboolean (*trav) (intercept_t *))
{
    fixed_t	xt1;
    fixed_t	yt1;
    fixed_t	xt2;
    fixed_t	yt2;
    
    fixed_t	xstep;
    fixed_t	ystep;
    
    fixed_t	partial;
    
    fixed_t	xintercept;
    fixed_t	yintercept;
    
    int		mapx;
    int		mapy;
    
    int		mapxstep;
    int		mapystep;

    int		count;
		
    Globals::g->earlyout = flags & PT_EARLYOUT;
		
    Globals::g->validcount++;
    Globals::g->intercept_p = Globals::g->intercepts;
	
    if ( ((x1-Globals::g->bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
	x1 += FRACUNIT;	// don't side exactly on a line
    
    if ( ((y1-Globals::g->bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
	y1 += FRACUNIT;	// don't side exactly on a line

    Globals::g->trace.x = x1;
    Globals::g->trace.y = y1;
    Globals::g->trace.dx = x2 - x1;
    Globals::g->trace.dy = y2 - y1;

    x1 -= Globals::g->bmaporgx;
    y1 -= Globals::g->bmaporgy;
    xt1 = x1>>MAPBLOCKSHIFT;
    yt1 = y1>>MAPBLOCKSHIFT;

    x2 -= Globals::g->bmaporgx;
    y2 -= Globals::g->bmaporgy;
    xt2 = x2>>MAPBLOCKSHIFT;
    yt2 = y2>>MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
	mapxstep = 1;
	partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
	ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else if (xt2 < xt1)
    {
	mapxstep = -1;
	partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
	ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else
    {
	mapxstep = 0;
	partial = FRACUNIT;
	ystep = 256*FRACUNIT;
    }	

    yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);

	
    if (yt2 > yt1)
    {
	mapystep = 1;
	partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
	xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else if (yt2 < yt1)
    {
	mapystep = -1;
	partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
	xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else
    {
	mapystep = 0;
	partial = FRACUNIT;
	xstep = 256*FRACUNIT;
    }	
    xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);
    
    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;
	
    for (count = 0 ; count < 64 ; count++)
    {
	if (flags & PT_ADDLINES)
	{
	    if (!P_BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
		return false;	// early out
	}
	
	if (flags & PT_ADDTHINGS)
	{
	    if (!P_BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
		return false;	// early out
	}
		
	if (mapx == xt2
	    && mapy == yt2)
	{
	    break;
	}
	
	if ( (yintercept >> FRACBITS) == mapy)
	{
	    yintercept += ystep;
	    mapx += mapxstep;
	}
	else if ( (xintercept >> FRACBITS) == mapx)
	{
	    xintercept += xstep;
	    mapy += mapystep;
	}
		
    }
    // go through the sorted list
    return P_TraverseIntercepts ( trav, FRACUNIT );
}




