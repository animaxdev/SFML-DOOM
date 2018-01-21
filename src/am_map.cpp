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
#include <limits>

#include "z_zone.hpp"
#include "doomdef.hpp"
#include "st_stuff.hpp"
#include "p_local.hpp"
#include "w_wad.hpp"

#include "m_cheat.hpp"
#include "i_system.hpp"

// Needs access to LFB.
#include "v_video.hpp"

// State.
#include "doomstat.hpp"
#include "r_state.hpp"

// Data.
#include "dstrings.hpp"

#include "am_map.hpp"

// SFML
#include <SFML/Window.hpp>

// Need this defined
namespace
{
    auto MAXINT = std::numeric_limits<int>::max();
}


// For use if I do walls with outsides/insides

// Automap colors

// drawing stuff



// scale on entry
// how much the automap moves window per tic in frame-Globals::g->buffer coordinates
// moves 140 pixels in 1 second
// how much zoom-in per tic
// goes to 2x in 1 second
// how much zoom-out per tic
// pulls out to 0.5x in 1 second

// translates between frame-Globals::g->buffer and map distances
// translates between frame-Globals::g->buffer and map coordinates

// the following is crap








//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
mline_t player_arrow[] = 
{
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/4 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/4 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
	{ { -R+R/8, 0 }, { -R-R/8, -R/4 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R

#define R ((8*PLAYERRADIUS)/7)
mline_t cheat_player_arrow[] = 
{
	{ { -R+R/8, 0 }, { R, 0 } }, // -----
	{ { R, 0 }, { R-R/2, R/6 } },  // ----->
	{ { R, 0 }, { R-R/2, -R/6 } },
	{ { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
	{ { -R+R/8, 0 }, { -R-R/8, -R/6 } },
	{ { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
	{ { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
	{ { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
	{ { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
	{ { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
	{ { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
	{ { -R/6, -R/6 }, { 0, -R/6 } },
	{ { 0, -R/6 }, { 0, R/4 } },
	{ { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
	{ { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
	{ { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R

#define R (FRACUNIT)
mline_t triangle_guy[] = 
{
	{ { fixed_t(-.867*R), fixed_t(-.5*R) }, { fixed_t(.867*R), fixed_t(-.5*R) } },
	{ { fixed_t(.867*R), fixed_t(-.5*R) } , { 0, R } },
	{ { 0, R }, { fixed_t(-.867*R), fixed_t(-.5*R) } }
};
#undef R

#define R (FRACUNIT)
mline_t thintriangle_guy[] = 
{
	{ { fixed_t(-.5*R), fixed_t(-.7*R) }, { R, 0 } },
	{ { R, 0 }, { fixed_t(-.5*R), fixed_t(.7*R) } },
	{ { fixed_t(-.5*R), fixed_t(.7*R) }, { fixed_t(-.5*R), fixed_t(-.7*R) } }
};
#undef R







// location of window on screen

// size of window on screen




//
// width/height of window on map (map coords)
//

// based on level size


// based on player size



// old stuff for recovery later

// old location used by the Follower routine

// used by MTOF to scale from map-to-frame-Globals::g->buffer coords
// used by FTOM to scale from frame-Globals::g->buffer-to-map coords (=1/Globals::g->scale_mtof)




const unsigned char cheat_amap_seq[] = 
{ 
	0xb2, 0x26, 0x26, 0x2e, 0xff 
};
cheatseq_t cheat_amap = cheatseq_t( cheat_amap_seq, 0 );


//extern unsigned char Globals::g->screens[][SCREENWIDTH*SCREENHEIGHT];



void
V_MarkRect
( int	x,
 int	y,
 int	width,
 int	height );

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void
AM_getIslope
( mline_t*	ml,
 islope_t*	is )
{
	int dx, dy;

	dy = ml->a.y - ml->b.y;
	dx = ml->b.x - ml->a.x;
	if (!dy) is->islp = (dx<0?-MAXINT:MAXINT);
	else is->islp = FixedDiv(dx, dy);
	if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
	else is->slp = FixedDiv(dy, dx);

}

//
//
//
void AM_activateNewScale(void)
{
	Globals::g->m_x += Globals::g->m_w/2;
	Globals::g->m_y += Globals::g->m_h/2;
	Globals::g->m_w = FTOM(Globals::g->f_w);
	Globals::g->m_h = FTOM(Globals::g->f_h);
	Globals::g->m_x -= Globals::g->m_w/2;
	Globals::g->m_y -= Globals::g->m_h/2;
	Globals::g->m_x2 = Globals::g->m_x + Globals::g->m_w;
	Globals::g->m_y2 = Globals::g->m_y + Globals::g->m_h;
}

//
//
//
void AM_saveScaleAndLoc(void)
{
	Globals::g->old_m_x = Globals::g->m_x;
	Globals::g->old_m_y = Globals::g->m_y;
	Globals::g->old_m_w = Globals::g->m_w;
	Globals::g->old_m_h = Globals::g->m_h;
}

//
//
//
void AM_restoreScaleAndLoc(void)
{

	Globals::g->m_w = Globals::g->old_m_w;
	Globals::g->m_h = Globals::g->old_m_h;
	if (!Globals::g->followplayer)
	{
		Globals::g->m_x = Globals::g->old_m_x;
		Globals::g->m_y = Globals::g->old_m_y;
	} else {
		Globals::g->m_x = Globals::g->amap_plr->mo->x - Globals::g->m_w/2;
		Globals::g->m_y = Globals::g->amap_plr->mo->y - Globals::g->m_h/2;
	}
	Globals::g->m_x2 = Globals::g->m_x + Globals::g->m_w;
	Globals::g->m_y2 = Globals::g->m_y + Globals::g->m_h;

	// Change the scaling multipliers
	Globals::g->scale_mtof = FixedDiv(Globals::g->f_w<<FRACBITS, Globals::g->m_w);
	Globals::g->scale_ftom = FixedDiv(FRACUNIT, Globals::g->scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(void)
{
	Globals::g->markpoints[Globals::g->markpointnum].x = Globals::g->m_x + Globals::g->m_w/2;
	Globals::g->markpoints[Globals::g->markpointnum].y = Globals::g->m_y + Globals::g->m_h/2;
	Globals::g->markpointnum = (Globals::g->markpointnum + 1) % AM_NUMMARKPOINTS;

}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(void)
{
	int i;
	fixed_t a;
	fixed_t b;

	Globals::g->min_x = Globals::g->min_y =  MAXINT;
	Globals::g->max_x = Globals::g->max_y = -MAXINT;

	for (i=0; i < Globals::g->numvertexes; i++)
	{
		if (Globals::g->vertexes[i].x < Globals::g->min_x)
			Globals::g->min_x = Globals::g->vertexes[i].x;
		else if (Globals::g->vertexes[i].x > Globals::g->max_x)
			Globals::g->max_x = Globals::g->vertexes[i].x;

		if (Globals::g->vertexes[i].y < Globals::g->min_y)
			Globals::g->min_y = Globals::g->vertexes[i].y;
		else if (Globals::g->vertexes[i].y > Globals::g->max_y)
			Globals::g->max_y = Globals::g->vertexes[i].y;
	}

	Globals::g->max_w = Globals::g->max_x - Globals::g->min_x;
	Globals::g->max_h = Globals::g->max_y - Globals::g->min_y;

	Globals::g->min_w = 2*PLAYERRADIUS; // const? never changed?
	Globals::g->min_h = 2*PLAYERRADIUS;

	a = FixedDiv(Globals::g->f_w<<FRACBITS, Globals::g->max_w);
	b = FixedDiv(Globals::g->f_h<<FRACBITS, Globals::g->max_h);

	Globals::g->min_scale_mtof = a < b ? a : b;
	Globals::g->max_scale_mtof = FixedDiv(Globals::g->f_h<<FRACBITS, 2*PLAYERRADIUS);

}


//
//
//
void AM_changeWindowLoc(void)
{
	if (Globals::g->m_paninc.x || Globals::g->m_paninc.y)
	{
		Globals::g->followplayer = 0;
		Globals::g->f_oldloc.x = MAXINT;
	}

	Globals::g->m_x += Globals::g->m_paninc.x;
	Globals::g->m_y += Globals::g->m_paninc.y;

	if (Globals::g->m_x + Globals::g->m_w/2 > Globals::g->max_x)
		Globals::g->m_x = Globals::g->max_x - Globals::g->m_w/2;
	else if (Globals::g->m_x + Globals::g->m_w/2 < Globals::g->min_x)
		Globals::g->m_x = Globals::g->min_x - Globals::g->m_w/2;

	if (Globals::g->m_y + Globals::g->m_h/2 > Globals::g->max_y)
		Globals::g->m_y = Globals::g->max_y - Globals::g->m_h/2;
	else if (Globals::g->m_y + Globals::g->m_h/2 < Globals::g->min_y)
		Globals::g->m_y = Globals::g->min_y - Globals::g->m_h/2;

	Globals::g->m_x2 = Globals::g->m_x + Globals::g->m_w;
	Globals::g->m_y2 = Globals::g->m_y + Globals::g->m_h;
}


//
//
//
void AM_initVariables(void)
{
	static event_t st_notify = { ev_keyup, AM_MSGENTERED };
	int pnum;

	Globals::g->automapactive = true;
	Globals::g->fb = Globals::g->screens[0];

	Globals::g->f_oldloc.x = MAXINT;
	Globals::g->amclock = 0;
	Globals::g->lightlev = 0;

	Globals::g->m_paninc.x = Globals::g->m_paninc.y = 0;
	Globals::g->ftom_zoommul = FRACUNIT;
	Globals::g->mtof_zoommul = FRACUNIT;

	Globals::g->m_w = FTOM(Globals::g->f_w);
	Globals::g->m_h = FTOM(Globals::g->f_h);

	// find player to center on initially
	if (!Globals::g->playeringame[pnum = Globals::g->consoleplayer])
		for (pnum=0;pnum<MAXPLAYERS;pnum++)
			if (Globals::g->playeringame[pnum])
				break;

	Globals::g->amap_plr = &Globals::g->players[pnum];
	Globals::g->m_x = Globals::g->amap_plr->mo->x - Globals::g->m_w/2;
	Globals::g->m_y = Globals::g->amap_plr->mo->y - Globals::g->m_h/2;
	AM_changeWindowLoc();

	// for saving & restoring
	Globals::g->old_m_x = Globals::g->m_x;
	Globals::g->old_m_y = Globals::g->m_y;
	Globals::g->old_m_w = Globals::g->m_w;
	Globals::g->old_m_h = Globals::g->m_h;

	// inform the status bar of the change
	ST_Responder(&st_notify);

}

//
// 
//
void AM_loadPics(void)
{
	int i;
	char namebuf[9];

	for (i=0;i<10;i++)
	{
		sprintf(namebuf, "AMMNUM%d", i);
		Globals::g->marknums[i] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
	}

}

void AM_unloadPics(void)
{
//	int i;

}

void AM_clearMarks(void)
{
	int i;

	for (i=0;i<AM_NUMMARKPOINTS;i++)
		Globals::g->markpoints[i].x = -1; // means empty
	Globals::g->markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
	Globals::g->leveljuststarted = 0;

	Globals::g->f_x = Globals::g->f_y = 0;
	Globals::g->f_w = Globals::g->finit_width;
	Globals::g->f_h = Globals::g->finit_height;

	AM_clearMarks();

	AM_findMinMaxBoundaries();
	Globals::g->scale_mtof = FixedDiv(Globals::g->min_scale_mtof, (int) (0.7*FRACUNIT));
	if (Globals::g->scale_mtof > Globals::g->max_scale_mtof)
		Globals::g->scale_mtof = Globals::g->min_scale_mtof;
	Globals::g->scale_ftom = FixedDiv(FRACUNIT, Globals::g->scale_mtof);
}




//
//
//
void AM_Stop (void)
{
	static event_t st_notify = { (evtype_t)0, ev_keyup, AM_MSGEXITED };

	AM_unloadPics();
	Globals::g->automapactive = false;
	ST_Responder(&st_notify);
	Globals::g->stopped = true;
}

//
//
//
void AM_Start (void)
{

	if (!Globals::g->stopped) AM_Stop();
	Globals::g->stopped = false;
	if (Globals::g->lastlevel != Globals::g->gamemap || Globals::g->lastepisode != Globals::g->gameepisode)
	{
		AM_LevelInit();
		Globals::g->lastlevel = Globals::g->gamemap;
		Globals::g->lastepisode = Globals::g->gameepisode;
	}
	AM_initVariables();
	AM_loadPics();
}

//
// set the window scale to the maximum size
//
void AM_minOutWindowScale(void)
{
	Globals::g->scale_mtof = Globals::g->min_scale_mtof;
	Globals::g->scale_ftom = FixedDiv(FRACUNIT, Globals::g->scale_mtof);
	AM_activateNewScale();
}

//
// set the window scale to the minimum size
//
void AM_maxOutWindowScale(void)
{
	Globals::g->scale_mtof = Globals::g->max_scale_mtof;
	Globals::g->scale_ftom = FixedDiv(FRACUNIT, Globals::g->scale_mtof);
	AM_activateNewScale();
}


//
// Handle Globals::g->events (user inputs) in automap mode
//
qboolean
AM_Responder
( event_t*	ev )
{

	int rc;
	rc = false;

	if (!Globals::g->automapactive)
	{
		if (ev->type == ev_keydown && ev->data1 == AM_STARTKEY)
		{
			AM_Start ();
			Globals::g->viewactive = false;
			rc = true;
		}
	}

	else if (ev->type == ev_keydown)
	{

		rc = true;
		switch(ev->data1)
		{
		case AM_PANRIGHTKEY: // pan right
			if (!Globals::g->followplayer) Globals::g->m_paninc.x = FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANLEFTKEY: // pan left
			if (!Globals::g->followplayer) Globals::g->m_paninc.x = -FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANUPKEY: // pan up
			if (!Globals::g->followplayer) Globals::g->m_paninc.y = FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_PANDOWNKEY: // pan down
			if (!Globals::g->followplayer) Globals::g->m_paninc.y = -FTOM(F_PANINC);
			else rc = false;
			break;
		case AM_ZOOMOUTKEY: // zoom out
			Globals::g->mtof_zoommul = M_ZOOMOUT;
			Globals::g->ftom_zoommul = M_ZOOMIN;
			break;
		case AM_ZOOMINKEY: // zoom in
			Globals::g->mtof_zoommul = M_ZOOMIN;
			Globals::g->ftom_zoommul = M_ZOOMOUT;
			break;
		case AM_ENDKEY:
			Globals::g->bigstate = 0;
			Globals::g->viewactive = true;
			AM_Stop ();
			break;
		case AM_GOBIGKEY:
			Globals::g->bigstate = !Globals::g->bigstate;
			if (Globals::g->bigstate)
			{
				AM_saveScaleAndLoc();
				AM_minOutWindowScale();
			}
			else AM_restoreScaleAndLoc();
			break;
		case AM_FOLLOWKEY:
			Globals::g->followplayer = !Globals::g->followplayer;
			Globals::g->f_oldloc.x = MAXINT;
			Globals::g->amap_plr->message = Globals::g->followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
			break;
		case AM_GRIDKEY:
			Globals::g->grid = !Globals::g->grid;
			Globals::g->amap_plr->message = Globals::g->grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
			break;
		case AM_MARKKEY:
			sprintf(Globals::g->buffer, "%s %d", AMSTR_MARKEDSPOT, Globals::g->markpointnum);
			Globals::g->amap_plr->message = Globals::g->buffer;
			AM_addMark();
			break;
		case AM_CLEARMARKKEY:
			AM_clearMarks();
			Globals::g->amap_plr->message = AMSTR_MARKSCLEARED;
			break;
		default:
			Globals::g->cheatstate=0;
			rc = false;
		}
		if (!Globals::g->deathmatch && cht_CheckCheat(&cheat_amap, ev->data1))
		{
			rc = false;
			Globals::g->cheating = (Globals::g->cheating+1) % 3;
		}
	}

	else if (ev->type == ev_keyup)
	{
		rc = false;
		switch (ev->data1)
		{
		case AM_PANRIGHTKEY:
			if (!Globals::g->followplayer) Globals::g->m_paninc.x = 0;
			break;
		case AM_PANLEFTKEY:
			if (!Globals::g->followplayer) Globals::g->m_paninc.x = 0;
			break;
		case AM_PANUPKEY:
			if (!Globals::g->followplayer) Globals::g->m_paninc.y = 0;
			break;
		case AM_PANDOWNKEY:
			if (!Globals::g->followplayer) Globals::g->m_paninc.y = 0;
			break;
		case AM_ZOOMOUTKEY:
		case AM_ZOOMINKEY:
			Globals::g->mtof_zoommul = FRACUNIT;
			Globals::g->ftom_zoommul = FRACUNIT;
			break;
		}
	}

	return rc;

}


//
// Zooming
//
void AM_changeWindowScale(void)
{

	// Change the scaling multipliers
	Globals::g->scale_mtof = FixedMul(Globals::g->scale_mtof, Globals::g->mtof_zoommul);
	Globals::g->scale_ftom = FixedDiv(FRACUNIT, Globals::g->scale_mtof);

	if (Globals::g->scale_mtof < Globals::g->min_scale_mtof)
		AM_minOutWindowScale();
	else if (Globals::g->scale_mtof > Globals::g->max_scale_mtof)
		AM_maxOutWindowScale();
	else
		AM_activateNewScale();
}


//
//
//
void AM_doFollowPlayer(void)
{

	if (Globals::g->f_oldloc.x != Globals::g->amap_plr->mo->x || Globals::g->f_oldloc.y != Globals::g->amap_plr->mo->y)
	{
		Globals::g->m_x = FTOM(MTOF(Globals::g->amap_plr->mo->x)) - Globals::g->m_w/2;
		Globals::g->m_y = FTOM(MTOF(Globals::g->amap_plr->mo->y)) - Globals::g->m_h/2;
		Globals::g->m_x2 = Globals::g->m_x + Globals::g->m_w;
		Globals::g->m_y2 = Globals::g->m_y + Globals::g->m_h;
		Globals::g->f_oldloc.x = Globals::g->amap_plr->mo->x;
		Globals::g->f_oldloc.y = Globals::g->amap_plr->mo->y;

		//  Globals::g->m_x = FTOM(MTOF(Globals::g->amap_plr->mo->x - Globals::g->m_w/2));
		//  Globals::g->m_y = FTOM(MTOF(Globals::g->amap_plr->mo->y - Globals::g->m_h/2));
		//  Globals::g->m_x = Globals::g->amap_plr->mo->x - Globals::g->m_w/2;
		//  Globals::g->m_y = Globals::g->amap_plr->mo->y - Globals::g->m_h/2;

	}

}

//
//
//
void AM_updateLightLev(void)
{
	//static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
	const static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };

	// Change light level
	if (Globals::g->amclock>Globals::g->nexttic)
	{
		Globals::g->lightlev = litelevels[Globals::g->litelevelscnt++];
		if (Globals::g->litelevelscnt == sizeof(litelevels)/sizeof(int)) Globals::g->litelevelscnt = 0;
		Globals::g->nexttic = Globals::g->amclock + 6 - (Globals::g->amclock % 6);
	}

}


//
// Updates on Game Tick
//
void AM_Ticker (void)
{

	if (!Globals::g->automapactive)
		return;

	Globals::g->amclock++;

	if (Globals::g->followplayer)
		AM_doFollowPlayer();

	// Change the zoom if necessary
	if (Globals::g->ftom_zoommul != FRACUNIT)
		AM_changeWindowScale();

	// Change x,y location
	if (Globals::g->m_paninc.x || Globals::g->m_paninc.y)
		AM_changeWindowLoc();

	// Update light level
	// AM_updateLightLev();

}


//
// Clear automap frame Globals::g->buffer.
//
void AM_clearFB(int color)
{
	memset(Globals::g->fb, color, Globals::g->f_w*Globals::g->f_h);
}


//
// Automap clipping of Globals::g->lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
qboolean
AM_clipMline
( mline_t*	ml,
 fline_t*	fl )
{
	enum
	{
		LEFT	=1,
		RIGHT	=2,
		BOTTOM	=4,
		TOP	=8
	};

	register	int outcode1 = 0;
	register	int outcode2 = 0;
	register	int outside;

	fpoint_t	tmp = { 0, 0 };
	int		dx;
	int		dy;




	// do trivial rejects and outcodes
	if (ml->a.y > Globals::g->m_y2)
		outcode1 = TOP;
	else if (ml->a.y < Globals::g->m_y)
		outcode1 = BOTTOM;

	if (ml->b.y > Globals::g->m_y2)
		outcode2 = TOP;
	else if (ml->b.y < Globals::g->m_y)
		outcode2 = BOTTOM;

	if (outcode1 & outcode2)
		return false; // trivially outside

	if (ml->a.x < Globals::g->m_x)
		outcode1 |= LEFT;
	else if (ml->a.x > Globals::g->m_x2)
		outcode1 |= RIGHT;

	if (ml->b.x < Globals::g->m_x)
		outcode2 |= LEFT;
	else if (ml->b.x > Globals::g->m_x2)
		outcode2 |= RIGHT;

	if (outcode1 & outcode2)
		return false; // trivially outside

	// transform to frame-Globals::g->buffer coordinates.
	fl->a.x = CXMTOF(ml->a.x);
	fl->a.y = CYMTOF(ml->a.y);
	fl->b.x = CXMTOF(ml->b.x);
	fl->b.y = CYMTOF(ml->b.y);

	DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	DOOUTCODE(outcode2, fl->b.x, fl->b.y);

	if (outcode1 & outcode2)
		return false;

	while (outcode1 | outcode2)
	{
		// may be partially inside box
		// find an outside point
		if (outcode1)
			outside = outcode1;
		else
			outside = outcode2;

		// clip to each side
		if (outside & TOP)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
			tmp.y = 0;
		}
		else if (outside & BOTTOM)
		{
			dy = fl->a.y - fl->b.y;
			dx = fl->b.x - fl->a.x;
			tmp.x = fl->a.x + (dx*(fl->a.y-Globals::g->f_h))/dy;
			tmp.y = Globals::g->f_h-1;
		}
		else if (outside & RIGHT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(Globals::g->f_w-1 - fl->a.x))/dx;
			tmp.x = Globals::g->f_w-1;
		}
		else if (outside & LEFT)
		{
			dy = fl->b.y - fl->a.y;
			dx = fl->b.x - fl->a.x;
			tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
			tmp.x = 0;
		}

		if (outside == outcode1)
		{
			fl->a = tmp;
			DOOUTCODE(outcode1, fl->a.x, fl->a.y);
		}
		else
		{
			fl->b = tmp;
			DOOUTCODE(outcode2, fl->b.x, fl->b.y);
		}

		if (outcode1 & outcode2)
			return false; // trivially outside
	}

	return true;
}
#undef DOOUTCODE


//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void
AM_drawFline
( fline_t*	fl,
 int		color )
{
	register int x;
	register int y;
	register int dx;
	register int dy;
	register int sx;
	register int sy;
	register int ax;
	register int ay;
	register int d;

	static int fuck = 0;

	// For debugging only
	if (      fl->a.x < 0 || fl->a.x >= Globals::g->f_w
		|| fl->a.y < 0 || fl->a.y >= Globals::g->f_h
		|| fl->b.x < 0 || fl->b.x >= Globals::g->f_w
		|| fl->b.y < 0 || fl->b.y >= Globals::g->f_h)
	{
		I_PrintfE("fuck %d \r", fuck++);
		return;
	}


	dx = fl->b.x - fl->a.x;
	ax = 2 * (dx<0 ? -dx : dx);
	sx = dx<0 ? -1 : 1;

	dy = fl->b.y - fl->a.y;
	ay = 2 * (dy<0 ? -dy : dy);
	sy = dy<0 ? -1 : 1;

	x = fl->a.x;
	y = fl->a.y;

	if (ax > ay)
	{
		d = ay - ax/2;
		while (1)
		{
			PUTDOT(x,y,color);
			if (x == fl->b.x) return;
			if (d>=0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay/2;
		while (1)
		{
			PUTDOT(x, y, color);
			if (y == fl->b.y) return;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}


//
// Clip Globals::g->lines, draw visible part sof Globals::g->lines.
//
void
AM_drawMline
( mline_t*	ml,
 int		color )
{
	static fline_t fl;

	if (AM_clipMline(ml, &fl))
		AM_drawFline(&fl, color); // draws it on frame Globals::g->buffer using Globals::g->fb coords
}



//
// Draws flat (floor/ceiling tile) aligned Globals::g->grid Globals::g->lines.
//
void AM_drawGrid(int color)
{
	fixed_t x, y;
	fixed_t start, end;
	mline_t ml;

	// Figure out start of vertical gridlines
	start = Globals::g->m_x;
	if ((start-Globals::g->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
		- ((start-Globals::g->bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
	end = Globals::g->m_x + Globals::g->m_w;

	// draw vertical gridlines
	ml.a.y = Globals::g->m_y;
	ml.b.y = Globals::g->m_y+Globals::g->m_h;
	for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
	{
		ml.a.x = x;
		ml.b.x = x;
		AM_drawMline(&ml, color);
	}

	// Figure out start of horizontal gridlines
	start = Globals::g->m_y;
	if ((start-Globals::g->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
		start += (MAPBLOCKUNITS<<FRACBITS)
		- ((start-Globals::g->bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
	end = Globals::g->m_y + Globals::g->m_h;

	// draw horizontal gridlines
	ml.a.x = Globals::g->m_x;
	ml.b.x = Globals::g->m_x + Globals::g->m_w;
	for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
	{
		ml.a.y = y;
		ml.b.y = y;
		AM_drawMline(&ml, color);
	}

}

//
// Determines visible Globals::g->lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AM_drawWalls(void)
{
	int i;
	static mline_t l;

	for (i = 0; i < Globals::g->numlines; i++)
	{
		l.a.x = Globals::g->lines[i].v1->x;
		l.a.y = Globals::g->lines[i].v1->y;
		l.b.x = Globals::g->lines[i].v2->x;
		l.b.y = Globals::g->lines[i].v2->y;
		if (Globals::g->cheating || (Globals::g->lines[i].flags & ML_MAPPED))
		{
			if ((Globals::g->lines[i].flags & LINE_NEVERSEE) && !Globals::g->cheating)
				continue;
			if (!Globals::g->lines[i].backsector)
			{
				AM_drawMline(&l, WALLCOLORS+Globals::g->lightlev);
			}
			else
			{
				if (Globals::g->lines[i].special == 39)
				{ // teleporters
					AM_drawMline(&l, WALLCOLORS+WALLRANGE/2);
				}
				else if (Globals::g->lines[i].flags & ML_SECRET) // secret door
				{
					if (Globals::g->cheating) AM_drawMline(&l, SECRETWALLCOLORS + Globals::g->lightlev);
					else AM_drawMline(&l, WALLCOLORS+Globals::g->lightlev);
				}
				else if (Globals::g->lines[i].backsector->floorheight
					!= Globals::g->lines[i].frontsector->floorheight) {
						AM_drawMline(&l, FDWALLCOLORS + Globals::g->lightlev); // floor level change
					}
				else if (Globals::g->lines[i].backsector->ceilingheight
					!= Globals::g->lines[i].frontsector->ceilingheight) {
						AM_drawMline(&l, CDWALLCOLORS+Globals::g->lightlev); // ceiling level change
					}
				else if (Globals::g->cheating) {
					AM_drawMline(&l, TSWALLCOLORS+Globals::g->lightlev);
				}
			}
		}
		else if (Globals::g->amap_plr->powers[pw_allmap])
		{
			if (!(Globals::g->lines[i].flags & LINE_NEVERSEE)) AM_drawMline(&l, GRAYS+3);
		}
	}
}


//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void
AM_rotate
( fixed_t*	x,
 fixed_t*	y,
 angle_t	a )
{
	fixed_t tmpx;

	tmpx =
		FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
		- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

	*y   =
		FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
		+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

	*x = tmpx;
}

void
AM_drawLineCharacter
( mline_t*	lineguy,
 int		lineguylines,
 fixed_t	scale,
 angle_t	angle,
 int		color,
 fixed_t	x,
 fixed_t	y )
{
	int		i;
	mline_t	l;

	for (i=0;i<lineguylines;i++)
	{
		l.a.x = lineguy[i].a.x;
		l.a.y = lineguy[i].a.y;

		if (scale)
		{
			l.a.x = FixedMul(scale, l.a.x);
			l.a.y = FixedMul(scale, l.a.y);
		}

		if (angle)
			AM_rotate(&l.a.x, &l.a.y, angle);

		l.a.x += x;
		l.a.y += y;

		l.b.x = lineguy[i].b.x;
		l.b.y = lineguy[i].b.y;

		if (scale)
		{
			l.b.x = FixedMul(scale, l.b.x);
			l.b.y = FixedMul(scale, l.b.y);
		}

		if (angle)
			AM_rotate(&l.b.x, &l.b.y, angle);

		l.b.x += x;
		l.b.y += y;

		AM_drawMline(&l, color);
	}
}

void AM_drawPlayers(void)
{
	int		i;
	player_t*	p;
	static int 	their_colors[] = { GREENS, GRAYS, BROWNS, REDS };
	int		their_color = -1;
	int		color;

	if (!Globals::g->netgame)
	{
		if (Globals::g->cheating)
			AM_drawLineCharacter
			(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
			Globals::g->amap_plr->mo->angle, WHITE, Globals::g->amap_plr->mo->x, Globals::g->amap_plr->mo->y);
		else
			AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, Globals::g->amap_plr->mo->angle,
			WHITE, Globals::g->amap_plr->mo->x, Globals::g->amap_plr->mo->y);
		return;
	}

	for (i=0;i<MAXPLAYERS;i++)
	{
		their_color++;
		p = &Globals::g->players[i];

		if ( (Globals::g->deathmatch && !Globals::g->singledemo) && p != Globals::g->amap_plr)
			continue;

		if (!Globals::g->playeringame[i])
			continue;

		if (p->powers[pw_invisibility])
			color = 246; // *close* to black
		else
			color = their_colors[their_color];

		AM_drawLineCharacter
			(player_arrow, NUMPLYRLINES, 0, p->mo->angle,
			color, p->mo->x, p->mo->y);
	}

}

void
AM_drawThings
( int	colors,
 int 	colorrange)
{
	int		i;
	mobj_t*	t;

	for (i = 0; i < Globals::g->numsectors; i++)
	{
		t = Globals::g->sectors[i].thinglist;
		while (t)
		{
			AM_drawLineCharacter
				(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
				16<<FRACBITS, t->angle, colors+Globals::g->lightlev, t->x, t->y);
			t = t->snext;
		}
	}
}

void AM_drawMarks(void)
{
	int i, fx, fy, w, h;

	for (i=0;i<AM_NUMMARKPOINTS;i++)
	{
		if (Globals::g->markpoints[i].x != -1)
		{
			//      w = SHORT(Globals::g->marknums[i]->width);
			//      h = SHORT(Globals::g->marknums[i]->height);
			w = 5; // because something's wrong with the wad, i guess
			h = 6; // because something's wrong with the wad, i guess
			fx = CXMTOF(Globals::g->markpoints[i].x);
			fy = CYMTOF(Globals::g->markpoints[i].y);
			if (fx >= Globals::g->f_x && fx <= Globals::g->f_w - w && fy >= Globals::g->f_y && fy <= Globals::g->f_h - h)
				V_DrawPatch(fx/GLOBAL_IMAGE_SCALER, fy/GLOBAL_IMAGE_SCALER, FB, Globals::g->marknums[i]);
		}
	}

}

void AM_drawCrosshair(int color)
{
	Globals::g->fb[(Globals::g->f_w*(Globals::g->f_h+1))/2] = color; // single point for now

}

void AM_Drawer (void)
{
	if (!Globals::g->automapactive) return;

	AM_clearFB(BACKGROUND);
	if (Globals::g->grid)
		AM_drawGrid(GRIDCOLORS);
	AM_drawWalls();
	AM_drawPlayers();
	if (Globals::g->cheating==2)
		AM_drawThings(THINGCOLORS, THINGRANGE);
	AM_drawCrosshair(XHAIRCOLORS);

	AM_drawMarks();

	V_MarkRect(Globals::g->f_x, Globals::g->f_y, Globals::g->f_w, Globals::g->f_h);

}

