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


#include <math.h>

#include "z_zone.hpp"

#include "m_swap.hpp"
#include "m_bbox.hpp"

#include "g_game.hpp"

#include "i_system.hpp"
#include "w_wad.hpp"

#include "doomdef.hpp"
#include "p_local.hpp"

#include "s_sound.hpp"

#include "doomstat.hpp"


void	P_SpawnMapThing (mapthing_t*	mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//








// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
// offsets in Globals::g->blockmap are from here
// origin of block map
// for thing chains


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//


// Maintain single and multi player starting spots.






//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
	unsigned char*		data;
	int			i;
	mapvertex_t*	ml;
	vertex_t*		li;

	// Determine number of lumps:
	//  total lump length / vertex record length.
	Globals::g->numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

	// Allocate zone memory for buffer.
//	Globals::g->vertexes = (vertex_t*)Z_Malloc (Globals::g->numvertexes*sizeof(vertex_t),PU_LEVEL,0);	
	if (MallocForLump( lump, Globals::g->numvertexes*sizeof(vertex_t ), Globals::g->vertexes, PU_LEVEL_SHARED ))
	{
		// Load data into cache.
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		ml = (mapvertex_t *)data;
		li = Globals::g->vertexes;

		// Copy and convert vertex coordinates,
		// internal representation as fixed.
		for (i=0 ; i < Globals::g->numvertexes ; i++, li++, ml++)
		{
			li->x = SHORT(ml->x)<<FRACBITS;
			li->y = SHORT(ml->y)<<FRACBITS;
		}

		// Free buffer memory.
		Z_Free(data);
	}
}



//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
	unsigned char*		data;
	int			i;
	mapseg_t*		ml;
	seg_t*		li;
	line_t*		ldef;
	int			psetup_linedef;
	int			side;

	Globals::g->numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
//	Globals::g->segs = (seg_t*)Z_Malloc (Globals::g->numsegs*sizeof(seg_t),PU_LEVEL,0);	

	if (MallocForLump( lump, Globals::g->numsegs*sizeof(seg_t), Globals::g->segs, PU_LEVEL_SHARED ))
	{
		memset (Globals::g->segs, 0, Globals::g->numsegs*sizeof(seg_t));
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		ml = (mapseg_t *)data;
		li = Globals::g->segs;
		for (i=0 ; i < Globals::g->numsegs ; i++, li++, ml++)
		{
			li->v1 = &Globals::g->vertexes[SHORT(ml->v1)];
			li->v2 = &Globals::g->vertexes[SHORT(ml->v2)];

			li->angle = (SHORT(ml->angle))<<16;
			li->offset = (SHORT(ml->offset))<<16;
			psetup_linedef = SHORT(ml->linedef);
			ldef = &Globals::g->lines[psetup_linedef];
			li->linedef = ldef;
			side = SHORT(ml->side);
			li->sidedef = &Globals::g->sides[ldef->sidenum[side]];
			li->frontsector = Globals::g->sides[ldef->sidenum[side]].sector;
			if (ldef-> flags & ML_TWOSIDED)
				li->backsector = Globals::g->sides[ldef->sidenum[side^1]].sector;
			else
				li->backsector = 0;
		}

		Z_Free(data);
	}
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
	unsigned char*		data;
	int			i;
	mapsubsector_t*	ms;
	subsector_t*	ss;

	Globals::g->numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);

	if (MallocForLump( lump, Globals::g->numsubsectors*sizeof(subsector_t), Globals::g->subsectors, PU_LEVEL_SHARED ))
	{
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		ms = (mapsubsector_t *)data;
		memset (Globals::g->subsectors,0, Globals::g->numsubsectors*sizeof(subsector_t));
		ss = Globals::g->subsectors;

		for (i=0 ; i < Globals::g->numsubsectors ; i++, ss++, ms++)
		{
			ss->numlines = SHORT(ms->numsegs);
			ss->firstline = SHORT(ms->firstseg);
		}

		Z_Free(data);
	}
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
	unsigned char*		data;
	int			i;
	mapsector_t*	ms;
	sector_t*		ss;

	Globals::g->numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
	
	Globals::g->sectors = (sector_t*)Z_Malloc( Globals::g->numsectors*sizeof(sector_t), PU_LEVEL, NULL );
	memset (Globals::g->sectors, 0, Globals::g->numsectors*sizeof(sector_t));
	data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

	ms = (mapsector_t *)data;
	ss = Globals::g->sectors;
	for (i=0 ; i < Globals::g->numsectors ; i++, ss++, ms++)
	{
		ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
		ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
		ss->floorpic = R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->thinglist = NULL;
	}

	Z_Free(data);

/*
	if (MallocForLump( lump, Globals::g->numsectors*sizeof(sector_t), (void**)&Globals::g->sectors, PU_LEVEL_SHARED ))
	{
		memset (Globals::g->sectors, 0, Globals::g->numsectors*sizeof(sector_t));
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		ms = (mapsector_t *)data;
		ss = Globals::g->sectors;
		for (i=0 ; i < Globals::g->numsectors ; i++, ss++, ms++)
		{
			ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
			ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
			ss->floorpic = R_FlatNumForName(ms->floorpic);
			ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
			ss->lightlevel = SHORT(ms->lightlevel);
			ss->special = SHORT(ms->special);
			ss->tag = SHORT(ms->tag);
			ss->thinglist = NULL;
		}

		DoomLib::Z_Free(data);
	}
*/	
}


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
	unsigned char*	data;
	int		i;
	int		j;
	int		k;
	mapnode_t*	mn;
	node_t*	no;

	Globals::g->numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	if (MallocForLump( lump, Globals::g->numnodes*sizeof(node_t), Globals::g->nodes, PU_LEVEL_SHARED ))
	{
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		mn = (mapnode_t *)data;
		no = Globals::g->nodes;

		for (i=0 ; i < Globals::g->numnodes ; i++, no++, mn++)
		{
			no->x = SHORT(mn->x)<<FRACBITS;
			no->y = SHORT(mn->y)<<FRACBITS;
			no->dx = SHORT(mn->dx)<<FRACBITS;
			no->dy = SHORT(mn->dy)<<FRACBITS;
			for (j=0 ; j<2 ; j++)
			{
				no->children[j] = SHORT(mn->children[j]);
				for (k=0 ; k<4 ; k++)
					no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
			}
		}

		Z_Free(data);
	}
}


//
// P_LoadThings
//
void P_LoadThings (int lump)
{
	unsigned char*		data;
	int			i;
	mapthing_t*		mt;
	int			numthings;
	qboolean		spawn;

	data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME
	numthings = (W_LumpLength (lump) / sizeof(mapthing_t));

	mt = (mapthing_t *)data;
	for (i=0 ; i<numthings ; i++, mt++)
	{
		spawn = true;

		// Do not spawn cool, new monsters if !commercial
		if ( Globals::g->gamemode != commercial)
		{
			switch(mt->type)
			{
			case 68:	// Arachnotron
			case 64:	// Archvile
			case 88:	// Boss Brain
			case 89:	// Boss Shooter
			case 69:	// Hell Knight
			case 67:	// Mancubus
			case 71:	// Pain Elemental
			case 65:	// Former Human Commando
			case 66:	// Revenant
			case 84:	// Wolf SS
				spawn = false;
				break;
			}
		}
		if (spawn == false)
			break;

		// Do spawn all other stuff. 
		mt->x = SHORT(mt->x);
		mt->y = SHORT(mt->y);
		mt->angle = SHORT(mt->angle);
		mt->type = SHORT(mt->type);
		mt->options = SHORT(mt->options);

		P_SpawnMapThing (mt);
	}

	Z_Free(data);
}


//
// P_LoadLineDefs
// Also counts secret Globals::g->lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
	unsigned char*		data;
	int			i;
	maplinedef_t*	mld;
	line_t*		ld;
	vertex_t*		v1;
	vertex_t*		v2;

	Globals::g->numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	if (MallocForLump( lump, Globals::g->numlines*sizeof(line_t), Globals::g->lines, PU_LEVEL_SHARED ))
	{
		memset (Globals::g->lines, 0, Globals::g->numlines*sizeof(line_t));
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		mld = (maplinedef_t *)data;
		ld = Globals::g->lines;
		for (i=0 ; i < Globals::g->numlines ; i++, mld++, ld++)
		{
			ld->flags = SHORT(mld->flags);
			ld->special = SHORT(mld->special);
			ld->tag = SHORT(mld->tag);
			v1 = ld->v1 = &Globals::g->vertexes[SHORT(mld->v1)];
			v2 = ld->v2 = &Globals::g->vertexes[SHORT(mld->v2)];
			ld->dx = v2->x - v1->x;
			ld->dy = v2->y - v1->y;

			if (!ld->dx)
				ld->slopetype = ST_VERTICAL;
			else if (!ld->dy)
				ld->slopetype = ST_HORIZONTAL;
			else
			{
				if (FixedDiv (ld->dy , ld->dx) > 0)
					ld->slopetype = ST_POSITIVE;
				else
					ld->slopetype = ST_NEGATIVE;
			}

			if (v1->x < v2->x)
			{
				ld->bbox[BOXLEFT] = v1->x;
				ld->bbox[BOXRIGHT] = v2->x;
			}
			else
			{
				ld->bbox[BOXLEFT] = v2->x;
				ld->bbox[BOXRIGHT] = v1->x;
			}

			if (v1->y < v2->y)
			{
				ld->bbox[BOXBOTTOM] = v1->y;
				ld->bbox[BOXTOP] = v2->y;
			}
			else
			{
				ld->bbox[BOXBOTTOM] = v2->y;
				ld->bbox[BOXTOP] = v1->y;
			}

			ld->sidenum[0] = SHORT(mld->sidenum[0]);
			ld->sidenum[1] = SHORT(mld->sidenum[1]);

			if (ld->sidenum[0] != -1)
				ld->frontsector = Globals::g->sides[ld->sidenum[0]].sector;
			else
				ld->frontsector = 0;

			if (ld->sidenum[1] != -1)
				ld->backsector = Globals::g->sides[ld->sidenum[1]].sector;
			else
				ld->backsector = 0;
		}

		Z_Free(data);
	}
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
	unsigned char*		data;
	int			i;
	mapsidedef_t*	msd;
	side_t*		sd;

	Globals::g->numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	if (MallocForLump( lump, Globals::g->numsides*sizeof(side_t), Globals::g->sides, PU_LEVEL_SHARED))
	{
		memset (Globals::g->sides, 0, Globals::g->numsides*sizeof(side_t));
		data = (unsigned char*)W_CacheLumpNum (lump,PU_CACHE_SHARED); // ALAN: LOADTIME

		msd = (mapsidedef_t *)data;
		sd = Globals::g->sides;
		for (i=0 ; i < Globals::g->numsides ; i++, msd++, sd++)
		{
			sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
			sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
			sd->toptexture = R_TextureNumForName(msd->toptexture);
			sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
			sd->midtexture = R_TextureNumForName(msd->midtexture);
			sd->sector = &Globals::g->sectors[SHORT(msd->sector)];
		}

		Z_Free(data);
	}
}


//
// P_LoadBlockMap
//
void P_LoadBlockMap (int lump)
{
	int		i;
	int		count;

	bool firstTime = false;
	if (!lumpcache[lump]) {			// SMF - solution for double endian conversion issue
		firstTime = true;
	}

	Globals::g->blockmaplump = (short*)W_CacheLumpNum (lump,PU_LEVEL_SHARED); // ALAN: This is initialized somewhere else as shared...
	Globals::g->blockmap = Globals::g->blockmaplump+4;
	count = W_LumpLength (lump)/2;

	if ( firstTime ) {				// SMF
		for (i=0 ; i<count ; i++)
			Globals::g->blockmaplump[i] = SHORT(Globals::g->blockmaplump[i]);
	}

	Globals::g->bmaporgx = ( Globals::g->blockmaplump[0] )<<FRACBITS;
	Globals::g->bmaporgy = ( Globals::g->blockmaplump[1] )<<FRACBITS;
	Globals::g->bmapwidth = ( Globals::g->blockmaplump[2] );
	Globals::g->bmapheight = ( Globals::g->blockmaplump[3] );

	// clear out mobj chains
	count = sizeof(*Globals::g->blocklinks)* Globals::g->bmapwidth*Globals::g->bmapheight;
	Globals::g->blocklinks = (mobj_t**)Z_Malloc (count,PU_LEVEL, 0);
	memset (Globals::g->blocklinks, 0, count);
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for Globals::g->sectors.
//
void P_GroupLines (void)
{
	line_t**		linebuffer;
	int			i;
	int			j;
	int			total;
	line_t*		li;
	sector_t*		sector;
	subsector_t*	ss;
	seg_t*		seg;
	fixed_t		bbox[4];
	int			block;

	
	// look up sector number for each subsector
	ss = Globals::g->subsectors;
	for (i=0 ; i < Globals::g->numsubsectors ; i++, ss++)
	{
		seg = &Globals::g->segs[ss->firstline];
		ss->sector = seg->sidedef->sector;
	}

	// count number of Globals::g->lines in each sector
	li = Globals::g->lines;
	total = 0;
	for (i=0 ; i < Globals::g->numlines ; i++, li++)
	{
		total++;
		li->frontsector->linecount++;

		if (li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}

	// build line tables for each sector	
	linebuffer = (line_t**)Z_Malloc (total*4, PU_LEVEL, 0);
	sector = Globals::g->sectors;
	for (i=0 ; i < Globals::g->numsectors ; i++, sector++)
	{
		M_ClearBox (bbox);
		sector->lines = linebuffer;
		li = Globals::g->lines;
		for (j=0 ; j < Globals::g->numlines ; j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox (bbox, li->v1->x, li->v1->y);
				M_AddToBox (bbox, li->v2->x, li->v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error ("P_GroupLines: miscounted");

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
		sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

		// adjust bounding box to map blocks
		block = (bbox[BOXTOP]-Globals::g->bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= Globals::g->bmapheight ? Globals::g->bmapheight-1 : block;
		sector->blockbox[BOXTOP]=block;

		block = (bbox[BOXBOTTOM]-Globals::g->bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM]=block;

		block = (bbox[BOXRIGHT]-Globals::g->bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= Globals::g->bmapwidth ? Globals::g->bmapwidth-1 : block;
		sector->blockbox[BOXRIGHT]=block;

		block = (bbox[BOXLEFT]-Globals::g->bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT]=block;
	}

}


//
// P_SetupLevel
//
void
P_SetupLevel
( int		episode,
 int		map,
 int		playermask,
 skill_t	skill)
{
	int		i;
	char	lumpname[9];
	int		lumpnum;

	Globals::g->totalkills = Globals::g->totalitems = Globals::g->totalsecret = Globals::g->wminfo.maxfrags = 0;
	Globals::g->wminfo.partime = 180;
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		Globals::g->players[i].killcount = Globals::g->players[i].secretcount 
			= Globals::g->players[i].itemcount = 0;

		Globals::g->players[i].chainsawKills = 0;
		Globals::g->players[i].berserkKills = 0;
	}

	// Initial height of PointOfView
	// will be set by player think.
	Globals::g->players[Globals::g->consoleplayer].viewz = 1; 

	// Make sure all sounds are stopped before Z_FreeTags.
	S_Start ();			

	Z_FreeTags( PU_LEVEL, PU_PURGELEVEL-1 );

	// UNUSED W_Profile ();
	P_InitThinkers ();

	// if working with a devlopment map, reload it
	// W_Reload ();

	// DHM - NERVE :: Update the cached asset pointers in case the wad files were reloaded
	{
		void ST_loadData(void);
		ST_loadData();

		void HU_Init(void);
		HU_Init();
	}

	// find map name
	if ( Globals::g->gamemode == commercial)
	{
		if (map<10)
			sprintf (lumpname,"map0%i", map);
		else
			sprintf (lumpname,"map%i", map);
	}
	else
	{
		lumpname[0] = 'E';
		lumpname[1] = '0' + episode;
		lumpname[2] = 'M';
		lumpname[3] = '0' + map;
		lumpname[4] = 0;
	}

	lumpnum = W_GetNumForName (lumpname);

	Globals::g->leveltime = 0;

	// note: most of this ordering is important	
	P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
	P_LoadVertexes (lumpnum+ML_VERTEXES);
	P_LoadSectors (lumpnum+ML_SECTORS);
	P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

	P_LoadLineDefs (lumpnum+ML_LINEDEFS);
	P_LoadSubsectors (lumpnum+ML_SSECTORS);
	P_LoadNodes (lumpnum+ML_NODES);
	P_LoadSegs (lumpnum+ML_SEGS);

	Globals::g->rejectmatrix = (unsigned char*)W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);

	P_GroupLines ();

	Globals::g->bodyqueslot = 0;
	Globals::g->deathmatch_p = Globals::g->deathmatchstarts;
	P_LoadThings (lumpnum+ML_THINGS);

	// if Globals::g->deathmatch, randomly spawn the active Globals::g->players
	if (Globals::g->deathmatch)
	{
		for (i=0 ; i<MAXPLAYERS ; i++)
			if (Globals::g->playeringame[i])
			{
				// DHM - Nerve :: In deathmatch, reset every player at match start
				Globals::g->players[i].playerstate = PST_REBORN;

				Globals::g->players[i].mo = NULL;
				G_DeathMatchSpawnPlayer (i);
			}

	}

	// clear special respawning que
	Globals::g->iquehead = Globals::g->iquetail = 0;		

	// set up world state
	P_SpawnSpecials ();

	// build subsector connect matrix
	//	UNUSED P_ConnectSubsectors ();

	// preload graphics
	if (Globals::g->precache)
		R_PrecacheLevel ();
}



//
// P_Init
//
void P_Init (void)
{
	P_InitSwitchList ();
	P_InitPicAnims ();
	R_InitSprites (sprnames);
}




