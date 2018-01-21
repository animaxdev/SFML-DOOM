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

#include "i_system.hpp"
#include "i_video.hpp"
#include "z_zone.hpp"
#include "m_random.hpp"
#include "w_wad.hpp"

#include "doomdef.hpp"

#include "g_game.hpp"

#include "st_stuff.hpp"
#include "st_lib.hpp"
#include "r_local.hpp"

#include "p_local.hpp"
#include "p_inter.hpp"

#include "am_map.hpp"
#include "m_cheat.hpp"

#include "s_sound.hpp"

// Needs access to LFB.
#include "v_video.hpp"

// State.
#include "doomstat.hpp"

// Data.
#include "dstrings.hpp"
#include "sounds.hpp"

//
// STATUS BAR DATA
//


// Palette indices.
// For damage/bonus red-/gold-shifts
// Radiation suit, green shift.

// N/256*100% probability
//  that the normal face state will change

// For Responder

// Location of status bar


// Should be set to patch width
//  for tall numbers later on

// Number of status Globals::g->faces.









// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.

// HEALTH number pos.

// Weapon pos.

// Frags pos.

// ARMOR number pos.

// Key icon positions.

// Ammunition counter.

// Indicate maximum ammunition.
// Only needed because backpack exists.

// pistol

// shotgun

// chain gun

// missile launcher

// plasma gun

// bfg

// WPNS title

// DETH title

//Incoming messages window location
//UNUSED
// #define ST_MSGTEXTX	   (Globals::g->viewwindowx)
// #define ST_MSGTEXTY	   (Globals::g->viewwindowy+Globals::g->viewheight-18)
// Dimensions given in characters.
// Or shall I say, in lines?


// Width, in characters again.
// Height, in Globals::g->lines. 





// main player in game

// ST_Start() has just been called

// used to execute ST_Init() only once

// lump number for PLAYPAL

// used for timing

// used for making messages go away

// used when in chat 

// States for the intermission


// whether in automap or first-person

// whether left-side main status bar is active

// whether status bar chat is active

// value of Globals::g->st_chat before message popped up

// whether chat window has the cursor on

// !Globals::g->deathmatch

// !Globals::g->deathmatch && Globals::g->st_statusbaron

// !Globals::g->deathmatch

// main bar left

// 0-9, tall numbers

// tall % sign

// 0-9, short, yellow (,different!) numbers

// 3 key-cards, 3 skulls

// face status patches

// face background

// main bar right

// weapon ownership patches

// ready-weapon widget

// in Globals::g->deathmatch only, summary of frags stats

// health widget

// Globals::g->arms background


// weapon ownership widgets

// face status widget

// keycard widgets

// armor widget

// ammo widgets

// max ammo widgets



// number of frags so far in Globals::g->deathmatch

// used to use appopriately pained face

// used for evil grin

// count until face changes

// current face index, used by Globals::g->w_faces

// holds key-type for each key box on bar

// a random number per tick



// Massive bunches of cheat shit
//  to keep it from being easy to figure them out.
// Yeah, right...
const unsigned char	cheat_mus_seq[] =
{
	0xb2, 0x26, 0xb6, 0xae, 0xea, 1, 0, 0, 0xff
};

const unsigned char	cheat_choppers_seq[] =
{
	0xb2, 0x26, 0xe2, 0x32, 0xf6, 0x2a, 0x2a, 0xa6, 0x6a, 0xea, 0xff // id...
};

const unsigned char	cheat_god_seq[] =
{
	0xb2, 0x26, 0x26, 0xaa, 0x26, 0xff  // iddqd
};

const unsigned char	cheat_ammo_seq[] =
{
	0xb2, 0x26, 0xf2, 0x66, 0xa2, 0xff	// idkfa
};

const unsigned char	cheat_ammonokey_seq[] =
{
	0xb2, 0x26, 0x66, 0xa2, 0xff	// idfa
};


// Smashing Pumpkins Into Samml Piles Of Putried Debris. 
const unsigned char	cheat_noclip_seq[] =
{
	0xb2, 0x26, 0xea, 0x2a, 0xb2,	// idspispopd
		0xea, 0x2a, 0xf6, 0x2a, 0x26, 0xff
};

//
const unsigned char	cheat_commercial_noclip_seq[] =
{
	0xb2, 0x26, 0xe2, 0x36, 0xb2, 0x2a, 0xff	// idclip
}; 



const unsigned char	cheat_powerup_seq[7][10] =
{
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6e, 0xff }, 	// beholdv
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xea, 0xff }, 	// beholds
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xb2, 0xff }, 	// beholdi
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6a, 0xff }, 	// beholdr
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xa2, 0xff }, 	// beholda
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x36, 0xff }, 	// beholdl
	{ 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xff }		// behold
};


const unsigned char	cheat_clev_seq[] =
{
	0xb2, 0x26,  0xe2, 0x36, 0xa6, 0x6e, 1, 0, 0, 0xff	// idclev
};


// my position cheat
const unsigned char	cheat_mypos_seq[] =
{
	0xb2, 0x26, 0xb6, 0xba, 0x2a, 0xf6, 0xea, 0xff	// idmypos
}; 


// Now what?
cheatseq_t	cheat_mus = cheatseq_t( cheat_mus_seq, 0 );
cheatseq_t	cheat_god = cheatseq_t( cheat_god_seq, 0 );
cheatseq_t	cheat_ammo = cheatseq_t( cheat_ammo_seq, 0 );
cheatseq_t	cheat_ammonokey = cheatseq_t( cheat_ammonokey_seq, 0 );
cheatseq_t	cheat_noclip = cheatseq_t( cheat_noclip_seq, 0 );
cheatseq_t	cheat_commercial_noclip = cheatseq_t( cheat_commercial_noclip_seq, 0 );

// ALAN

// DISABLED cheatseq_t( cheat_powerup_seq[0], 0 ), cheatseq_t( cheat_powerup_seq[1], 0 ),
// cheatseq_t( cheat_powerup_seq[2], 0 ),
// DISABLED cheatseq_t( cheat_powerup_seq[3], 0 ), 
// cheatseq_t( cheat_powerup_seq[4], 0 ),cheatseq_t( cheat_powerup_seq[5], 0 ),cheatseq_t( cheat_powerup_seq[6], 0 ) };

cheatseq_t	cheat_choppers = cheatseq_t( cheat_choppers_seq, 0 );
cheatseq_t	cheat_clev = cheatseq_t( cheat_clev_seq, 0 );
cheatseq_t	cheat_mypos = cheatseq_t( cheat_mypos_seq, 0 );


// 
const extern char*	mapnames[];


//
// STATUS BAR CODE
//
void ST_Stop(void);

void ST_refreshBackground(void)
{

	if (Globals::g->st_statusbaron)
	{
		V_DrawPatch(ST_X, 0, BG, Globals::g->sbar);

		if (Globals::g->netgame)
			V_DrawPatch(ST_FX, 0, BG, Globals::g->faceback);

		V_CopyRect(ST_X, 0, BG, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, FG);
	}

}


// Respond to keyboard input Globals::g->events,
//  intercept cheats.
qboolean
ST_Responder (event_t* ev)
{
	int		i;

	// Filter automap on/off.
	if (ev->type == ev_keyup
		&& ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
	{
		switch(ev->data1)
		{
		case AM_MSGENTERED:
			Globals::g->st_gamestate = AutomapState;
			Globals::g->st_firsttime = true;
			break;

		case AM_MSGEXITED:
			//	I_PrintfE( "AM exited\n");
			Globals::g->st_gamestate = FirstPersonState;
			break;
		}
	}

	// if a user keypress...
	else if (ev->type == ev_keydown)
	{
		if (!Globals::g->netgame)
		{
			// b. - enabled for more debug fun.
			// if (Globals::g->gameskill != sk_nightmare) {

			// 'dqd' cheat for toggleable god mode
			if (cht_CheckCheat(&cheat_god, ev->data1))
			{
				Globals::g->plyr->cheats ^= CF_GODMODE;
				if (Globals::g->plyr->cheats & CF_GODMODE)
				{
					if (Globals::g->plyr->mo)
						Globals::g->plyr->mo->health = 100;

					Globals::g->plyr->health = 100;
					Globals::g->plyr->message = STSTR_DQDON;
				}
				else 
					Globals::g->plyr->message = STSTR_DQDOFF;
			}
			// 'fa' cheat for killer fucking arsenal
			else if (cht_CheckCheat(&cheat_ammonokey, ev->data1))
			{
				Globals::g->plyr->armorpoints = 200;
				Globals::g->plyr->armortype = 2;

				for (i=0;i<NUMWEAPONS;i++)
					Globals::g->plyr->weaponowned[i] = true;

				for (i=0;i<NUMAMMO;i++)
					Globals::g->plyr->ammo[i] = Globals::g->plyr->maxammo[i];

				Globals::g->plyr->message = STSTR_FAADDED;
			}
			// 'kfa' cheat for key full ammo
			else if (cht_CheckCheat(&cheat_ammo, ev->data1))
			{
				Globals::g->plyr->armorpoints = 200;
				Globals::g->plyr->armortype = 2;

				for (i=0;i<NUMWEAPONS;i++)
					Globals::g->plyr->weaponowned[i] = true;

				for (i=0;i<NUMAMMO;i++)
					Globals::g->plyr->ammo[i] = Globals::g->plyr->maxammo[i];

				for (i=0;i<NUMCARDS;i++)
					Globals::g->plyr->cards[i] = true;

				Globals::g->plyr->message = STSTR_KFAADDED;
			}
			// 'mus' cheat for changing music
			else if (cht_CheckCheat(&cheat_mus, ev->data1))
			{

				char	buf[3];
				int		musnum;

				Globals::g->plyr->message = STSTR_MUS;
				cht_GetParam(&cheat_mus, buf);

				if (Globals::g->gamemode == commercial)
				{
					musnum = mus_runnin + (buf[0]-'0')*10 + buf[1]-'0' - 1;

					if (((buf[0]-'0')*10 + buf[1]-'0') > 35)
						Globals::g->plyr->message = STSTR_NOMUS;
					else
						S_ChangeMusic(musnum, 1);
				}
				else
				{
					musnum = mus_e1m1 + (buf[0]-'1')*9 + (buf[1]-'1');

					if (((buf[0]-'1')*9 + buf[1]-'1') > 31)
						Globals::g->plyr->message = STSTR_NOMUS;
					else
						S_ChangeMusic(musnum, 1);
				}
			}
			// Simplified, accepting both "noclip" and "idspispopd".
			// no clipping mode cheat
			else if ( cht_CheckCheat(&cheat_noclip, ev->data1) 
				|| cht_CheckCheat(&cheat_commercial_noclip,ev->data1) )
			{	
				Globals::g->plyr->cheats ^= CF_NOCLIP;

				if (Globals::g->plyr->cheats & CF_NOCLIP)
					Globals::g->plyr->message = STSTR_NCON;
				else
					Globals::g->plyr->message = STSTR_NCOFF;
			}
			// 'behold?' power-up cheats
			for (i=0;i<6;i++)
			{
				if (cht_CheckCheat(&Globals::g->cheat_powerup[i], ev->data1))
				{
					if (!Globals::g->plyr->powers[i])
						P_GivePower( Globals::g->plyr, i);
					else if (i!=pw_strength)
						Globals::g->plyr->powers[i] = 1;
					else
						Globals::g->plyr->powers[i] = 0;

					Globals::g->plyr->message = STSTR_BEHOLDX;
				}
			}

			// 'behold' power-up menu
			if (cht_CheckCheat(&Globals::g->cheat_powerup[6], ev->data1))
			{
				Globals::g->plyr->message = STSTR_BEHOLD;
			}
			// 'choppers' invulnerability & chainsaw
			else if (cht_CheckCheat(&cheat_choppers, ev->data1))
			{
				Globals::g->plyr->weaponowned[wp_chainsaw] = true;
				Globals::g->plyr->powers[pw_invulnerability] = true;
				Globals::g->plyr->message = STSTR_CHOPPERS;
			}
			// 'mypos' for player position
			else if (cht_CheckCheat(&cheat_mypos, ev->data1))
			{
				static char	buf[ST_MSGWIDTH];
				sprintf(buf, "ang=0x%x;x,y=(0x%x,0x%x)",
					Globals::g->players[Globals::g->consoleplayer].mo->angle,
					Globals::g->players[Globals::g->consoleplayer].mo->x,
					Globals::g->players[Globals::g->consoleplayer].mo->y);
				Globals::g->plyr->message = buf;
			}
		}

		// 'clev' change-level cheat
// ALAN NETWORKING
		if (false) // cht_CheckCheat(&cheat_clev, ev->data1))
		{
			char		buf[3];
			int		epsd;
			int		map;

			cht_GetParam(&cheat_clev, buf);

			if (Globals::g->gamemode == commercial)
			{
				epsd = 0;
				map = (buf[0] - '0')*10 + buf[1] - '0';
			}
			else
			{
				epsd = buf[0] - '0';
				map = buf[1] - '0';
			}

			// Catch invalid maps.
			if (epsd < 1)
				return false;

			if (map < 1)
				return false;

			// Ohmygod - this is not going to work.
			if ((Globals::g->gamemode == retail)
				&& ((epsd > 4) || (map > 9)))
				return false;

			if ((Globals::g->gamemode == registered)
				&& ((epsd > 3) || (map > 9)))
				return false;

			if ((Globals::g->gamemode == shareware)
				&& ((epsd > 1) || (map > 9)))
				return false;

			if ((Globals::g->gamemode == commercial)
				&& (( epsd > 1) || (map > 34)))
				return false;

			// So be it.
			Globals::g->plyr->message = STSTR_CLEV;
			G_DeferedInitNew(Globals::g->gameskill, epsd, map);
		}    
	}
	return false;
}



int ST_calcPainOffset(void)
{
	int		health;

	health = Globals::g->plyr->health > 100 ? 100 : Globals::g->plyr->health;

	if (health != Globals::g->oldhealth)
	{
		Globals::g->lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
		Globals::g->oldhealth = health;
	}
	return Globals::g->lastcalc;
}


//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(void)
{
	int		i;
	angle_t	badguyangle;
	angle_t	diffang;
	qboolean	doevilgrin;

	if (Globals::g->priority < 10)
	{
		// dead
		if (!Globals::g->plyr->health)
		{
			Globals::g->priority = 9;
			Globals::g->st_faceindex = ST_DEADFACE;
			Globals::g->st_facecount = 1;
		}
	}

	if (Globals::g->priority < 9)
	{
		if (Globals::g->plyr->bonuscount)
		{
			// picking up bonus
			doevilgrin = false;

			for (i=0;i<NUMWEAPONS;i++)
			{
				if (Globals::g->oldweaponsowned[i] != Globals::g->plyr->weaponowned[i])
				{
					doevilgrin = true;
					Globals::g->oldweaponsowned[i] = Globals::g->plyr->weaponowned[i];
				}
			}
			if (doevilgrin) 
			{
				// evil grin if just picked up weapon
				Globals::g->priority = 8;
				Globals::g->st_facecount = ST_EVILGRINCOUNT;
				Globals::g->st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
			}
		}

	}

	if (Globals::g->priority < 8)
	{
		if (Globals::g->plyr->damagecount
			&& Globals::g->plyr->attacker
			&& Globals::g->plyr->attacker != Globals::g->plyr->mo)
		{
			// being attacked
			Globals::g->priority = 7;

			if (Globals::g->plyr->health - Globals::g->st_oldhealth > ST_MUCHPAIN)
			{
				Globals::g->st_facecount = ST_TURNCOUNT;
				Globals::g->st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
			}
			else
			{
				badguyangle = R_PointToAngle2(Globals::g->plyr->mo->x,
					Globals::g->plyr->mo->y,
					Globals::g->plyr->attacker->x,
					Globals::g->plyr->attacker->y);

				if (badguyangle > Globals::g->plyr->mo->angle)
				{
					// whether right or left
					diffang = badguyangle - Globals::g->plyr->mo->angle;
					i = diffang > ANG180; 
				}
				else
				{
					// whether left or right
					diffang = Globals::g->plyr->mo->angle - badguyangle;
					i = diffang <= ANG180; 
				} // confusing, aint it?


				Globals::g->st_facecount = ST_TURNCOUNT;
				Globals::g->st_faceindex = ST_calcPainOffset();

				if (diffang < ANG45)
				{
					// head-on    
					Globals::g->st_faceindex += ST_RAMPAGEOFFSET;
				}
				else if (i)
				{
					// turn face right
					Globals::g->st_faceindex += ST_TURNOFFSET;
				}
				else
				{
					// turn face left
					Globals::g->st_faceindex += ST_TURNOFFSET+1;
				}
			}
		}
	}

	if (Globals::g->priority < 7)
	{
		// getting hurt because of your own damn stupidity
		if (Globals::g->plyr->damagecount)
		{
			if (Globals::g->plyr->health - Globals::g->st_oldhealth > ST_MUCHPAIN)
			{
				Globals::g->priority = 7;
				Globals::g->st_facecount = ST_TURNCOUNT;
				Globals::g->st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
			}
			else
			{
				Globals::g->priority = 6;
				Globals::g->st_facecount = ST_TURNCOUNT;
				Globals::g->st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
			}

		}

	}

	if (Globals::g->priority < 6)
	{
		// rapid firing
		if (Globals::g->plyr->attackdown)
		{
			if (Globals::g->lastattackdown==-1)
				Globals::g->lastattackdown = ST_RAMPAGEDELAY;
			else if (!--Globals::g->lastattackdown)
			{
				Globals::g->priority = 5;
				Globals::g->st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
				Globals::g->st_facecount = 1;
				Globals::g->lastattackdown = 1;
			}
		}
		else
			Globals::g->lastattackdown = -1;

	}

	if (Globals::g->priority < 5)
	{
		// invulnerability
		if ((Globals::g->plyr->cheats & CF_GODMODE)
			|| Globals::g->plyr->powers[pw_invulnerability])
		{
			Globals::g->priority = 4;

			Globals::g->st_faceindex = ST_GODFACE;
			Globals::g->st_facecount = 1;

		}

	}

	// look left or look right if the facecount has timed out
	if (!Globals::g->st_facecount)
	{
		Globals::g->st_faceindex = ST_calcPainOffset() + (Globals::g->st_randomnumber % 3);
		Globals::g->st_facecount = ST_STRAIGHTFACECOUNT;
		Globals::g->priority = 0;
	}

	Globals::g->st_facecount--;

}

void ST_updateWidgets(void)
{
	int		i;

	// must redirect the pointer if the ready weapon has changed.
	//  if (Globals::g->w_ready.data != Globals::g->plyr->readyweapon)
	//  {
	if (weaponinfo[Globals::g->plyr->readyweapon].ammo == am_noammo)
		Globals::g->w_ready.num = &Globals::g->largeammo;
	else
		Globals::g->w_ready.num = &Globals::g->plyr->ammo[weaponinfo[Globals::g->plyr->readyweapon].ammo];
	//{
	// static int tic=0;
	// static int dir=-1;
	// if (!(tic&15))
	//   Globals::g->plyr->ammo[weaponinfo[Globals::g->plyr->readyweapon].ammo]+=dir;
	// if (Globals::g->plyr->ammo[weaponinfo[Globals::g->plyr->readyweapon].ammo] == -100)
	//   dir = 1;
	// tic++;
	// }
	Globals::g->w_ready.data = Globals::g->plyr->readyweapon;

	// if (*Globals::g->w_ready.on)
	//  STlib_updateNum(&Globals::g->w_ready, true);
	// refresh weapon change
	//  }

	// update keycard multiple widgets
	for (i=0;i<3;i++)
	{
		Globals::g->keyboxes[i] = Globals::g->plyr->cards[i] ? i : -1;

		if (Globals::g->plyr->cards[i+3])
			Globals::g->keyboxes[i] = i+3;
	}

	// refresh everything if this is him coming back to life
	ST_updateFaceWidget();

	// used by the Globals::g->w_armsbg widget
	Globals::g->st_notdeathmatch = !Globals::g->deathmatch;

	// used by Globals::g->w_arms[] widgets
	Globals::g->st_armson = Globals::g->st_statusbaron && !Globals::g->deathmatch; 

	// used by Globals::g->w_frags widget
	Globals::g->st_fragson = Globals::g->deathmatch && Globals::g->st_statusbaron; 
	Globals::g->st_fragscount = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if (i != Globals::g->consoleplayer)
			Globals::g->st_fragscount += Globals::g->plyr->frags[i];
		else
			Globals::g->st_fragscount -= Globals::g->plyr->frags[i];
	}

	// get rid of chat window if up because of message
	if (!--Globals::g->st_msgcounter)
		Globals::g->st_chat = Globals::g->st_oldchat;

}

void ST_Ticker (void)
{

	Globals::g->st_clock++;
	Globals::g->st_randomnumber = M_Random();
	ST_updateWidgets();
	Globals::g->st_oldhealth = Globals::g->plyr->health;

}


void ST_doPaletteStuff(void)
{

	int		palette;
	unsigned char*	pal;
	int		cnt;
	int		bzc;

	cnt = Globals::g->plyr->damagecount;

	if (Globals::g->plyr->powers[pw_strength])
	{
		// slowly fade the berzerk out
		bzc = 12 - (Globals::g->plyr->powers[pw_strength]>>6);

		if (bzc > cnt)
			cnt = bzc;
	}

	if (cnt)
	{
		palette = (cnt+7)>>3;

		if (palette >= NUMREDPALS)
			palette = NUMREDPALS-1;

		palette += STARTREDPALS;
	}

	else if (Globals::g->plyr->bonuscount)
	{
		palette = (Globals::g->plyr->bonuscount+7)>>3;

		if (palette >= NUMBONUSPALS)
			palette = NUMBONUSPALS-1;

		palette += STARTBONUSPALS;
	}

	else if ( Globals::g->plyr->powers[pw_ironfeet] > 4*32
		|| Globals::g->plyr->powers[pw_ironfeet]&8)
		palette = RADIATIONPAL;
	else
		palette = 0;

	if (palette != Globals::g->st_palette)
	{
		Globals::g->st_palette = palette;
		pal = (unsigned char *) W_CacheLumpNum (Globals::g->lu_palette, PU_CACHE_SHARED)+palette*768;
		I_SetPalette (pal);
	}

}

void ST_drawWidgets(qboolean refresh)
{
	int		i;

	Globals::g->st_notdeathmatch = !Globals::g->deathmatch;

	// used by Globals::g->w_arms[] widgets
	Globals::g->st_armson = Globals::g->st_statusbaron && !Globals::g->deathmatch;

	// used by Globals::g->w_frags widget
	Globals::g->st_fragson = Globals::g->deathmatch && Globals::g->st_statusbaron; 

	STlib_updateNum(&Globals::g->w_ready, refresh);

	for (i=0;i<4;i++)
	{
		STlib_updateNum(&Globals::g->w_ammo[i], refresh);
		STlib_updateNum(&Globals::g->w_maxammo[i], refresh);
	}

	STlib_updatePercent(&Globals::g->w_health, refresh);
	STlib_updatePercent(&Globals::g->w_armor, refresh);

	STlib_updateBinIcon(&Globals::g->w_armsbg, refresh);

	for (i=0;i<6;i++)
		STlib_updateMultIcon(&Globals::g->w_arms[i], refresh);

	STlib_updateMultIcon(&Globals::g->w_faces, refresh);

	for (i=0;i<3;i++)
		STlib_updateMultIcon(&Globals::g->w_keyboxes[i], refresh);

	STlib_updateNum(&Globals::g->w_frags, refresh);

}

void ST_doRefresh(void)
{
	Globals::g->st_firsttime = false;

	// draw status bar background to off-screen buff
	ST_refreshBackground();

	// and refresh all widgets
	ST_drawWidgets(true);
}

void ST_diffDraw(void)
{
	// update all widgets
	ST_drawWidgets(false);
}

void ST_Drawer (qboolean fullscreen, qboolean refresh)
{
	Globals::g->st_statusbaron = (!fullscreen) || Globals::g->automapactive;
	Globals::g->st_firsttime = Globals::g->st_firsttime || refresh;

	// Do red-/gold-shifts from damage/items
	ST_doPaletteStuff();

	// If just after ST_Start(), refresh all
	if (Globals::g->st_firsttime) ST_doRefresh();
	// Otherwise, update as little as possible
	else ST_diffDraw();
}

void ST_loadGraphics(void)
{
	static bool ST_HasBeenCalled = false;

//	if (ST_HasBeenCalled == true)
//		return;
	ST_HasBeenCalled = true;
	
	int		i;
	int		j;
	int		facenum;

	char	namebuf[9];

	// Load the numbers, tall and short
	for (i=0;i<10;i++)
	{
		sprintf(namebuf, "STTNUM%d", i);
		Globals::g->tallnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC_SHARED);

		sprintf(namebuf, "STYSNUM%d", i);
		Globals::g->shortnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC_SHARED);
	}

	// Load percent key.
	//Note: why not load STMINUS here, too?
	Globals::g->tallpercent = (patch_t *) W_CacheLumpName("STTPRCNT", PU_STATIC_SHARED);

	// key cards
	for (i=0;i<NUMCARDS;i++)
	{
		sprintf(namebuf, "STKEYS%d", i);
		Globals::g->keys[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC_SHARED);
	}

	// Globals::g->arms background
	Globals::g->armsbg = (patch_t *) W_CacheLumpName("STARMS", PU_STATIC_SHARED);

	// Globals::g->arms ownership widgets
	for (i=0;i<6;i++)
	{
		sprintf(namebuf, "STGNUM%d", i+2);

		// gray #
		Globals::g->arms[i][0] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC_SHARED);

		// yellow #
		Globals::g->arms[i][1] = Globals::g->shortnum[i+2]; 
	}

	// face backgrounds for different color Globals::g->players
	sprintf(namebuf, "STFB%d", Globals::g->consoleplayer);
	Globals::g->faceback = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC_SHARED);

	// status bar background bits
	Globals::g->sbar = (patch_t *) W_CacheLumpName("STBAR", PU_STATIC_SHARED);

	// face states
	facenum = 0;
	for (i=0;i<ST_NUMPAINFACES;i++)
	{
		for (j=0;j<ST_NUMSTRAIGHTFACES;j++)
		{
			sprintf(namebuf, "STFST%d%d", i, j);
			Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
		}
		sprintf(namebuf, "STFTR%d0", i);	// turn right
		Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
		sprintf(namebuf, "STFTL%d0", i);	// turn left
		Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
		sprintf(namebuf, "STFOUCH%d", i);	// ouch!
		Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
		sprintf(namebuf, "STFEVL%d", i);	// evil grin ;)
		Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
		sprintf(namebuf, "STFKILL%d", i);	// pissed off
		Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName(namebuf, PU_STATIC_SHARED);
	}
	Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName("STFGOD0", PU_STATIC_SHARED);
	Globals::g->faces[facenum++] = (patch_t*)W_CacheLumpName("STFDEAD0", PU_STATIC_SHARED);

}

void ST_loadData(void)
{
	Globals::g->lu_palette = W_GetNumForName ("PLAYPAL");
	ST_loadGraphics();
}

void ST_unloadGraphics(void)
{
	// These things are always reloaded... so just don't bother to clean them up!
}

void ST_unloadData(void)
{
	ST_unloadGraphics();
}

void ST_initData(void)
{

	int		i;

	Globals::g->st_firsttime = true;
	Globals::g->plyr = &Globals::g->players[Globals::g->consoleplayer];

	Globals::g->st_clock = 0;
	Globals::g->st_chatstate = StartChatState;
	Globals::g->st_gamestate = FirstPersonState;

	Globals::g->st_statusbaron = true;
	Globals::g->st_oldchat = Globals::g->st_chat = false;
	Globals::g->st_cursoron = false;

	Globals::g->st_faceindex = 0;
	Globals::g->st_palette = -1;

	Globals::g->st_oldhealth = -1;

	for (i=0;i<NUMWEAPONS;i++)
		Globals::g->oldweaponsowned[i] = Globals::g->plyr->weaponowned[i];

	for (i=0;i<3;i++)
		Globals::g->keyboxes[i] = -1;

	STlib_init();

}



void ST_createWidgets(void)
{

	int i;

	// ready weapon ammo
	STlib_initNum(&Globals::g->w_ready,
		ST_AMMOX,
		ST_AMMOY,
		Globals::g->tallnum,
		&Globals::g->plyr->ammo[weaponinfo[Globals::g->plyr->readyweapon].ammo],
		&Globals::g->st_statusbaron,
		ST_AMMOWIDTH );

	// the last weapon type
	Globals::g->w_ready.data = Globals::g->plyr->readyweapon; 

	// health percentage
	STlib_initPercent(&Globals::g->w_health,
		ST_HEALTHX,
		ST_HEALTHY,
		Globals::g->tallnum,
		&Globals::g->plyr->health,
		&Globals::g->st_statusbaron,
		Globals::g->tallpercent);

	// Globals::g->arms background
	STlib_initBinIcon(&Globals::g->w_armsbg,
		ST_ARMSBGX,
		ST_ARMSBGY,
		Globals::g->armsbg,
		&Globals::g->st_notdeathmatch,
		&Globals::g->st_statusbaron);

	// weapons owned
	for(i=0;i<6;i++)
	{
		STlib_initMultIcon(&Globals::g->w_arms[i],
			ST_ARMSX+(i%3)*ST_ARMSXSPACE,
			ST_ARMSY+(i/3)*ST_ARMSYSPACE,
			Globals::g->arms[i], (int *) &Globals::g->plyr->weaponowned[i+1],
			&Globals::g->st_armson);
	}

	// frags sum
	STlib_initNum(&Globals::g->w_frags,
		ST_FRAGSX,
		ST_FRAGSY,
		Globals::g->tallnum,
		&Globals::g->st_fragscount,
		&Globals::g->st_fragson,
		ST_FRAGSWIDTH);

	// Globals::g->faces
	STlib_initMultIcon(&Globals::g->w_faces,
		ST_FACESX,
		ST_FACESY,
		Globals::g->faces,
		&Globals::g->st_faceindex,
		&Globals::g->st_statusbaron);

	// armor percentage - should be colored later
	STlib_initPercent(&Globals::g->w_armor,
		ST_ARMORX,
		ST_ARMORY,
		Globals::g->tallnum,
		&Globals::g->plyr->armorpoints,
		&Globals::g->st_statusbaron, Globals::g->tallpercent);

	// Globals::g->keyboxes 0-2
	STlib_initMultIcon(&Globals::g->w_keyboxes[0],
		ST_KEY0X,
		ST_KEY0Y,
		Globals::g->keys,
		&Globals::g->keyboxes[0],
		&Globals::g->st_statusbaron);

	STlib_initMultIcon(&Globals::g->w_keyboxes[1],
		ST_KEY1X,
		ST_KEY1Y,
		Globals::g->keys,
		&Globals::g->keyboxes[1],
		&Globals::g->st_statusbaron);

	STlib_initMultIcon(&Globals::g->w_keyboxes[2],
		ST_KEY2X,
		ST_KEY2Y,
		Globals::g->keys,
		&Globals::g->keyboxes[2],
		&Globals::g->st_statusbaron);

	// ammo count (all four kinds)
	STlib_initNum(&Globals::g->w_ammo[0],
		ST_AMMO0X,
		ST_AMMO0Y,
		Globals::g->shortnum,
		&Globals::g->plyr->ammo[0],
		&Globals::g->st_statusbaron,
		ST_AMMO0WIDTH);

	STlib_initNum(&Globals::g->w_ammo[1],
		ST_AMMO1X,
		ST_AMMO1Y,
		Globals::g->shortnum,
		&Globals::g->plyr->ammo[1],
		&Globals::g->st_statusbaron,
		ST_AMMO1WIDTH);

	STlib_initNum(&Globals::g->w_ammo[2],
		ST_AMMO2X,
		ST_AMMO2Y,
		Globals::g->shortnum,
		&Globals::g->plyr->ammo[2],
		&Globals::g->st_statusbaron,
		ST_AMMO2WIDTH);

	STlib_initNum(&Globals::g->w_ammo[3],
		ST_AMMO3X,
		ST_AMMO3Y,
		Globals::g->shortnum,
		&Globals::g->plyr->ammo[3],
		&Globals::g->st_statusbaron,
		ST_AMMO3WIDTH);

	// max ammo count (all four kinds)
	STlib_initNum(&Globals::g->w_maxammo[0],
		ST_MAXAMMO0X,
		ST_MAXAMMO0Y,
		Globals::g->shortnum,
		&Globals::g->plyr->maxammo[0],
		&Globals::g->st_statusbaron,
		ST_MAXAMMO0WIDTH);

	STlib_initNum(&Globals::g->w_maxammo[1],
		ST_MAXAMMO1X,
		ST_MAXAMMO1Y,
		Globals::g->shortnum,
		&Globals::g->plyr->maxammo[1],
		&Globals::g->st_statusbaron,
		ST_MAXAMMO1WIDTH);

	STlib_initNum(&Globals::g->w_maxammo[2],
		ST_MAXAMMO2X,
		ST_MAXAMMO2Y,
		Globals::g->shortnum,
		&Globals::g->plyr->maxammo[2],
		&Globals::g->st_statusbaron,
		ST_MAXAMMO2WIDTH);

	STlib_initNum(&Globals::g->w_maxammo[3],
		ST_MAXAMMO3X,
		ST_MAXAMMO3Y,
		Globals::g->shortnum,
		&Globals::g->plyr->maxammo[3],
		&Globals::g->st_statusbaron,
		ST_MAXAMMO3WIDTH);

}



void ST_Start (void)
{

	if (!Globals::g->st_stopped)
		ST_Stop();

	ST_initData();
	ST_createWidgets();
	Globals::g->st_stopped = false;

}

void ST_Stop (void)
{
	if (Globals::g->st_stopped)
		return;

	I_SetPalette ((unsigned char*)W_CacheLumpNum ((int)Globals::g->lu_palette, PU_CACHE_SHARED));

	Globals::g->st_stopped = true;
}

void ST_Init (void)
{
	Globals::g->veryfirsttime = 0;
	ST_loadData();
	Globals::g->screens[4] = (unsigned char *) DoomLib::Z_Malloc( SCREENWIDTH * SCREENHEIGHT /*ST_WIDTH*ST_HEIGHT*/, PU_STATIC, 0);
	memset( Globals::g->screens[4], 0, SCREENWIDTH * SCREENHEIGHT );
}


/*CONSOLE_COMMAND_SHIP( idqd, "cheat for toggleable god mode", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	Globals::g->plyr->cheats ^= CF_GODMODE;
	if (Globals::g->plyr->cheats & CF_GODMODE)
	{
		if (Globals::g->plyr->mo)
			Globals::g->plyr->mo->health = 100;

		Globals::g->plyr->health = 100;
		Globals::g->plyr->message = STSTR_DQDON;
	}
	else 
		Globals::g->plyr->message = STSTR_DQDOFF;

	DoomLib::SetPlayer( oldPlayer );
}

CONSOLE_COMMAND_SHIP( idfa, "cheat for killer fucking arsenal", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	int i = 0;
	Globals::g->plyr->armorpoints = 200;
	Globals::g->plyr->armortype = 2;

	for (i=0;i<NUMWEAPONS;i++)
		Globals::g->plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
		Globals::g->plyr->ammo[i] = Globals::g->plyr->maxammo[i];

	Globals::g->plyr->message = STSTR_FAADDED;

	DoomLib::SetPlayer( oldPlayer );
}

CONSOLE_COMMAND_SHIP( idkfa, "cheat for key full ammo", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	int i = 0;
	Globals::g->plyr->armorpoints = 200;
	Globals::g->plyr->armortype = 2;

	for (i=0;i<NUMWEAPONS;i++)
		Globals::g->plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
		Globals::g->plyr->ammo[i] = Globals::g->plyr->maxammo[i];

	for (i=0;i<NUMCARDS;i++)
		Globals::g->plyr->cards[i] = true;

	Globals::g->plyr->message = STSTR_KFAADDED;

	DoomLib::SetPlayer( oldPlayer );
}


CONSOLE_COMMAND_SHIP( idclip, "cheat for no clip", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	Globals::g->plyr->cheats ^= CF_NOCLIP;

	if (Globals::g->plyr->cheats & CF_NOCLIP)
		Globals::g->plyr->message = STSTR_NCON;
	else
		Globals::g->plyr->message = STSTR_NCOFF;

	DoomLib::SetPlayer( oldPlayer );
}
CONSOLE_COMMAND_SHIP( idmypos, "for player position", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	static char	buf[ST_MSGWIDTH];
	sprintf(buf, "ang=0x%x;x,y=(0x%x,0x%x)",
		Globals::g->players[Globals::g->consoleplayer].mo->angle,
		Globals::g->players[Globals::g->consoleplayer].mo->x,
		Globals::g->players[Globals::g->consoleplayer].mo->y);
	Globals::g->plyr->message = buf;

	DoomLib::SetPlayer( oldPlayer );
}

CONSOLE_COMMAND_SHIP( idclev, "warp to next level", 0 ) {
	int oldPlayer = DoomLib::GetPlayer();
	DoomLib::SetPlayer( 0 );
	if ( Globals::g == NULL ) {
		return;
	}

	if (Globals::g->gamestate != GS_LEVEL) {
		return;
	}

	int		epsd;
	int		map;

	if (Globals::g->gamemode == commercial)
	{
		
		if( args.Argc() > 1 ) {
			epsd = 1;
			map = atoi( args.Argv( 1 ) );
		} else {
			idLib::Printf( "idclev takes map as first argument \n"  );
			return;
		}

		if( map > 32 ) {
			map = 1;
		}
	}
	else
	{
		if( args.Argc() > 2 ) {
			epsd = atoi( args.Argv( 1 ) );
			map = atoi( args.Argv( 2 ) );
		} else {
			idLib::Printf( "idclev takes episode and map as first two arguments \n"  );
			return;
		}
	}

	// Catch invalid maps.
	if (epsd < 1)
		return;

	if (map < 1)
		return;

	// Ohmygod - this is not going to work.
	if ((Globals::g->gamemode == retail)
		&& ((epsd > 4) || (map > 9)))
		return;

	if ((Globals::g->gamemode == registered)
		&& ((epsd > 3) || (map > 9)))
		return;

	if ((Globals::g->gamemode == shareware)
		&& ((epsd > 1) || (map > 9)))
		return;

	if ((Globals::g->gamemode == commercial)
		&& (( epsd > 1) || (map > 34)))
		return;

	// So be it.
	Globals::g->plyr->message = STSTR_CLEV;
	G_DeferedInitNew(Globals::g->gameskill, epsd, map);

	DoomLib::SetPlayer( oldPlayer );
}*/
