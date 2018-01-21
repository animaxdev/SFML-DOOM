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

#include "doomdef.hpp"
#include "doomstat.hpp"

#include "r_local.hpp"
#include "r_sky.hpp"


// OPTIMIZE: closed two sided Globals::g->lines as single sided

// True if any of the Globals::g->segs textures might be visible.

// False if the back side is the same plane.



// angle to line origin

//
// regular wall
//










//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned		index;
    postColumn_t*	col;
    int				lightnum;
    int				texnum;
    
    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    Globals::g->curline = ds->curline;
    Globals::g->frontsector = Globals::g->curline->frontsector;
    Globals::g->backsector = Globals::g->curline->backsector;
    texnum = Globals::g->texturetranslation[Globals::g->curline->sidedef->midtexture];
	
    lightnum = (Globals::g->frontsector->lightlevel >> LIGHTSEGSHIFT)+Globals::g->extralight;

    if (Globals::g->curline->v1->y == Globals::g->curline->v2->y)
	lightnum--;
    else if (Globals::g->curline->v1->x == Globals::g->curline->v2->x)
	lightnum++;

    if (lightnum < 0)		
	Globals::g->walllights = Globals::g->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	Globals::g->walllights = Globals::g->scalelight[LIGHTLEVELS-1];
    else
	Globals::g->walllights = Globals::g->scalelight[lightnum];

    Globals::g->maskedtexturecol = ds->maskedtexturecol;

    Globals::g->rw_scalestep = ds->scalestep;		
    Globals::g->spryscale = ds->scale1 + (x1 - ds->x1)*Globals::g->rw_scalestep;
    Globals::g->mfloorclip = ds->sprbottomclip;
    Globals::g->mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (Globals::g->curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	Globals::g->dc_texturemid = Globals::g->frontsector->floorheight > Globals::g->backsector->floorheight
	    ? Globals::g->frontsector->floorheight : Globals::g->backsector->floorheight;
	Globals::g->dc_texturemid = Globals::g->dc_texturemid + Globals::g->s_textureheight[texnum] - Globals::g->viewz;
    }
    else
    {
	Globals::g->dc_texturemid =Globals::g->frontsector->ceilingheight < Globals::g->backsector->ceilingheight
	    ? Globals::g->frontsector->ceilingheight : Globals::g->backsector->ceilingheight;
	Globals::g->dc_texturemid = Globals::g->dc_texturemid - Globals::g->viewz;
    }
    Globals::g->dc_texturemid += Globals::g->curline->sidedef->rowoffset;
			
    if (Globals::g->fixedcolormap)
	Globals::g->dc_colormap = Globals::g->fixedcolormap;
    
    // draw the columns
    for (Globals::g->dc_x = x1 ; Globals::g->dc_x <= x2 ; Globals::g->dc_x++)
    {
	// calculate lighting
	if (Globals::g->maskedtexturecol[Globals::g->dc_x] != SHRT_MAX)
	{
	    if (!Globals::g->fixedcolormap)
	    {
		index = Globals::g->spryscale>>LIGHTSCALESHIFT;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		Globals::g->dc_colormap = Globals::g->walllights[index];
	    }
			
	    Globals::g->sprtopscreen = Globals::g->centeryfrac - FixedMul(Globals::g->dc_texturemid, Globals::g->spryscale);
	    Globals::g->dc_iscale = 0xffffffffu / (unsigned)Globals::g->spryscale;
	    
	    // draw the texture
	    col = (postColumn_t *)( 
		(unsigned char *)R_GetColumn(texnum,Globals::g->maskedtexturecol[Globals::g->dc_x]) -3);
			
	    R_DrawMaskedColumn (col);
	    Globals::g->maskedtexturecol[Globals::g->dc_x] = SHRT_MAX;
	}
	Globals::g->spryscale += Globals::g->rw_scalestep;
    }
	
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

void R_RenderSegLoop (void)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    fixed_t		texturecolumn;
    int			top;
    int			bottom;

    texturecolumn = 0;				// shut up compiler warning
	
    for ( ; Globals::g->rw_x < Globals::g->rw_stopx ; Globals::g->rw_x++)
    {
		// mark floor / ceiling areas
		yl = (Globals::g->topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

		// no space above wall?
		if (yl < Globals::g->ceilingclip[Globals::g->rw_x]+1)
			yl = Globals::g->ceilingclip[Globals::g->rw_x]+1;
		
		if (Globals::g->markceiling)
		{
			top = Globals::g->ceilingclip[Globals::g->rw_x]+1;
			bottom = yl-1;

			if (bottom >= Globals::g->floorclip[Globals::g->rw_x])
				bottom = Globals::g->floorclip[Globals::g->rw_x]-1;

			if (top <= bottom)
			{
				Globals::g->ceilingplane->top[Globals::g->rw_x] = top;
				Globals::g->ceilingplane->bottom[Globals::g->rw_x] = bottom;
			}
		}
		
		yh = Globals::g->bottomfrac>>HEIGHTBITS;

		if (yh >= Globals::g->floorclip[Globals::g->rw_x])
			yh = Globals::g->floorclip[Globals::g->rw_x]-1;

		if (Globals::g->markfloor)
		{
			top = yh+1;
			bottom = Globals::g->floorclip[Globals::g->rw_x]-1;
			if (top <= Globals::g->ceilingclip[Globals::g->rw_x])
				top = Globals::g->ceilingclip[Globals::g->rw_x]+1;
			if (top <= bottom)
			{
				Globals::g->floorplane->top[Globals::g->rw_x] = top;
				Globals::g->floorplane->bottom[Globals::g->rw_x] = bottom;
			}
		}
		
	// texturecolumn and lighting are independent of wall tiers
	if (Globals::g->segtextured)
	{
	    // calculate texture offset
	    angle = (Globals::g->rw_centerangle + Globals::g->xtoviewangle[Globals::g->rw_x])>>ANGLETOFINESHIFT;
	    texturecolumn = Globals::g->rw_offset-FixedMul(finetangent[angle],Globals::g->rw_distance);
	    texturecolumn >>= FRACBITS;
	    // calculate lighting
	    index = Globals::g->rw_scale>>LIGHTSCALESHIFT;

	    if (index >=  MAXLIGHTSCALE )
			index = MAXLIGHTSCALE-1;

	    Globals::g->dc_colormap = Globals::g->walllights[index];
	    Globals::g->dc_x = Globals::g->rw_x;
	    Globals::g->dc_iscale = 0xffffffffu / (unsigned)Globals::g->rw_scale;
	}
	
	// draw the wall tiers
	if (Globals::g->midtexture)
	{
	    // single sided line
	    Globals::g->dc_yl = yl;
	    Globals::g->dc_yh = yh;
	    Globals::g->dc_texturemid = Globals::g->rw_midtexturemid;
	    Globals::g->dc_source = R_GetColumn(Globals::g->midtexture,texturecolumn);
	    colfunc ( Globals::g->dc_colormap, Globals::g->dc_source );
	    Globals::g->ceilingclip[Globals::g->rw_x] = Globals::g->viewheight;
	    Globals::g->floorclip[Globals::g->rw_x] = -1;
	}
	else
	{
	    // two sided line
	    if (Globals::g->toptexture)
	    {
			// top wall
			mid = Globals::g->pixhigh>>HEIGHTBITS;
			Globals::g->pixhigh += Globals::g->pixhighstep;

			if (mid >= Globals::g->floorclip[Globals::g->rw_x])
				mid = Globals::g->floorclip[Globals::g->rw_x]-1;

			if (mid >= yl)
			{
				Globals::g->dc_yl = yl;
				Globals::g->dc_yh = mid;
				Globals::g->dc_texturemid = Globals::g->rw_toptexturemid;
				Globals::g->dc_source = R_GetColumn(Globals::g->toptexture,texturecolumn);
				colfunc ( Globals::g->dc_colormap, Globals::g->dc_source );
				Globals::g->ceilingclip[Globals::g->rw_x] = mid;
			}
			else
				Globals::g->ceilingclip[Globals::g->rw_x] = yl-1;
		}
	    else
	    {
			// no top wall
			if (Globals::g->markceiling)
				Globals::g->ceilingclip[Globals::g->rw_x] = yl-1;
		}
			
	    if (Globals::g->bottomtexture)
	    {
			// bottom wall
			mid = (Globals::g->pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
			Globals::g->pixlow += Globals::g->pixlowstep;

			// no space above wall?
			if (mid <= Globals::g->ceilingclip[Globals::g->rw_x])
				mid = Globals::g->ceilingclip[Globals::g->rw_x]+1;
			
			if (mid <= yh)
			{
				Globals::g->dc_yl = mid;
				Globals::g->dc_yh = yh;
				Globals::g->dc_texturemid = Globals::g->rw_bottomtexturemid;
				Globals::g->dc_source = R_GetColumn(Globals::g->bottomtexture,
							texturecolumn);
				colfunc ( Globals::g->dc_colormap, Globals::g->dc_source );
				Globals::g->floorclip[Globals::g->rw_x] = mid;
			}
			else
				Globals::g->floorclip[Globals::g->rw_x] = yh+1;
	    }
	    else
	    {
			// no bottom wall
			if (Globals::g->markfloor)
				Globals::g->floorclip[Globals::g->rw_x] = yh+1;
			}
			
			if (Globals::g->maskedtexture)
			{
				// save texturecol
				//  for backdrawing of masked mid texture
				Globals::g->maskedtexturecol[Globals::g->rw_x] = texturecolumn;
			}
		}
		
		Globals::g->rw_scale += Globals::g->rw_scalestep;
		Globals::g->topfrac += Globals::g->topstep;
		Globals::g->bottomfrac += Globals::g->bottomstep;
    }
}




//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( int	start,
  int	stop )
{
    fixed_t		hyp;
    fixed_t		sineval;
    angle_t		distangle, offsetangle;
    fixed_t		vtop;
    int			lightnum;

    // don't overflow and crash
    if (Globals::g->ds_p == &Globals::g->drawsegs[MAXDRAWSEGS])
	return;		
		
#ifdef RANGECHECK
    if (start >=Globals::g->viewwidth || start > stop)
	I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    Globals::g->sidedef = Globals::g->curline->sidedef;
    Globals::g->linedef = Globals::g->curline->linedef;

    // mark the segment as visible for auto map
    Globals::g->linedef->flags |= ML_MAPPED;
    
    // calculate Globals::g->rw_distance for scale calculation
    Globals::g->rw_normalangle = Globals::g->curline->angle + ANG90;
	offsetangle = abs((long)(Globals::g->rw_normalangle-Globals::g->rw_angle1));
    
    if (offsetangle > ANG90)
	offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (Globals::g->curline->v1->x, Globals::g->curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    Globals::g->rw_distance = FixedMul (hyp, sineval);
		
	
    Globals::g->ds_p->x1 = Globals::g->rw_x = start;
    Globals::g->ds_p->x2 = stop;
    Globals::g->ds_p->curline = Globals::g->curline;
    Globals::g->rw_stopx = stop+1;
    
    // calculate scale at both ends and step
	extern angle_t GetViewAngle();
    Globals::g->ds_p->scale1 = Globals::g->rw_scale = 
	R_ScaleFromGlobalAngle (GetViewAngle() + Globals::g->xtoviewangle[start]);
    
    if (stop > start )
    {
	Globals::g->ds_p->scale2 = R_ScaleFromGlobalAngle (GetViewAngle() + Globals::g->xtoviewangle[stop]);
	Globals::g->ds_p->scalestep = Globals::g->rw_scalestep = 
	    (Globals::g->ds_p->scale2 - Globals::g->rw_scale) / (stop-start);
    }
    else
    {
	// UNUSED: try to fix the stretched line bug
#if 0
	if (Globals::g->rw_distance < FRACUNIT/2)
	{
	    fixed_t		trx,try;
	    fixed_t		gxt,gyt;

	extern fixed_t GetViewX(); extern fixed_t GetViewY(); 
	    trx = Globals::g->curline->v1->x - GetViewX();
	    try = Globals::g->curline->v1->y - GetVewY();
			
	    gxt = FixedMul(trx,Globals::g->viewcos); 
	    gyt = -FixedMul(try,Globals::g->viewsin); 
	    Globals::g->ds_p->scale1 = FixedDiv(Globals::g->projection, gxt-gyt) << Globals::g->detailshift;
	}
#endif
	Globals::g->ds_p->scale2 = Globals::g->ds_p->scale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    Globals::g->worldtop = Globals::g->frontsector->ceilingheight - Globals::g->viewz;
    Globals::g->worldbottom = Globals::g->frontsector->floorheight - Globals::g->viewz;
	
    Globals::g->midtexture = Globals::g->toptexture = Globals::g->bottomtexture = Globals::g->maskedtexture = 0;
    Globals::g->ds_p->maskedtexturecol = NULL;
	
    if (!Globals::g->backsector)
    {
	// single sided line
	Globals::g->midtexture = Globals::g->texturetranslation[Globals::g->sidedef->midtexture];
	// a single sided line is terminal, so it must mark ends
	Globals::g->markfloor = Globals::g->markceiling = true;
	if (Globals::g->linedef->flags & ML_DONTPEGBOTTOM)
	{
	    vtop = Globals::g->frontsector->floorheight +
		Globals::g->s_textureheight[Globals::g->sidedef->midtexture];
	    // bottom of texture at bottom
	    Globals::g->rw_midtexturemid = vtop - Globals::g->viewz;	
	}
	else
	{
	    // top of texture at top
	    Globals::g->rw_midtexturemid = Globals::g->worldtop;
	}
	Globals::g->rw_midtexturemid += Globals::g->sidedef->rowoffset;

	Globals::g->ds_p->silhouette = SIL_BOTH;
	Globals::g->ds_p->sprtopclip = Globals::g->screenheightarray;
	Globals::g->ds_p->sprbottomclip = Globals::g->negonearray;
	Globals::g->ds_p->bsilheight = std::numeric_limits<int>::max();
	Globals::g->ds_p->tsilheight = std::numeric_limits<int>::min();
    }
    else
    {
	// two sided line
	Globals::g->ds_p->sprtopclip = Globals::g->ds_p->sprbottomclip = NULL;
	Globals::g->ds_p->silhouette = 0;
	
	if (Globals::g->frontsector->floorheight > Globals::g->backsector->floorheight)
	{
	    Globals::g->ds_p->silhouette = SIL_BOTTOM;
	    Globals::g->ds_p->bsilheight = Globals::g->frontsector->floorheight;
	}
	else if (Globals::g->backsector->floorheight > Globals::g->viewz)
	{
	    Globals::g->ds_p->silhouette = SIL_BOTTOM;
	    Globals::g->ds_p->bsilheight = std::numeric_limits<int>::max();
	    // Globals::g->ds_p->sprbottomclip = Globals::g->negonearray;
	}
	
	if (Globals::g->frontsector->ceilingheight < Globals::g->backsector->ceilingheight)
	{
	    Globals::g->ds_p->silhouette |= SIL_TOP;
	    Globals::g->ds_p->tsilheight = Globals::g->frontsector->ceilingheight;
	}
	else if (Globals::g->backsector->ceilingheight < Globals::g->viewz)
	{
	    Globals::g->ds_p->silhouette |= SIL_TOP;
	    Globals::g->ds_p->tsilheight = std::numeric_limits<int>::min();
	    // Globals::g->ds_p->sprtopclip = Globals::g->screenheightarray;
	}
		
	if (Globals::g->backsector->ceilingheight <= Globals::g->frontsector->floorheight)
	{
	    Globals::g->ds_p->sprbottomclip = Globals::g->negonearray;
	    Globals::g->ds_p->bsilheight = std::numeric_limits<int>::max();
	    Globals::g->ds_p->silhouette |= SIL_BOTTOM;
	}
	
	if (Globals::g->backsector->floorheight >= Globals::g->frontsector->ceilingheight)
	{
	    Globals::g->ds_p->sprtopclip = Globals::g->screenheightarray;
	    Globals::g->ds_p->tsilheight = std::numeric_limits<int>::min();
	    Globals::g->ds_p->silhouette |= SIL_TOP;
	}
	
	Globals::g->worldhigh = Globals::g->backsector->ceilingheight - Globals::g->viewz;
	Globals::g->worldlow = Globals::g->backsector->floorheight - Globals::g->viewz;
		
	// hack to allow height changes in outdoor areas
	if (Globals::g->frontsector->ceilingpic == Globals::g->skyflatnum 
	    && Globals::g->backsector->ceilingpic == Globals::g->skyflatnum)
	{
	    Globals::g->worldtop = Globals::g->worldhigh;
	}
	
			
	if (Globals::g->worldlow != Globals::g->worldbottom 
	    || Globals::g->backsector->floorpic != Globals::g->frontsector->floorpic
	    || Globals::g->backsector->lightlevel != Globals::g->frontsector->lightlevel)
	{
	    Globals::g->markfloor = true;
	}
	else
	{
	    // same plane on both Globals::g->sides
	    Globals::g->markfloor = false;
	}
	
			
	if (Globals::g->worldhigh != Globals::g->worldtop 
	    || Globals::g->backsector->ceilingpic != Globals::g->frontsector->ceilingpic
	    || Globals::g->backsector->lightlevel != Globals::g->frontsector->lightlevel)
	{
	    Globals::g->markceiling = true;
	}
	else
	{
	    // same plane on both Globals::g->sides
	    Globals::g->markceiling = false;
	}
	
	if (Globals::g->backsector->ceilingheight <= Globals::g->frontsector->floorheight
	    || Globals::g->backsector->floorheight >= Globals::g->frontsector->ceilingheight)
	{
	    // closed door
	    Globals::g->markceiling = Globals::g->markfloor = true;
	}
	

	if (Globals::g->worldhigh < Globals::g->worldtop)
	{
	    // top texture
	    Globals::g->toptexture = Globals::g->texturetranslation[Globals::g->sidedef->toptexture];
	    if (Globals::g->linedef->flags & ML_DONTPEGTOP)
	    {
		// top of texture at top
		Globals::g->rw_toptexturemid = Globals::g->worldtop;
	    }
	    else
	    {
		vtop =
		    Globals::g->backsector->ceilingheight
		    + Globals::g->s_textureheight[Globals::g->sidedef->toptexture];
		
		// bottom of texture
		Globals::g->rw_toptexturemid = vtop - Globals::g->viewz;	
	    }
	}
	if (Globals::g->worldlow > Globals::g->worldbottom)
	{
	    // bottom texture
	    Globals::g->bottomtexture = Globals::g->texturetranslation[Globals::g->sidedef->bottomtexture];

	    if (Globals::g->linedef->flags & ML_DONTPEGBOTTOM )
	    {
		// bottom of texture at bottom
		// top of texture at top
		Globals::g->rw_bottomtexturemid = Globals::g->worldtop;
	    }
	    else	// top of texture at top
		Globals::g->rw_bottomtexturemid = Globals::g->worldlow;
	}
	Globals::g->rw_toptexturemid += Globals::g->sidedef->rowoffset;
	Globals::g->rw_bottomtexturemid += Globals::g->sidedef->rowoffset;
	
	// allocate space for masked texture tables
	if (Globals::g->sidedef->midtexture)
	{
	    // masked Globals::g->midtexture
	    Globals::g->maskedtexture = true;
	    Globals::g->ds_p->maskedtexturecol = Globals::g->maskedtexturecol = Globals::g->lastopening - Globals::g->rw_x;
	    Globals::g->lastopening += Globals::g->rw_stopx - Globals::g->rw_x;
	}
    }
    
    // calculate Globals::g->rw_offset (only needed for textured Globals::g->lines)
    Globals::g->segtextured = Globals::g->midtexture | Globals::g->toptexture | Globals::g->bottomtexture | Globals::g->maskedtexture;

    if (Globals::g->segtextured)
    {
		offsetangle = Globals::g->rw_normalangle-Globals::g->rw_angle1;
		
		if (offsetangle > ANG180)
			offsetangle = -offsetangle; // ALANHACK UNSIGNED

		if (offsetangle > ANG90)
			offsetangle = ANG90;

		sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
		Globals::g->rw_offset = FixedMul (hyp, sineval);

		if (Globals::g->rw_normalangle-Globals::g->rw_angle1 < ANG180)
			Globals::g->rw_offset = -Globals::g->rw_offset;

		Globals::g->rw_offset += Globals::g->sidedef->textureoffset + Globals::g->curline->offset;
		Globals::g->rw_centerangle = ANG90 + GetViewAngle() - Globals::g->rw_normalangle;
		
		// calculate light table
		//  use different light tables
		//  for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!Globals::g->fixedcolormap)
		{
			lightnum = (Globals::g->frontsector->lightlevel >> LIGHTSEGSHIFT)+Globals::g->extralight;

			if (Globals::g->curline->v1->y == Globals::g->curline->v2->y)
			lightnum--;
			else if (Globals::g->curline->v1->x == Globals::g->curline->v2->x)
			lightnum++;

			if (lightnum < 0)		
			Globals::g->walllights = Globals::g->scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
			Globals::g->walllights = Globals::g->scalelight[LIGHTLEVELS-1];
			else
			Globals::g->walllights = Globals::g->scalelight[lightnum];
		}
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    
  
    if (Globals::g->frontsector->floorheight >= Globals::g->viewz)
    {
	// above view plane
	Globals::g->markfloor = false;
    }
    
    if (Globals::g->frontsector->ceilingheight <= Globals::g->viewz 
	&& Globals::g->frontsector->ceilingpic != Globals::g->skyflatnum)
    {
	// below view plane
	Globals::g->markceiling = false;
    }

    
    // calculate incremental stepping values for texture edges
    Globals::g->worldtop >>= 4;
    Globals::g->worldbottom >>= 4;
	
    Globals::g->topstep = -FixedMul (Globals::g->rw_scalestep, Globals::g->worldtop);
    Globals::g->topfrac = (Globals::g->centeryfrac>>4) - FixedMul (Globals::g->worldtop, Globals::g->rw_scale);

    Globals::g->bottomstep = -FixedMul (Globals::g->rw_scalestep,Globals::g->worldbottom);
    Globals::g->bottomfrac = (Globals::g->centeryfrac>>4) - FixedMul (Globals::g->worldbottom, Globals::g->rw_scale);
	
    if (Globals::g->backsector)
    {	
	Globals::g->worldhigh >>= 4;
	Globals::g->worldlow >>= 4;

	if (Globals::g->worldhigh < Globals::g->worldtop)
	{
	    Globals::g->pixhigh = (Globals::g->centeryfrac>>4) - FixedMul (Globals::g->worldhigh, Globals::g->rw_scale);
	    Globals::g->pixhighstep = -FixedMul (Globals::g->rw_scalestep,Globals::g->worldhigh);
	}
	
	if (Globals::g->worldlow > Globals::g->worldbottom)
	{
	    Globals::g->pixlow = (Globals::g->centeryfrac>>4) - FixedMul (Globals::g->worldlow, Globals::g->rw_scale);
	    Globals::g->pixlowstep = -FixedMul (Globals::g->rw_scalestep,Globals::g->worldlow);
	}
    }
    
    // render it
	 if (Globals::g->markceiling)
		 Globals::g->ceilingplane = R_CheckPlane (Globals::g->ceilingplane, Globals::g->rw_x, Globals::g->rw_stopx-1);

	 if (Globals::g->markfloor)
		 Globals::g->floorplane = R_CheckPlane (Globals::g->floorplane, Globals::g->rw_x, Globals::g->rw_stopx-1);

    R_RenderSegLoop ();

    
    // save sprite clipping info
    if ( ((Globals::g->ds_p->silhouette & SIL_TOP) || Globals::g->maskedtexture)
	 && !Globals::g->ds_p->sprtopclip)
    {
	memcpy (Globals::g->lastopening, Globals::g->ceilingclip+start, 2*(Globals::g->rw_stopx-start));
	Globals::g->ds_p->sprtopclip = Globals::g->lastopening - start;
	Globals::g->lastopening += Globals::g->rw_stopx - start;
    }
    
    if ( ((Globals::g->ds_p->silhouette & SIL_BOTTOM) || Globals::g->maskedtexture)
	 && !Globals::g->ds_p->sprbottomclip)
    {
	memcpy (Globals::g->lastopening, Globals::g->floorclip+start, 2*(Globals::g->rw_stopx-start));
	Globals::g->ds_p->sprbottomclip = Globals::g->lastopening - start;
	Globals::g->lastopening += Globals::g->rw_stopx - start;	
    }

    if (Globals::g->maskedtexture && !(Globals::g->ds_p->silhouette&SIL_TOP))
    {
	Globals::g->ds_p->silhouette |= SIL_TOP;
	Globals::g->ds_p->tsilheight = std::numeric_limits<int>::min();
    }
    if (Globals::g->maskedtexture && !(Globals::g->ds_p->silhouette&SIL_BOTTOM))
    {
	Globals::g->ds_p->silhouette |= SIL_BOTTOM;
	Globals::g->ds_p->bsilheight = std::numeric_limits<int>::min();
    }
    Globals::g->ds_p++;
}


