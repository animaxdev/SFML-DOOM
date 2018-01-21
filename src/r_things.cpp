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


#include <stdio.h>
#include <stdlib.h>


#include "doomdef.hpp"
#include "m_swap.hpp"

#include "i_system.hpp"
#include "z_zone.hpp"
#include "w_wad.hpp"

#include "r_local.hpp"

#include "doomstat.hpp"




//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);






//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//


// constant arrays
//  used for psprite clipping and initializing clipping


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t Globals::g->sprites patches






//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void
R_InstallSpriteLump
( int		lump,
  unsigned	frame,
  unsigned	rotation,
  qboolean	flipped )
{
    int		r;
	
    if (frame >= 29 || rotation > 8)
	I_Error("R_InstallSpriteLump: "
		"Bad frame characters in lump %i", lump);
	
    if ((int)frame > Globals::g->maxframe)
	Globals::g->maxframe = frame;
		
    if (rotation == 0)
    {
	// the lump should be used for all rotations
	if (Globals::g->sprtemp[frame].rotate == false)
	    I_Error ("R_InitSprites: Sprite %s frame %c has "
		"multip rot=0 lump", Globals::g->spritename, 'A'+frame);

	if (Globals::g->sprtemp[frame].rotate == true)
	    I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
		     "and a rot=0 lump", Globals::g->spritename, 'A'+frame);
			
	Globals::g->sprtemp[frame].rotate = false;
	for (r=0 ; r<8 ; r++)
	{
	    Globals::g->sprtemp[frame].lump[r] = lump - Globals::g->firstspritelump;
	    Globals::g->sprtemp[frame].flip[r] = (unsigned char)flipped;
	}
	return;
    }
	
    // the lump is only used for one rotation
    if (Globals::g->sprtemp[frame].rotate == false)
	I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
		 "and a rot=0 lump", Globals::g->spritename, 'A'+frame);
		
    Globals::g->sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;		
    if (Globals::g->sprtemp[frame].lump[rotation] != -1)
	I_Error ("R_InitSprites: Sprite %s : %c : %c "
		 "has two lumps mapped to it",
		 Globals::g->spritename, 'A'+frame, '1'+rotation);
		
    Globals::g->sprtemp[frame].lump[rotation] = lump - Globals::g->firstspritelump;
    Globals::g->sprtemp[frame].flip[rotation] = (unsigned char)flipped;
}




//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped Globals::g->sprites.
// Will report an error if the lumps are inconsistant. 
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs (const char* const* namelist) 
{ 
    const char* const*	check;
    int		i;
    int		l;
    int		intname;
    int		frame;
    int		rotation;
    int		start;
    int		end;
    int		patched;
		
    // count the number of sprite names
    check = namelist;
    while (*check != NULL)
	check++;

    Globals::g->numsprites = check-namelist;
	
    if (!Globals::g->numsprites)
	return;
		
    Globals::g->sprites = (spritedef_t*)DoomLib::Z_Malloc(Globals::g->numsprites *sizeof(*Globals::g->sprites), PU_STATIC, NULL);
	
    start = Globals::g->firstspritelump-1;
    end = Globals::g->lastspritelump+1;
	
    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints
    for (i=0 ; i < Globals::g->numsprites ; i++)
    {
	Globals::g->spritename = namelist[i];
	memset (Globals::g->sprtemp,-1, sizeof(Globals::g->sprtemp));
		
	Globals::g->maxframe = -1;
	intname = *(int *)namelist[i];
	
	// scan the lumps,
	//  filling in the frames for whatever is found
	for (l=start+1 ; l<end ; l++)
	{
	    if (*(int *)lumpinfo[l].name.data() == intname)
	    {
		frame = lumpinfo[l].name[4] - 'A';
		rotation = lumpinfo[l].name[5] - '0';

		if (Globals::g->modifiedgame)
            patched = W_GetNumForName (lumpinfo[l].name.data());
		else
		    patched = l;

		R_InstallSpriteLump (patched, frame, rotation, false);

		if (lumpinfo[l].name[6])
		{
		    frame = lumpinfo[l].name[6] - 'A';
		    rotation = lumpinfo[l].name[7] - '0';
		    R_InstallSpriteLump (l, frame, rotation, true);
		}
	    }
	}
	
	// check the frames that were found for completeness
	if (Globals::g->maxframe == -1)
	{
	    Globals::g->sprites[i].numframes = 0;
	    continue;
	}
		
	Globals::g->maxframe++;
	
	for (frame = 0 ; frame < Globals::g->maxframe ; frame++)
	{
	    switch ((int)Globals::g->sprtemp[frame].rotate)
	    {
	      case -1:
		// no rotations were found for that frame at all
		I_Error ("R_InitSprites: No patches found "
			 "for %s frame %c", namelist[i], frame+'A');
		break;
		
	      case 0:
		// only the first rotation is needed
		break;
			
	      case 1:
		// must have all 8 frames
		for (rotation=0 ; rotation<8 ; rotation++)
		    if (Globals::g->sprtemp[frame].lump[rotation] == -1)
			I_Error ("R_InitSprites: Sprite %s frame %c "
				 "is missing rotations",
				 namelist[i], frame+'A');
		break;
	    }
	}
	
	// allocate space for the frames present and copy Globals::g->sprtemp to it
	Globals::g->sprites[i].numframes = Globals::g->maxframe;
	Globals::g->sprites[i].spriteframes = 
	    (spriteframe_t*)DoomLib::Z_Malloc (Globals::g->maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (Globals::g->sprites[i].spriteframes, Globals::g->sprtemp, Globals::g->maxframe*sizeof(spriteframe_t));
    }

}




//
// GAME FUNCTIONS
//



//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (const char* const* namelist)
{
    int		i;
	
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	Globals::g->negonearray[i] = -1;
    }
	
    R_InitSpriteDefs (namelist);
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (void)
{
    Globals::g->vissprite_p = Globals::g->vissprites;
}


//
// R_NewVisSprite
//

vissprite_t* R_NewVisSprite (void)
{
    if (Globals::g->vissprite_p == &Globals::g->vissprites[MAXVISSPRITES])
	return &Globals::g->overflowsprite;
    
    Globals::g->vissprite_p++;
    return Globals::g->vissprite_p-1;
}



//
// R_DrawMaskedColumn
// Used for Globals::g->sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//


void R_DrawMaskedColumn (postColumn_t* column)
{
    int		topscreen;
    int 	bottomscreen;
    fixed_t	basetexturemid;
	
    basetexturemid = Globals::g->dc_texturemid;
	
    for ( ; column->topdelta != 0xff ; ) 
    {
	// calculate unclipped screen coordinates
	//  for post
	topscreen = Globals::g->sprtopscreen + Globals::g->spryscale*column->topdelta;
	bottomscreen = topscreen + Globals::g->spryscale*column->length;

	Globals::g->dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
	Globals::g->dc_yh = (bottomscreen-1)>>FRACBITS;
		
	if (Globals::g->dc_yh >= Globals::g->mfloorclip[Globals::g->dc_x])
	    Globals::g->dc_yh = Globals::g->mfloorclip[Globals::g->dc_x]-1;
	if (Globals::g->dc_yl <= Globals::g->mceilingclip[Globals::g->dc_x])
	    Globals::g->dc_yl = Globals::g->mceilingclip[Globals::g->dc_x]+1;

	if (Globals::g->dc_yl <= Globals::g->dc_yh)
	{
	    Globals::g->dc_source = (unsigned char *)column + 3;
	    Globals::g->dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
	    // Globals::g->dc_source = (unsigned char *)column + 3 - column->topdelta;

	    // Drawn by either R_DrawColumn
	    //  or (SHADOW) R_DrawFuzzColumn.
	    colfunc ( Globals::g->dc_colormap, Globals::g->dc_source );	
	}
	column = (postColumn_t *)(  (unsigned char *)column + column->length + 4);
    }
	
    Globals::g->dc_texturemid = basetexturemid;
}



//
// R_DrawVisSprite
//  Globals::g->mfloorclip and Globals::g->mceilingclip should also be set.
//
void
R_DrawVisSprite
( vissprite_t*		vis,
  int			x1,
  int			x2 )
{
    postColumn_t*		column;
    int			texturecolumn;
    fixed_t		frac;
    patch_t*		patch;
	
	
    patch = (patch_t*)W_CacheLumpNum (vis->patch+Globals::g->firstspritelump, PU_CACHE_SHARED);

    Globals::g->dc_colormap = vis->colormap;
    
    if (!Globals::g->dc_colormap)
    {
	// NULL colormap = shadow draw
	colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
	colfunc = R_DrawTranslatedColumn;
	Globals::g->dc_translation = Globals::g->translationtables - 256 +
	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
	
    Globals::g->dc_iscale = abs(vis->xiscale)>>Globals::g->detailshift;
    Globals::g->dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    Globals::g->spryscale = vis->scale;
    Globals::g->sprtopscreen = Globals::g->centeryfrac - FixedMul(Globals::g->dc_texturemid,Globals::g->spryscale);

    for (Globals::g->dc_x=vis->x1 ; Globals::g->dc_x<=vis->x2 ; Globals::g->dc_x++, frac += vis->xiscale)
    {
	texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
	if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
	    I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
	column = (postColumn_t *) ((unsigned char *)patch +
			       LONG(patch->columnofs[texturecolumn]));
	R_DrawMaskedColumn (column);
    }

    colfunc = basecolfunc;
}







//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite (mobj_t* thing)
{
    fixed_t		tr_x;
    fixed_t		tr_y;
    
    fixed_t		gxt;
    fixed_t		gyt;
    
    fixed_t		tx;
    fixed_t		tz;

    fixed_t		xscale;
    
    int			x1;
    int			x2;

    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    
    unsigned		rot;
    qboolean		flip;
    
    int			index;

    vissprite_t*	vis;
    
    angle_t		ang;
    fixed_t		iscale;
    
    // transform the origin point
	extern fixed_t GetViewX(); extern fixed_t GetViewY();
    tr_x = thing->x - GetViewX();
    tr_y = thing->y - GetViewY();
	
    gxt = FixedMul(tr_x,Globals::g->viewcos); 
    gyt = -FixedMul(tr_y,Globals::g->viewsin);
    
    tz = gxt-gyt; 

    // thing is behind view plane?
    if (tz < MINZ)
	return;
    
    xscale = FixedDiv(Globals::g->projection, tz);
	
    gxt = -FixedMul(tr_x,Globals::g->viewsin); 
    gyt = FixedMul(tr_y,Globals::g->viewcos); 
    tx = -(gyt+gxt); 

    // too far off the side?
    if (abs(tx)>(tz<<2))
	return;
    
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if (thing->sprite >= Globals::g->numsprites)
	I_Error ("R_ProjectSprite: invalid sprite number %i ",
		 thing->sprite);
#endif
    sprdef = &Globals::g->sprites[thing->sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
		 thing->sprite, thing->frame);
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
	// choose a different rotation based on player view
	ang = R_PointToAngle (thing->x, thing->y);
	rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
	lump = sprframe->lump[rot];
	flip = (qboolean)sprframe->flip[rot];
    }
    else
    {
	// use single rotation for all views
	lump = sprframe->lump[0];
	flip = (qboolean)sprframe->flip[0];
    }
    
    // calculate edges of the shape
    tx -= Globals::g->spriteoffset[lump];	
    x1 = (Globals::g->centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    // off the right side?
    if (x1 > Globals::g->viewwidth)
	return;
    
    tx +=  Globals::g->spritewidth[lump];
    x2 = ((Globals::g->centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
	return;
    
    // store information in a vissprite
    vis = R_NewVisSprite ();
    vis->mobjflags = thing->flags;
    vis->scale = xscale << Globals::g->detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = thing->z + Globals::g->spritetopoffset[lump];
    vis->texturemid = vis->gzt - Globals::g->viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= Globals::g->viewwidth ? Globals::g->viewwidth-1 : x2;	
    iscale = FixedDiv (FRACUNIT, xscale);

    if (flip)
    {
	vis->startfrac = Globals::g->spritewidth[lump]-1;
	vis->xiscale = -iscale;
    }
    else
    {
	vis->startfrac = 0;
	vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);
    vis->patch = lump;
    
    // get light level
    if (thing->flags & MF_SHADOW)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (Globals::g->fixedcolormap)
    {
	// fixed map
	vis->colormap = Globals::g->fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = Globals::g->colormaps;
    }
    
    else
    {
	// diminished light
	index = xscale>>(LIGHTSCALESHIFT-Globals::g->detailshift);

	if (index >= MAXLIGHTSCALE) 
	    index = MAXLIGHTSCALE-1;

	vis->colormap = Globals::g->spritelights[index];
    }	
}




//
// R_AddSprites
// During BSP traversal, this adds Globals::g->sprites by sector.
//
void R_AddSprites (sector_t* sec)
{
    mobj_t*		thing;
    int			lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  Globals::g->subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == Globals::g->validcount)
	return;		

    // Well, now it will be done.
    sec->validcount = Globals::g->validcount;
	
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+Globals::g->extralight;

    if (lightnum < 0)		
	Globals::g->spritelights = Globals::g->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	Globals::g->spritelights = Globals::g->scalelight[LIGHTLEVELS-1];
    else
	Globals::g->spritelights = Globals::g->scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
	R_ProjectSprite (thing);
}


//
// R_DrawPSprite
//
void R_DrawPSprite (pspdef_t* psp)
{
    fixed_t		tx;
    int			x1;
    int			x2;
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int			lump;
    qboolean		flip;
    vissprite_t*	vis;
    vissprite_t		avis;
    
    // decide which patch to use
#ifdef RANGECHECK
    if ( psp->state->sprite >= Globals::g->numsprites)
	I_Error ("R_ProjectSprite: invalid sprite number %i ",
		 psp->state->sprite);
#endif
    sprdef = &Globals::g->sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
	I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
		 psp->state->sprite, psp->state->frame);
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (qboolean)sprframe->flip[0];
    
    // calculate edges of the shape
    tx = psp->sx-160*FRACUNIT;
	
    tx -= Globals::g->spriteoffset[lump];	
    x1 = (Globals::g->centerxfrac + FixedMul (tx,Globals::g->pspritescale) ) >>FRACBITS;

    // off the right side
    if (x1 > Globals::g->viewwidth)
	return;		

    tx +=  Globals::g->spritewidth[lump];
    x2 = ((Globals::g->centerxfrac + FixedMul (tx, Globals::g->pspritescale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
	return;
    
    // store information in a vissprite
    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-Globals::g->spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= Globals::g->viewwidth ? Globals::g->viewwidth-1 : x2;	
    vis->scale = Globals::g->pspritescale << Globals::g->detailshift;
    
    if (flip)
    {
	vis->xiscale = -Globals::g->pspriteiscale;
	vis->startfrac = Globals::g->spritewidth[lump]-1;
    }
    else
    {
	vis->xiscale = Globals::g->pspriteiscale;
	vis->startfrac = 0;
    }
    
    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    if (Globals::g->viewplayer->powers[pw_invisibility] > 4*32
	|| Globals::g->viewplayer->powers[pw_invisibility] & 8)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (Globals::g->fixedcolormap)
    {
	// fixed color
	vis->colormap = Globals::g->fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = Globals::g->colormaps;
    }
    else
    {
	// local light
	vis->colormap = Globals::g->spritelights[MAXLIGHTSCALE-1];
    }
	
    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (void)
{
    int		i;
    int		lightnum;
    pspdef_t*	psp;
    
    // get light level
    lightnum =
	(Globals::g->viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) 
	+Globals::g->extralight;

    if (lightnum < 0)		
	Globals::g->spritelights = Globals::g->scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	Globals::g->spritelights = Globals::g->scalelight[LIGHTLEVELS-1];
    else
	Globals::g->spritelights = Globals::g->scalelight[lightnum];
    
    // clip to screen bounds
    Globals::g->mfloorclip = Globals::g->screenheightarray;
    Globals::g->mceilingclip = Globals::g->negonearray;
    
    // add all active psprites
    for (i=0, psp=Globals::g->viewplayer->psprites;
	 i<NUMPSPRITES;
	 i++,psp++)
    {
	if (psp->state)
	    R_DrawPSprite (psp);
    }
}




//
// R_SortVisSprites
//


void R_SortVisSprites (void)
{
    int			i;
    int			count;
    vissprite_t*	ds = NULL;
    vissprite_t*	best = NULL;
    vissprite_t		unsorted;
    fixed_t		bestscale;

    count = Globals::g->vissprite_p - Globals::g->vissprites;
	
    unsorted.next = unsorted.prev = &unsorted;

    if (!count)
	return;
		
    for (ds=Globals::g->vissprites ; ds < Globals::g->vissprite_p ; ds++)
    {
	ds->next = ds+1;
	ds->prev = ds-1;
    }
    
    Globals::g->vissprites[0].prev = &unsorted;
    unsorted.next = &Globals::g->vissprites[0];
    (Globals::g->vissprite_p-1)->next = &unsorted;
    unsorted.prev = Globals::g->vissprite_p-1;
    
    // pull the Globals::g->vissprites out by scale
    //best = 0;		// shut up the compiler warning
    Globals::g->vsprsortedhead.next = Globals::g->vsprsortedhead.prev = &Globals::g->vsprsortedhead;
    for (i=0 ; i<count ; i++)
    {
	bestscale = std::numeric_limits<int>::max();
	for (ds=unsorted.next ; ds!= &unsorted ; ds=ds->next)
	{
	    if (ds->scale < bestscale)
	    {
		bestscale = ds->scale;
		best = ds;
	    }
	}
	best->next->prev = best->prev;
	best->prev->next = best->next;
	best->next = &Globals::g->vsprsortedhead;
	best->prev = Globals::g->vsprsortedhead.prev;
	Globals::g->vsprsortedhead.prev->next = best;
	Globals::g->vsprsortedhead.prev = best;
    }
}



//
// R_DrawSprite
//
void R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*		ds;
    short		clipbot[SCREENWIDTH];
    short		cliptop[SCREENWIDTH];
    int			x;
    int			r1;
    int			r2;
    fixed_t		scale;
    fixed_t		lowscale;
    int			silhouette;
		
    for (x = spr->x1 ; x<=spr->x2 ; x++)
	clipbot[x] = cliptop[x] = -2;
    
    // Scan Globals::g->drawsegs from end to start for obscuring Globals::g->segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds=Globals::g->ds_p-1 ; ds >= Globals::g->drawsegs ; ds--)
    {
	// determine if the drawseg obscures the sprite
	if (ds->x1 > spr->x2
	    || ds->x2 < spr->x1
	    || (!ds->silhouette
		&& !ds->maskedtexturecol) )
	{
	    // does not cover sprite
	    continue;
	}
			
	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

	if (ds->scale1 > ds->scale2)
	{
	    lowscale = ds->scale2;
	    scale = ds->scale1;
	}
	else
	{
	    lowscale = ds->scale1;
	    scale = ds->scale2;
	}
		
	if (scale < spr->scale
	    || ( lowscale < spr->scale
		 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
	{
	    // masked mid texture?
	    if (ds->maskedtexturecol)	
		R_RenderMaskedSegRange (ds, r1, r2);
	    // seg is behind sprite
	    continue;			
	}

	
	// clip this piece of the sprite
	silhouette = ds->silhouette;
	
	if (spr->gz >= ds->bsilheight)
	    silhouette &= ~SIL_BOTTOM;

	if (spr->gzt <= ds->tsilheight)
	    silhouette &= ~SIL_TOP;
			
	if (silhouette == 1)
	{
	    // bottom sil
	    for (x=r1 ; x<=r2 ; x++)
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
	}
	else if (silhouette == 2)
	{
	    // top sil
	    for (x=r1 ; x<=r2 ; x++)
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	}
	else if (silhouette == 3)
	{
	    // both
	    for (x=r1 ; x<=r2 ; x++)
	    {
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];
	    }
	}
		
    }
    
    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (clipbot[x] == -2)		
	    clipbot[x] = Globals::g->viewheight;

	if (cliptop[x] == -2)
	    cliptop[x] = -1;
    }
		
    Globals::g->mfloorclip = clipbot;
    Globals::g->mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked (void)
{
    vissprite_t*	spr;
    drawseg_t*		ds;
	
    R_SortVisSprites ();

    if (Globals::g->vissprite_p > Globals::g->vissprites)
    {
	// draw all Globals::g->vissprites back to front
	for (spr = Globals::g->vsprsortedhead.next ;
	     spr != &Globals::g->vsprsortedhead ;
	     spr=spr->next)
	{
	    
	    R_DrawSprite (spr);
	}
    }
    
    // render any remaining masked mid textures
    for (ds=Globals::g->ds_p-1 ; ds >= Globals::g->drawsegs ; ds--)
	if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, ds->x1, ds->x2);
    
    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!Globals::g->viewangleoffset)		
	R_DrawPlayerSprites ();
}




