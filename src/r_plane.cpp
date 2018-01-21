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

#include "i_system.hpp"
#include "z_zone.hpp"
#include "w_wad.hpp"

#include "doomdef.hpp"
#include "doomstat.hpp"

#include "r_local.hpp"
#include "r_sky.hpp"




//
// opening
//

// Here comes the obnoxious "visplane".



//
// Clip values are the solid pixel bounding the range.
//  Globals::g->floorclip starts out SCREENHEIGHT
//  Globals::g->ceilingclip starts out -1
//

//
// Globals::g->spanstart holds the start of a plane span
// initialized to 0 at start
//

//
// texture mapping
//





//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
  // Doh!
}


//
// R_MapPlane
//
// Uses global vars:
//  Globals::g->planeheight
//  Globals::g->ds_source
//  Globals::g->basexscale
//  Globals::g->baseyscale
//  Globals::g->viewx
//  Globals::g->viewy
//
// BASIC PRIMITIVE
//
void
R_MapPlane
( int		y,
  int		x1,
  int		x2 )
{
    angle_t	angle;
    fixed_t	distance;
    fixed_t	length;
    unsigned	index;
	
//#ifdef RANGECHECK
    if ( x2 < x1 || x1<0 || x2>=Globals::g->viewwidth || y>Globals::g->viewheight )
    {
		//I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
		return;
    }
//#endif

    if (Globals::g->planeheight != Globals::g->cachedheight[y])
    {
	Globals::g->cachedheight[y] = Globals::g->planeheight;
	distance = Globals::g->cacheddistance[y] = FixedMul (Globals::g->planeheight, Globals::g->yslope[y]);
	Globals::g->ds_xstep = Globals::g->cachedxstep[y] = FixedMul (distance,Globals::g->basexscale);
	Globals::g->ds_ystep = Globals::g->cachedystep[y] = FixedMul (distance,Globals::g->baseyscale);
    }
    else
    {
	distance = Globals::g->cacheddistance[y];
	Globals::g->ds_xstep = Globals::g->cachedxstep[y];
	Globals::g->ds_ystep = Globals::g->cachedystep[y];
    }
	
	extern angle_t GetViewAngle();
    length = FixedMul (distance,Globals::g->distscale[x1]);
    angle = (GetViewAngle() + Globals::g->xtoviewangle[x1])>>ANGLETOFINESHIFT;
	extern fixed_t GetViewX(); extern fixed_t GetViewY();
    Globals::g->ds_xfrac = GetViewX() + FixedMul(finecosine[angle], length);
    Globals::g->ds_yfrac = -GetViewY() - FixedMul(finesine[angle], length);

    if (Globals::g->fixedcolormap)
	Globals::g->ds_colormap = Globals::g->fixedcolormap;
    else
    {
	index = distance >> LIGHTZSHIFT;
	
	if (index >= MAXLIGHTZ )
	    index = MAXLIGHTZ-1;

	Globals::g->ds_colormap = Globals::g->planezlight[index];
    }
	
    Globals::g->ds_y = y;
    Globals::g->ds_x1 = x1;
    Globals::g->ds_x2 = x2;

    // high or low detail
    spanfunc (
		Globals::g->ds_xfrac,
		Globals::g->ds_yfrac,
		Globals::g->ds_y,
		Globals::g->ds_x1,
		Globals::g->ds_x2,
		Globals::g->ds_xstep,
		Globals::g->ds_ystep,
		Globals::g->ds_colormap,
		Globals::g->ds_source );	
}


//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes (void)
{
    int		i;
    angle_t	angle;
    
    // opening / clipping determination
    for (i=0 ; i < Globals::g->viewwidth ; i++)
    {
	Globals::g->floorclip[i] = Globals::g->viewheight;
	Globals::g->ceilingclip[i] = -1;
    }

	Globals::g->lastvisplane = Globals::g->visplanes;
    Globals::g->lastopening = Globals::g->openings;

    // texture calculation
    memset (Globals::g->cachedheight, 0, sizeof(Globals::g->cachedheight));

    // left to right mapping
	extern angle_t GetViewAngle();
    angle = (GetViewAngle()-ANG90)>>ANGLETOFINESHIFT;
	
    // scale will be unit scale at SCREENWIDTH/2 distance
    Globals::g->basexscale = FixedDiv (finecosine[angle],Globals::g->centerxfrac);
    Globals::g->baseyscale = -FixedDiv (finesine[angle],Globals::g->centerxfrac);
}




//
// R_FindPlane
//
visplane_t* R_FindPlane( fixed_t height, int picnum, int lightlevel ) {
    visplane_t*	check;
	
    if (picnum == Globals::g->skyflatnum) {
		height = 0;			// all skys map together
		lightlevel = 0;
	}
	
	for (check=Globals::g->visplanes; check < Globals::g->lastvisplane; check++) {
		if (height == check->height && picnum == check->picnum && lightlevel == check->lightlevel) {
			break;
		}
	}

	if (check < Globals::g->lastvisplane)
		return check;
		
    //if (Globals::g->lastvisplane - Globals::g->visplanes == MAXVISPLANES)
		//I_Error ("R_FindPlane: no more visplanes");
	if ( Globals::g->lastvisplane - Globals::g->visplanes == MAXVISPLANES ) {
		check = Globals::g->visplanes;
		return check;
	}
		
    Globals::g->lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;

    memset(check->top,0xff,sizeof(check->top));

    return check;
}


//
// R_CheckPlane
//
visplane_t*
R_CheckPlane
( visplane_t*	pl,
  int		start,
  int		stop )
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;
	
	if (start < pl->minx)
	{
		intrl = pl->minx;
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
		intrl = start;
	}

	if (stop > pl->maxx)
	{
		intrh = pl->maxx;
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
		intrh = stop;
	}

	for (x=intrl ; x<= intrh ; x++)
		if (pl->top[x] != 0xffff)
			break;

	if (x > intrh)
	{
		pl->minx = unionl;
		pl->maxx = unionh;

		// use the same one
		return pl;		
	}
	
	if ( Globals::g->lastvisplane - Globals::g->visplanes == MAXVISPLANES ) {
		return pl;
	}

    // make a new visplane
    Globals::g->lastvisplane->height = pl->height;
    Globals::g->lastvisplane->picnum = pl->picnum;
    Globals::g->lastvisplane->lightlevel = pl->lightlevel;
    
    pl = Globals::g->lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    memset(pl->top,0xff,sizeof(pl->top));
		
    return pl;
}


//
// R_MakeSpans
//
void
R_MakeSpans
( int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 )
{
    while (t1 < t2 && t1<=b1)
    {
	R_MapPlane (t1,Globals::g->spanstart[t1],x-1);
	t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
	R_MapPlane (b1,Globals::g->spanstart[b1],x-1);
	b1--;
    }
	
    while (t2 < t1 && t2<=b2)
    {
	Globals::g->spanstart[t2] = x;
	t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
	Globals::g->spanstart[b2] = x;
	b2--;
    }
}



//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes (void)
{
    visplane_t*		pl;
    int			light;
    int			x;
    int			stop;
    int			angle;
				
#ifdef RANGECHECK
    if (Globals::g->ds_p - Globals::g->drawsegs > MAXDRAWSEGS)
	I_Error ("R_DrawPlanes: Globals::g->drawsegs overflow (%i)",
		 Globals::g->ds_p - Globals::g->drawsegs);
    
    if (Globals::g->lastvisplane - Globals::g->visplanes > MAXVISPLANES)
	I_Error ("R_DrawPlanes: visplane overflow (%i)",
		 Globals::g->lastvisplane - Globals::g->visplanes);
    
    if (Globals::g->lastopening - Globals::g->openings > MAXOPENINGS)
	I_Error ("R_DrawPlanes: opening overflow (%i)",
		 Globals::g->lastopening - Globals::g->openings);
#endif

    for (pl = Globals::g->visplanes ; pl < Globals::g->lastvisplane ; pl++)
    {
	if (pl->minx > pl->maxx)
	    continue;

	
	// sky flat
	if (pl->picnum == Globals::g->skyflatnum)
	{
	    Globals::g->dc_iscale = Globals::g->pspriteiscale>>Globals::g->detailshift;
	    
	    // Sky is allways drawn full bright,
	    //  i.e. Globals::g->colormaps[0] is used.
	    // Because of this hack, sky is not affected
	    //  by INVUL inverse mapping.
	    Globals::g->dc_colormap = Globals::g->colormaps;
	    Globals::g->dc_texturemid = Globals::g->skytexturemid;
	    for (x=pl->minx ; x <= pl->maxx ; x++)
	    {
		Globals::g->dc_yl = pl->top[x];
		Globals::g->dc_yh = pl->bottom[x];

		if (Globals::g->dc_yl <= Globals::g->dc_yh)
		{
			extern angle_t GetViewAngle();
		    angle = (GetViewAngle() + Globals::g->xtoviewangle[x])>>ANGLETOSKYSHIFT;
		    Globals::g->dc_x = x;
		    Globals::g->dc_source = R_GetColumn(Globals::g->skytexture, angle);
		    colfunc ( Globals::g->dc_colormap, Globals::g->dc_source );
		}
	    }
	    continue;
	}
	
	// regular flat
	Globals::g->ds_source = (unsigned char*)W_CacheLumpNum(Globals::g->firstflat +
				   Globals::g->flattranslation[pl->picnum],
				   PU_CACHE_SHARED);
	
	Globals::g->planeheight = abs(pl->height-Globals::g->viewz);
	light = (pl->lightlevel >> LIGHTSEGSHIFT)+Globals::g->extralight;

	if (light >= LIGHTLEVELS)
	    light = LIGHTLEVELS-1;

	if (light < 0)
	    light = 0;

	Globals::g->planezlight = Globals::g->zlight[light];

	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;
		
	stop = pl->maxx + 1;

	for (x=pl->minx ; x<= stop ; x++)
	{
	    R_MakeSpans(x,pl->top[x-1],
			pl->bottom[x-1],
			pl->top[x],
			pl->bottom[x]);
	}
    }
}

