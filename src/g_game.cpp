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
#include "Main.hpp"
//#include "sys/sys_signin.hpp"
//#include "d3xp/Game_local.hpp"


#include <string.h>
#include <stdlib.h>

#include "doomdef.hpp" 
#include "doomstat.hpp"

#include "z_zone.hpp"
#include "f_finale.hpp"
#include "m_argv.hpp"
#include "m_misc.hpp"
#include "m_menu.hpp"
#include "m_random.hpp"
#include "i_system.hpp"

#include "p_setup.hpp"
#include "p_saveg.hpp"
#include "p_tick.hpp"

#include "d_main.hpp"

#include "wi_stuff.hpp"
#include "hu_stuff.hpp"
#include "st_stuff.hpp"
#include "am_map.hpp"

// Needs access to LFB.
#include "v_video.hpp"

#include "w_wad.hpp"

#include "p_local.hpp" 

#include "s_sound.hpp"

// Data.
#include "dstrings.hpp"
#include "sounds.hpp"

// SKY handling - still the wrong place.
#include "r_data.hpp"
#include "r_sky.hpp"

#include "g_game.hpp"

//#include "framework/Common.hpp"
//#include "sys/sys_lobby.hpp"

#include <limits>


extern bool waitingForWipe;

bool	loadingGame = false;

unsigned char	demoversion = 0;

qboolean	G_CheckDemoStatus (void); 
void	G_ReadDemoTiccmd (ticcmd_t* cmd); 
void	G_WriteDemoTiccmd (ticcmd_t* cmd); 
void	G_PlayerReborn (int player); 
void	G_InitNew (skill_t skill, int episode, int map ); 

void	G_DoReborn (int playernum); 

void	G_DoLoadLevel (); 
void	G_DoNewGame (void); 
qboolean	G_DoLoadGame (); 
void	G_DoPlayDemo (void); 
void	G_DoCompleted (void); 
void	G_DoVictory (void); 
void	G_DoWorldDone (void); 
qboolean	G_DoSaveGame (void); 


#define	DEBUG_DEMOS
#define DEBUG_DEMOS_WRITE

#ifdef DEBUG_DEMOS
unsigned char testprndindex = 0;
int printErrorCount = 0;
bool demoDebugOn = false;
#endif

// 
// controls (have defaults) 
// 

// mouse values are used once 

// joystick values are repeated 


int G_CmdChecksum (ticcmd_t* cmd) 
{ 
	int		i;
	int		sum = 0; 

	for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
		sum += ((int *)cmd)[i]; 

	return sum; 
} 

// jedi academy meets doom hehehehehehehe
void G_MouseClamp(int *x, int *y)
{
	float ax = (float)fabs((float)*x);
	float ay = (float)fabs((float)*y);

	ax = (ax-10)*(0.04676) * (ax-10) * (ax > 10);
	ay = (ay-10)*(0.04676) * (ay-10) * (ay > 10);
	if (*x < 0)
		*x = static_cast<int>(-ax);
	else
		*x = static_cast<int>(ax);
	if (*y < 0)
		*y = static_cast<int>(-ay);
	else
		*y = static_cast<int>(ay);
}

/*
========================
Returns true if the player is holding down the run button, or
if they have set "Always run" in the options. Returns false otherwise.
========================
*/
/*bool IsPlayerRunning( const usercmd_t & command ) {

	if( DoomLib::GetPlayer() < 0 ) {
		return false;
	}

	// DHM - Nerve :: Always Run setting
	idLocalUser * user = session->GetSignInManager().GetLocalUserByIndex( DoomLib::GetPlayer() );
	bool autorun = false;


	// TODO: PC
#if 0
	if( user ) {
		idPlayerProfileDoom * profile = static_cast< idPlayerProfileDoom * >( user->GetProfile() );

		if( profile && profile->GetAlwaysRun() ) {
			autorun = true;
		}
	}
#endif

	if ( command.buttons & BUTTON_RUN  ) {
		return !autorun;
	}

	

	

	return autorun;
}*/

/*
========================
G_PerformImpulse
========================
*/
void G_PerformImpulse( const int impulse, ticcmd_t* cmd ) {

	/*if( impulse == IMPULSE_15 ) {
		cmd->buttons |= BT_CHANGE; 
		cmd->nextPrevWeapon = 1 ; 
	} else if( impulse == IMPULSE_14 ) {
		cmd->buttons |= BT_CHANGE; 
		cmd->nextPrevWeapon = 2 ; 
	}  */

}

/*
========================
Converts a degree value to DOOM format angle value.
========================
*/
fixed_t DegreesToDoomAngleTurn( float degrees ) {
	const float anglefrac = degrees / 360.0f;
	const fixed_t doomangle = anglefrac * std::numeric_limits<unsigned short>::max();

	return doomangle;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 
void G_BuildTiccmd (ticcmd_t* cmd, int newTics )
{ 
	int		i; 
	int		speed;
	int		tspeed; 
	int		forward;
	int		side;

	ticcmd_t*	base;

	base = I_BaseTiccmd ();		// empty, or external driver
	memcpy (cmd,base,sizeof(*cmd)); 

	cmd->consistancy = Globals::g->consistancy[Globals::g->consoleplayer][Globals::g->maketic%BACKUPTICS]; 

	// Grab the tech5 tic so we can convert it to a doom tic.
//    //if ( userCmdMgr != NULL ) {
//        const int playerIndex = DoomLib::GetPlayer();
//
//        if( playerIndex < 0 ) {
//            return;
//        }
//
//#ifdef ID_ENABLE_NETWORKING
//        const int lobbyIndex = gameLocal->GetLobbyIndexFromDoomLibIndex( playerIndex );
//        const idLocalUser * const localUser = session->GetGameLobbyBase().GetLocalUserFromLobbyUser( lobbyIndex );
//#else
//        const int lobbyIndex = 0;
//    //    const idLocalUser * const localUser = session->GetSignInManager().GetMasterLocalUser();
//#endif
//
///*        if ( localUser == NULL ) {
//            return;
//        }
//
//        usercmd_t * tech5commands[2] = { 0, 0 };
//
//        const int numCommands = userCmdMgr->GetPlayerCmds( lobbyIndex, tech5commands, 2 );
//
//        usercmd_t prevTech5Command;
//        usercmd_t curTech5Command;*/
//
//        // Use default commands if the manager didn't have enough.
//        /*if ( numCommands == 1 ) {
//            curTech5Command = *(tech5commands)[0];
//        }
//
//        if ( numCommands == 2 ) {
//            prevTech5Command = *(tech5commands)[0];
//            curTech5Command = *(tech5commands)[1];
//        }*/
//
//        //const bool isRunning = IsPlayerRunning( curTech5Command );
//        const bool isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
//
//        // tech5 move commands range from -127 o 127. Scale to doom range of -25 to 25.
//        //const float scaledForward = curTech5Command.forwardmove / 127.0f;
//    float scaledForward = 0.f;
//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
//        scaledForward += 1.f;
//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
//        scaledForward -= 1.f;
//
//        if ( isRunning ) {
//            cmd->forwardmove = scaledForward * 50.0f;
//        } else {
//            cmd->forwardmove = scaledForward * 25.0f;
//        }
//
//        // tech5 move commands range from -127 o 127. Scale to doom range of -24 to 24.
//        //const float scaledSide = curTech5Command.rightmove / 127.0f;
//    float scaledSide = 0.f;
//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
//        scaledSide += 1.f;
//    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
//        scaledSide -= 1.f;
//
//        if ( isRunning ) {
//            cmd->sidemove = scaledSide * 40.0f;
//        } else {
//            cmd->sidemove = scaledSide * 24.0f;
//        }
//
//        /*idAngles angleDelta;
//        angleDelta.pitch    = SHORT2ANGLE( curTech5Command.angles[ 0 ] ) - SHORT2ANGLE( prevTech5Command.angles[ 0 ] );
//        angleDelta.yaw        = SHORT2ANGLE( curTech5Command.angles[ 1 ] ) - SHORT2ANGLE( prevTech5Command.angles[ 1 ] );
//        angleDelta.roll        = 0.0f;
//        angleDelta.Normalize180();*/
//
//        // We will be running a number of tics equal to newTics before we get a new command from tech5.
//        // So to keep input smooth, divide the angles between all the newTics.
//        if ( newTics > 0 ) {
//            //angleDelta.yaw /= newTics;
//        }
//
//        // idAngles is stored in degrees. Convert to doom format.
//        //cmd->angleturn = DegreesToDoomAngleTurn( angleDelta.yaw );
//
//
//        // Translate buttons
//        //if ( curTech5Command.inhibited == false ) {
//            // Attack 1 attacks always, whether in the automap or not.
////            if ( curTech5Command.buttons & BUTTON_ATTACK ) {
////                cmd->buttons |= BT_ATTACK;
////            }
//
//#if 0
//            // Attack 2 only attacks if not in the automap, because when in the automap,
//            // it is the zoom function.
////            if ( curTech5Command.buttons & BUTTON_ATTACK2 ) {
////                if ( !Globals::g->automapactive ) {
////                    cmd->buttons |= BT_ATTACK;
////                }
////            }
//#endif
//
//            // Try to read any impulses that have happened.
////            static int oldImpulseSequence = 0;
////            if( oldImpulseSequence != curTech5Command.impulseSequence ) {
////                G_PerformImpulse( curTech5Command.impulse, cmd );
////            }
////            oldImpulseSequence = curTech5Command.impulseSequence;
//
//            // weapon toggle
//    static std::array<sf::Keyboard::Key, NUMWEAPONS+1> weaponButtons = {{
//        sf::Keyboard::Num0,
//        sf::Keyboard::Num1,
//        sf::Keyboard::Num2,
//        sf::Keyboard::Num3,
//        sf::Keyboard::Num4,
//        sf::Keyboard::Num5,
//        sf::Keyboard::Num6,
//        sf::Keyboard::Num7,
//        sf::Keyboard::Num8,
//        sf::Keyboard::Num9
//    }};
//
//    for (auto button : weaponButtons)
//    {
//        if ( sf::Keyboard::isKeyPressed(button) )
//        {
//            cmd->buttons |= BT_CHANGE;
//            cmd->buttons |= (std::distance(std::find(weaponButtons.begin(),weaponButtons.end(),button),weaponButtons.begin()) - 1) <<BT_WEAPONSHIFT;
//            break;
//        }
//    }
//
//
//    //if ( curTech5Command.buttons & BUTTON_USE || curTech5Command.buttons & BUTTON_JUMP ) {
//    if ( sf::Keyboard::isKeyPressed(sf::Keyboard::E) || sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
//        cmd->buttons |= BT_USE;
//    }
//
//    // TODO: PC
//#if 0
//    if ( curTech5Command.buttons & BUTTON_WEAP_NEXT ) {
//        cmd->buttons |= BT_CHANGE;
//        cmd->buttons |= 1 << BT_WEAPONSHIFT;
//    }
//
//    if ( curTech5Command.buttons & BUTTON_WEAP_PREV ) {
//        cmd->buttons |= BT_CHANGE;
//        cmd->buttons |= 0 << BT_WEAPONSHIFT;
//    }
//
//            if( curTech5Command.buttons & BUTTON_WEAP_0 ) {
//                cmd->buttons |= BT_CHANGE;
//                cmd->buttons |= 2 << BT_WEAPONSHIFT;
//            }
//
//            if( curTech5Command.buttons & BUTTON_WEAP_1 ) {
//                cmd->buttons |= BT_CHANGE;
//                cmd->buttons |= 3 << BT_WEAPONSHIFT;
//            }
//
//            if( curTech5Command.buttons & BUTTON_WEAP_2 ) {
//                cmd->buttons |= BT_CHANGE;
//                cmd->buttons |= 4 << BT_WEAPONSHIFT;
//            }
//
//            if( curTech5Command.buttons & BUTTON_WEAP_3 ) {
//                cmd->buttons |= BT_CHANGE;
//                cmd->buttons |= 5 << BT_WEAPONSHIFT;
//            }
//#endif
//
//        //}
//
//        //return;
//    //}

	// DHM - Nerve :: Always Run setting
	//idLocalUser * user = session->GetSignInManager().GetLocalUserByIndex( DoomLib::GetPlayer() );

	//if( user ) {
		// TODO: PC
#if 0
		idPlayerProfileDoom * profile = static_cast< idPlayerProfileDoom * >( user->GetProfile() );

		if( profile && profile->GetAlwaysRun() ) {
			speed = !Globals::g->gamekeydown[Globals::g->key_speed];
		} else
#endif
		{
			speed = Globals::g->gamekeydown[Globals::g->key_speed];
		}

//	} else  {

		// Should not happen.
//		speed = !Globals::g->gamekeydown[Globals::g->key_speed];
//	}

	forward = side = 0;

	// use two stage accelerative turning
	// on the keyboard and joystick
	if (/*:g->joyxmove != 0  ||*/   Globals::g->gamekeydown[Globals::g->key_right] ||Globals::g->gamekeydown[Globals::g->key_left] || Globals::g->mousex != 0)
		Globals::g->turnheld += Globals::g->ticdup;
	else 
		Globals::g->turnheld = 0; 

	if (Globals::g->turnheld < SLOWTURNTICS) 
		tspeed = 2;             // slow turn 
	else 
		tspeed = speed;


	// clamp for turning
	int mousex = Globals::g->mousex;
	int mousey = Globals::g->mousey;
	G_MouseClamp( &mousex, &mousey );

	if (Globals::g->gamekeydown[Globals::g->key_right] /*|| Globals::g->joyxmove > 0*/)
		cmd->angleturn -= Globals::g->angleturn[tspeed];
	else if (Globals::g->mousex > 0) {
		cmd->angleturn -= tspeed == 1 ? 2 * mousex : mousex;
	}

	if (Globals::g->gamekeydown[Globals::g->key_left] /*|| Globals::g->joyxmove < 0*/)
		cmd->angleturn += Globals::g->angleturn[tspeed];
	else if (Globals::g->mousex < 0) {
		cmd->angleturn += tspeed == 1 ? -2 * mousex : -mousex;
	}

	if (Globals::g->mousey > 0 || Globals::g->mousey < 0) {
		//forward += Globals::g->forwardmove[speed];
		forward += speed == 1 ? 2 * Globals::g->mousey : Globals::g->mousey;
	}

	if (Globals::g->mousey < 0) {
		forward -= Globals::g->forwardmove[speed];
	}


	if (Globals::g->gamekeydown[Globals::g->key_straferight]) 
		side += Globals::g->sidemove[speed]; 
	if (Globals::g->gamekeydown[Globals::g->key_strafeleft]) 
		side -= Globals::g->sidemove[speed];


	if ( Globals::g->joyxmove > 0 || Globals::g->joyxmove < 0 ) {
		side += speed == 1 ? 2 * Globals::g->joyxmove : Globals::g->joyxmove;
	}

	// buttons
	if (Globals::g->gamekeydown[Globals::g->key_fire] || Globals::g->mousebuttons[Globals::g->mousebfire] || Globals::g->joybuttons[Globals::g->joybfire]) 
		cmd->buttons |= BT_ATTACK; 

	if (Globals::g->gamekeydown[Globals::g->key_use] || Globals::g->joybuttons[Globals::g->joybuse] ) 
		cmd->buttons |= BT_USE;

	// DHM - Nerve :: In the intermission or finale screens, make START also create a 'use' command.
	if ( (Globals::g->gamestate == GS_INTERMISSION || Globals::g->gamestate == GS_FINALE) && Globals::g->gamekeydown[KEY_ESCAPE] ) {		
		cmd->buttons |= BT_USE;
	}

	// weapon toggle
	for (i=0 ; i<NUMWEAPONS-1 ; i++) 
	{   
		if (Globals::g->gamekeydown['1'+i]) 
		{ 
			cmd->buttons |= BT_CHANGE; 
			cmd->buttons |= i<<BT_WEAPONSHIFT;
			break; 
		}
	}

	Globals::g->mousex = Globals::g->mousey = 0; 

	if (forward > MAXPLMOVE) 
		forward = MAXPLMOVE; 
	else if (forward < -MAXPLMOVE) 
		forward = -MAXPLMOVE; 
	if (side > MAXPLMOVE) 
		side = MAXPLMOVE; 
	else if (side < -MAXPLMOVE) 
		side = -MAXPLMOVE; 

	cmd->forwardmove += forward; 
	cmd->sidemove += side;

	// special buttons
	if (Globals::g->sendpause) 
	{ 
		Globals::g->sendpause = false; 
		cmd->buttons = BT_SPECIAL | BTS_PAUSE; 
	} 

	if (Globals::g->sendsave) 
	{ 
		Globals::g->sendsave = false; 
		cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (Globals::g->savegameslot<<BTS_SAVESHIFT); 
	}
} 


//
// G_DoLoadLevel 
//

void G_DoLoadLevel () 
{ 
	int             i; 

	M_ClearRandom();

	// Set the sky map.
	// First thing, we have a dummy sky texture name,
	//  a flat. The data is in the WAD only because
	//  we look for an actual index, instead of simply
	//  setting one.
	Globals::g->skyflatnum = R_FlatNumForName ( SKYFLATNAME );

	// DOOM determines the sky texture to be used
	// depending on the current episode, and the game version.
	if ( Globals::g->gamemode == commercial )
	{
		Globals::g->skytexture = R_TextureNumForName ("SKY3");

		if (Globals::g->gamemap < 12) {
			Globals::g->skytexture = R_TextureNumForName ("SKY1");
		}
		else if (Globals::g->gamemap < 21) {
			Globals::g->skytexture = R_TextureNumForName ("SKY2");
		}
	}

	Globals::g->levelstarttic = Globals::g->gametic;        // for time calculation

	if (Globals::g->wipegamestate == GS_LEVEL) {
		Globals::g->wipegamestate = (gamestate_t)-1;             // force a wipe 
	} else if ( Globals::g->netgame ) {
		Globals::g->wipegamestate = GS_LEVEL;
	}

	Globals::g->gamestate = GS_LEVEL; 

	for (i=0 ; i<MAXPLAYERS ; i++) 
	{ 
		if (Globals::g->playeringame[i] && Globals::g->players[i].playerstate == PST_DEAD) 
			Globals::g->players[i].playerstate = PST_REBORN; 
		memset (Globals::g->players[i].frags,0,sizeof(Globals::g->players[i].frags));
		memset (&(Globals::g->players[i].cmd),0,sizeof(Globals::g->players[i].cmd)); 
	} 

	const char * difficultyNames[] = {  "I'm Too Young To Die!", "Hey, Not Too Rough!", "Hurt Me Plenty!", "Ultra-Violence", "Nightmare" };
	const ExpansionData * expansion = DoomLib::GetCurrentExpansion();
	
	int truemap = Globals::g->gamemap;

	if( Globals::g->gamemission == doom ) {
		truemap = ( Globals::g->gameepisode - 1 ) * 9 + ( Globals::g->gamemap );
	}

	//idMatchParameters newParms = session->GetActingGameStateLobbyBase().GetMatchParms();
	DoomLib::SetCurrentMapName( expansion->mapNames[ truemap - 1 ] );
	DoomLib::SetCurrentDifficulty( difficultyNames[ Globals::g->gameskill ]  );

	P_SetupLevel (Globals::g->gameepisode, Globals::g->gamemap, 0, Globals::g->gameskill);

	Globals::g->displayplayer = Globals::g->consoleplayer;		// view the guy you are playing    
	Globals::g->starttime = I_GetTime (); 
	Globals::g->gameaction = ga_nothing; 

	// clear cmd building stuff
	memset (Globals::g->gamekeydown, 0, sizeof(Globals::g->gamekeydown)); 
	Globals::g->joyxmove = Globals::g->joyymove = 0; 
	Globals::g->mousex = Globals::g->mousey = 0; 
	Globals::g->sendpause = Globals::g->sendsave = Globals::g->paused = false; 
	memset (Globals::g->mousebuttons, 0, sizeof(Globals::g->mousebuttons)); 
	memset (Globals::g->joybuttons, 0, sizeof(Globals::g->joybuttons));
}

//
// G_Responder  
// Get info needed to make ticcmd_ts for the Globals::g->players.
// 
qboolean G_Responder (event_t* ev) 
{ 
	// allow spy mode changes even during the demo
	if (Globals::g->gamestate == GS_LEVEL && ev->type == ev_keydown 
		&& ev->data1 == KEY_F12 && (Globals::g->singledemo || !Globals::g->deathmatch) )
	{
		// spy mode 
		do 
		{ 
			Globals::g->displayplayer++; 
			if (Globals::g->displayplayer == MAXPLAYERS) 
				Globals::g->displayplayer = 0; 
		} while (!Globals::g->playeringame[Globals::g->displayplayer] && Globals::g->displayplayer != Globals::g->consoleplayer); 
		return true; 
	}

	// any other key pops up menu if in demos
	if (Globals::g->gameaction == ga_nothing && !Globals::g->singledemo && 
		(Globals::g->demoplayback || Globals::g->gamestate == GS_DEMOSCREEN) 
		) 
	{ 
		if (ev->type == ev_keydown ||  
			(ev->type == ev_mouse && ev->data1) ||
			(ev->type == ev_joystick && ev->data1) ) 
		{ 
			M_StartControlPanel (); 
			return true; 
		} 
		return false; 
	} 

	if (Globals::g->gamestate == GS_LEVEL && ( Globals::g->usergame || Globals::g->netgame || Globals::g->demoplayback )) 
	{ 
#if 0 
		if (Globals::g->devparm && ev->type == ev_keydown && ev->data1 == ';') 
		{ 
			G_DeathMatchSpawnPlayer (0); 
			return true; 
		} 
#endif 
		if (HU_Responder (ev)) 
			return true;	// chat ate the event 
		if (ST_Responder (ev)) 
			return true;	// status window ate it 
		if (AM_Responder (ev)) 
			return true;	// automap ate it 
	} 

	if (Globals::g->gamestate == GS_FINALE) 
	{ 
		if (F_Responder (ev)) 
			return true;	// finale ate the event 
	} 

	switch (ev->type) 
	{ 
	case ev_keydown: 
		if (ev->data1 == KEY_PAUSE) 
		{ 
			Globals::g->sendpause = true; 
			return true; 
		} 
		if (ev->data1 <NUMKEYS) 
			Globals::g->gamekeydown[ev->data1] = true; 
		return true;    // eat key down Globals::g->events 

	case ev_keyup:
		// DHM - Nerve :: Old School!
		//if ( ev->data1 == '-' ) {
			//App->Renderer->oldSchool = !App->Renderer->oldSchool;
		//}

		if (ev->data1 <NUMKEYS) 
			Globals::g->gamekeydown[ev->data1] = false; 
		return false;   // always let key up Globals::g->events filter down 

	case ev_mouse: 
 		Globals::g->mousebuttons[0] = ev->data1 & 1; 
 		Globals::g->mousebuttons[1] = ev->data1 & 2; 
 		Globals::g->mousebuttons[2] = ev->data1 & 4; 
 		Globals::g->mousex = ev->data2*(Globals::g->mouseSensitivity+5)/10; 
 		Globals::g->mousey = ev->data3*(Globals::g->mouseSensitivity+5)/10; 
 		return true;    // eat Globals::g->events 

	case ev_joystick: 
		Globals::g->joybuttons[0] = ev->data1 & 1; 
		Globals::g->joybuttons[1] = ev->data1 & 2; 
		Globals::g->joybuttons[2] = ev->data1 & 4; 
		Globals::g->joybuttons[3] = ev->data1 & 8; 
		Globals::g->joyxmove = ev->data2; 
/*
		Globals::g->gamekeydown[Globals::g->key_straferight] = Globals::g->gamekeydown[Globals::g->key_strafeleft] = 0;
		if (ev->data2 > 0)
			Globals::g->gamekeydown[Globals::g->key_straferight] = 1;
		else if (ev->data2 < 0)
			Globals::g->gamekeydown[Globals::g->key_strafeleft] = 1;
*/
		Globals::g->joyymove = ev->data3; 
		return true;    // eat Globals::g->events 

	default: 
		break; 
	} 

	return false; 
} 



//
// G_Ticker
// Make ticcmd_ts for the Globals::g->players.
//
void G_Ticker (void) 
{ 
	int		i;
	int		buf; 
	ticcmd_t*	cmd;

	// do player reborns if needed
	for (i=0 ; i<MAXPLAYERS ; i++) 
		if (Globals::g->playeringame[i] && Globals::g->players[i].playerstate == PST_REBORN) 
			G_DoReborn (i);

	// do things to change the game state
	while (Globals::g->gameaction != ga_nothing) 
	{ 
		switch (Globals::g->gameaction) 
		{ 
		case ga_loadlevel: 
			G_DoLoadLevel (); 
			break; 
		case ga_newgame: 
			G_DoNewGame (); 
			break; 
		case ga_loadgame: 
			G_DoLoadGame ();
			break; 
		case ga_savegame: 
			G_DoSaveGame (); 
			break; 
		case ga_playdemo: 
			G_DoPlayDemo (); 
			break; 
		case ga_completed: 
			G_DoCompleted (); 
			break; 
		case ga_victory: 
			F_StartFinale (); 
			break; 
		case ga_worlddone: 
			G_DoWorldDone ();
			break; 
		case ga_screenshot: 
			M_ScreenShot (); 
			Globals::g->gameaction = ga_nothing; 
			break; 
		case ga_nothing: 
			break; 
		} 
	}

	// get commands, check Globals::g->consistancy,
	// and build new Globals::g->consistancy check
	buf = (Globals::g->gametic/Globals::g->ticdup)%BACKUPTICS; 

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if (Globals::g->playeringame[i]) 
		{ 
			cmd = &Globals::g->players[i].cmd; 

			memcpy (cmd, &Globals::g->netcmds[i][buf], sizeof(ticcmd_t)); 

			if ( Globals::g->demoplayback ) {
				G_ReadDemoTiccmd( cmd );
#ifdef DEBUG_DEMOS
				if ( demoDebugOn && testprndindex != Globals::g->prndindex && printErrorCount++ < 10 ) {
					I_Printf( "time: %d, g->prndindex(%d) does not match demo prndindex(%d)!\n", Globals::g->leveltime, Globals::g->prndindex, testprndindex );
				}
#endif
			}

			if ( Globals::g->demorecording ) {
				G_WriteDemoTiccmd (cmd);
			}

			// HACK ALERT ( the GS_FINALE CRAP IS A HACK.. )
			if (Globals::g->netgame && !Globals::g->netdemo && !(Globals::g->gametic % Globals::g->ticdup) && !(Globals::g->gamestate == GS_FINALE ) ) 
			{
				if (Globals::g->gametic > BACKUPTICS && Globals::g->consistancy[i][buf] != cmd->consistancy) 
				{
					printf ("consistency failure (%i should be %i)",
						cmd->consistancy, Globals::g->consistancy[i][buf]); 

					// TODO: If we ever support splitscreen and online,
					// we'll have to call D_QuitNetGame for all local players.
					D_QuitNetGame();

					/*session->QuitMatch();
					common->Dialog().AddDialog( GDM_CONNECTION_LOST_HOST, DIALOG_ACCEPT, NULL, NULL, false );*/
				}

				if (Globals::g->players[i].mo) 
					Globals::g->consistancy[i][buf] = Globals::g->players[i].mo->x; 
				else 
					Globals::g->consistancy[i][buf] = Globals::g->rndindex;
			} 
		}
	}

	// check for special buttons
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		if (Globals::g->playeringame[i]) 
		{ 
			if (Globals::g->players[i].cmd.buttons & BT_SPECIAL) 
			{ 
				switch (Globals::g->players[i].cmd.buttons & BT_SPECIALMASK) 
				{ 
				case BTS_PAUSE: 
					Globals::g->paused ^= 1;

					// DHM - Nerve :: Don't pause the music
					/*
					if (Globals::g->paused) 
						S_PauseSound (); 
					else 
						S_ResumeSound (); 
					*/
					break; 

				case BTS_SAVEGAME: 
					
					if (!Globals::g->savedescription[0]) 
						strcpy (Globals::g->savedescription, "NET GAME"); 
					Globals::g->savegameslot = (Globals::g->players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
					Globals::g->gameaction = ga_savegame; 
					
					break; 
				} 
			} 
		}
	}

	// do main actions
	switch (Globals::g->gamestate) 
	{ 
	case GS_LEVEL: 
		P_Ticker (); 
		ST_Ticker (); 
		AM_Ticker (); 
		HU_Ticker ();            
		break; 

	case GS_INTERMISSION: 
		WI_Ticker (); 
		break; 

	case GS_FINALE: 
		F_Ticker (); 
		break; 

	case GS_DEMOSCREEN: 
		D_PageTicker (); 
		break; 
	}        
} 


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer (int player) 
{ 
	player_t*	p; 

	// set up the saved info         
	p = &Globals::g->players[player]; 

	// clear everything else to defaults 
	G_PlayerReborn (player); 

} 



//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (int player) 
{ 
	player_t*	p; 

	p = &Globals::g->players[player]; 

	memset (p->powers, 0, sizeof (p->powers)); 
	memset (p->cards, 0, sizeof (p->cards)); 
	p->mo->flags &= ~MF_SHADOW;		// cancel invisibility 
	p->extralight = 0;			// cancel gun flashes 
	p->fixedcolormap = 0;		// cancel ir gogles 
	p->damagecount = 0;			// no palette changes 
	p->bonuscount = 0; 
} 

//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
void G_PlayerReborn (int player) 
{ 
	player_t*	p; 
	int		i; 
	int		frags[MAXPLAYERS]; 
	int		killcount;
	int		itemcount;
	int		secretcount;

	// DHM - Nerve :: cards are saved across death in coop multiplayer
	qboolean cards[NUMCARDS];
	bool hasMapPowerup = false;

	hasMapPowerup = Globals::g->players[player].powers[pw_allmap] != 0;
	memcpy( cards, Globals::g->players[player].cards, sizeof(cards) );
	memcpy( frags, Globals::g->players[player].frags, sizeof(frags) );
	killcount = Globals::g->players[player].killcount;
	itemcount = Globals::g->players[player].itemcount;
	secretcount = Globals::g->players[player].secretcount;

	p = &Globals::g->players[player];
	memset (p, 0, sizeof(*p));

	// DHM - Nerve :: restore cards in multiplayer
	// TODO: Networking
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if ( common->IsMultiplayer() || gameLocal->IsSplitscreen() || (Globals::g->demoplayback && Globals::g->netdemo) ) {
		if ( hasMapPowerup ) {
			Globals::g->players[player].powers[pw_allmap] = 1;
		}
		memcpy (Globals::g->players[player].cards, cards, sizeof(Globals::g->players[player].cards));
	}
#endif
	memcpy (Globals::g->players[player].frags, frags, sizeof(Globals::g->players[player].frags));
	Globals::g->players[player].killcount = killcount;
	Globals::g->players[player].itemcount = itemcount;
	Globals::g->players[player].secretcount = secretcount;

	p->usedown = p->attackdown = true;	// don't do anything immediately
	p->playerstate = PST_LIVE;
	p->health = MAXHEALTH;
	p->readyweapon = p->pendingweapon = wp_pistol;
	p->weaponowned[wp_fist] = true;
	p->weaponowned[wp_pistol] = true;
	p->ammo[am_clip] = 50;
	// TODO: PC
#if 0
	p->cheats = gameLocal->cheats;
#else
	p->cheats = 0;
#endif

	for (i=0 ; i<NUMAMMO ; i++)
		p->maxammo[i] = maxammo[i];
}

//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
void P_SpawnPlayer (mapthing_t* mthing); 

qboolean
G_CheckSpot
( int		playernum,
 mapthing_t*	mthing ) 
{ 
	fixed_t		x;
	fixed_t		y; 
	subsector_t*	ss; 
	unsigned		an; 
	mobj_t*		mo; 
	int			i;

	if (!Globals::g->players[playernum].mo)
	{
		// first spawn of level, before corpses
		for (i=0 ; i<playernum ; i++)
			if (Globals::g->players[i].mo->x == mthing->x << FRACBITS
				&& Globals::g->players[i].mo->y == mthing->y << FRACBITS)
				return false;	
		return true;
	}

	x = mthing->x << FRACBITS; 
	y = mthing->y << FRACBITS; 

	if (!P_CheckPosition (Globals::g->players[playernum].mo, x, y) ) 
		return false; 

	// flush an old corpse if needed 
	if (Globals::g->bodyqueslot >= BODYQUESIZE) 
		P_RemoveMobj (Globals::g->bodyque[Globals::g->bodyqueslot%BODYQUESIZE]); 
	Globals::g->bodyque[Globals::g->bodyqueslot%BODYQUESIZE] = Globals::g->players[playernum].mo; 
	Globals::g->bodyqueslot++; 

	// spawn a teleport fog 
	ss = R_PointInSubsector (x,y); 
	an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT; 

	mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an] 
	, ss->sector->floorheight 
		, MT_TFOG); 

	if (Globals::g->players[Globals::g->consoleplayer].viewz != 1 && (playernum == Globals::g->consoleplayer)) 
		S_StartSound (Globals::g->players[Globals::g->consoleplayer].mo, sfx_telept);	// don't start sound on first frame 

	return true; 
} 


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
void G_DeathMatchSpawnPlayer (int playernum) 
{ 
	int             i,j; 
	int				selections; 

	selections = Globals::g->deathmatch_p - Globals::g->deathmatchstarts; 
	if (selections < 4) 
		I_Error ("Only %i Globals::g->deathmatch spots, 4 required", selections); 

	for (j=0 ; j<20 ; j++) 
	{ 
		i = P_Random() % selections; 
		if (G_CheckSpot (playernum, &Globals::g->deathmatchstarts[i]) ) 
		{ 
			Globals::g->deathmatchstarts[i].type = playernum+1; 
			P_SpawnPlayer (&Globals::g->deathmatchstarts[i]); 
			return; 
		} 
	} 

	// no good spot, so the player will probably get stuck 
	P_SpawnPlayer (&Globals::g->playerstarts[playernum]); 
} 


//
// G_DoReborn 
// 
void G_DoReborn (int playernum) 
{ 
	int                             i; 

	if (!Globals::g->netgame)
	{
		// reload the level from scratch
		Globals::g->gameaction = ga_loadlevel;  
	}
	else 
	{
		// respawn at the start

		// first dissasociate the corpse 
		Globals::g->players[playernum].mo->player = NULL;   

		// spawn at random spot if in death match 
		if (Globals::g->deathmatch) 
		{ 
			G_DeathMatchSpawnPlayer (playernum); 
			return; 
		} 

		if (G_CheckSpot (playernum, &Globals::g->playerstarts[playernum]) ) 
		{ 
			P_SpawnPlayer (&Globals::g->playerstarts[playernum]); 
			return; 
		}

		// try to spawn at one of the other Globals::g->players spots 
		for (i=0 ; i<MAXPLAYERS ; i++)
		{
			if (G_CheckSpot (playernum, &Globals::g->playerstarts[i]) ) 
			{ 
				Globals::g->playerstarts[i].type = playernum+1;	// fake as other player 
				P_SpawnPlayer (&Globals::g->playerstarts[i]); 
				Globals::g->playerstarts[i].type = i+1;		// restore 
				return; 
			}	    
			// he's going to be inside something.  Too bad.
		}
		P_SpawnPlayer (&Globals::g->playerstarts[playernum]); 
	} 
} 


void G_ScreenShot (void) 
{ 
	Globals::g->gameaction = ga_screenshot; 
} 


// DHM - Nerve :: Added episode 4 par times
// DOOM Par Times
const int pars[5][10] = 
{ 
	{0}, 
	{0,30,75,120,90,165,180,180,30,165},
	{0,90,90,90,120,90,360,240,30,170},
	{0,90,45,90,150,90,90,165,30,135},
	{0,165,255,135,150,180,390,135,360,180}
}; 

// DOOM II Par Times
const int cpars[32] =
{
	30,90,120,120,90,150,120,120,270,90,		//  1-10
	210,150,150,150,210,150,420,150,210,150,	// 11-20
	240,150,180,150,150,300,330,420,300,180,	// 21-30
	120,30										// 31-32
};


//
// G_DoCompleted 
//

void G_ExitLevel (void) 
{ 
	Globals::g->secretexit = false; 
	Globals::g->gameaction = ga_completed; 
} 

// Here's for the german edition.
void G_SecretExitLevel (void) 
{ 
	// IF NO WOLF3D LEVELS, NO SECRET EXIT!
	if ( (Globals::g->gamemode == commercial)
		&& (W_CheckNumForName("map31")<0))
		Globals::g->secretexit = false;
	else
		Globals::g->secretexit = true; 
	Globals::g->gameaction = ga_completed; 
} 

void G_DoCompleted (void) 
{ 
	int             i; 

	Globals::g->gameaction = ga_nothing; 

	for (i=0 ; i<MAXPLAYERS ; i++) {
		if (Globals::g->playeringame[i]) {
			G_PlayerFinishLevel (i);        // take away cards and stuff
		}
	}

	if (Globals::g->automapactive) {
		AM_Stop();
	}

	if ( Globals::g->demoplayback ) {
		G_CheckDemoStatus();
		return;
	}

	if ( Globals::g->demorecording ) {
		G_CheckDemoStatus();
	}

	// DHM - Nerve :: Deathmatch doesn't go to finale screen, just do intermission
	if ( Globals::g->gamemode != commercial && !Globals::g->deathmatch ) {
		switch(Globals::g->gamemap) {

		case 8:
			Globals::g->gameaction = ga_victory;
			return;
		case 9: 
			for (i=0 ; i<MAXPLAYERS ; i++) 
				Globals::g->players[i].didsecret = true; 
			break;
		}
	}

	Globals::g->wminfo.didsecret = Globals::g->players[Globals::g->consoleplayer].didsecret; 
	Globals::g->wminfo.epsd = Globals::g->gameepisode -1; 
	Globals::g->wminfo.last = Globals::g->gamemap -1;

	// Globals::g->wminfo.next is 0 biased, unlike Globals::g->gamemap
	if ( Globals::g->gamemode == commercial)
	{
		if (Globals::g->secretexit) {
			if ( Globals::g->gamemission == doom2 ) {
				switch(Globals::g->gamemap)
				{
					case 15: Globals::g->wminfo.next = 30; break;
					case 31: Globals::g->wminfo.next = 31; break;
				}
			} else if( Globals::g->gamemission == pack_nerve ) {

				// DHM - Nerve :: Secret level is always level 9 on extra Doom2 missions
				Globals::g->wminfo.next = 8;
			}
		}
		else {
			if ( Globals::g->gamemission == doom2 ) {
				switch(Globals::g->gamemap)
				{
					case 31:
					case 32: Globals::g->wminfo.next = 15; break;
					default: Globals::g->wminfo.next = Globals::g->gamemap;
				}
			}
			else if( Globals::g->gamemission == pack_nerve) {
				switch(Globals::g->gamemap)
				{	case 9:
						Globals::g->wminfo.next = 4;
						break;
					default:
						Globals::g->wminfo.next = Globals::g->gamemap;
						break;
				}
			} else {
				Globals::g->wminfo.next = Globals::g->gamemap;
			}
		}
	}
	else
	{
		if (Globals::g->secretexit) { 
			Globals::g->wminfo.next = 8; 	// go to secret level 
		}
		else if (Globals::g->gamemap == 9 ) 
		{
			// returning from secret level 
			switch (Globals::g->gameepisode) 
			{ 
			case 1: 
				Globals::g->wminfo.next = 3; 
				break; 
			case 2: 
				Globals::g->wminfo.next = 5; 
				break; 
			case 3: 
				Globals::g->wminfo.next = 6; 
				break; 
			case 4:
				Globals::g->wminfo.next = 2;
				break;
			}                
		} 
		else 
			Globals::g->wminfo.next = Globals::g->gamemap;          // go to next level 
	}

	// DHM - Nerve :: In deathmatch, repeat the current level.  User must exit and choose a new level.
	if ( Globals::g->deathmatch ) {
		Globals::g->wminfo.next = Globals::g->wminfo.last;
	}

	Globals::g->wminfo.maxkills = Globals::g->totalkills; 
	Globals::g->wminfo.maxitems = Globals::g->totalitems; 
	Globals::g->wminfo.maxsecret = Globals::g->totalsecret; 
	Globals::g->wminfo.maxfrags = 0; 

	if ( Globals::g->gamemode == commercial ) {
		Globals::g->wminfo.partime = TICRATE *cpars[Globals::g->gamemap-1];
	}
	else
		Globals::g->wminfo.partime = TICRATE * pars[Globals::g->gameepisode][Globals::g->gamemap]; 

	Globals::g->wminfo.pnum = Globals::g->consoleplayer; 

	for (i=0 ; i<MAXPLAYERS ; i++) 
	{ 
		Globals::g->wminfo.plyr[i].in = Globals::g->playeringame[i]; 
		Globals::g->wminfo.plyr[i].skills = Globals::g->players[i].killcount; 
		Globals::g->wminfo.plyr[i].sitems = Globals::g->players[i].itemcount; 
		Globals::g->wminfo.plyr[i].ssecret = Globals::g->players[i].secretcount; 
		Globals::g->wminfo.plyr[i].stime = Globals::g->leveltime; 
		memcpy (Globals::g->wminfo.plyr[i].frags, Globals::g->players[i].frags 
			, sizeof(Globals::g->wminfo.plyr[i].frags)); 
	} 

	Globals::g->gamestate = GS_INTERMISSION; 
	Globals::g->viewactive = false; 
	Globals::g->automapactive = false; 

	WI_Start (&Globals::g->wminfo); 
} 


//
// G_WorldDone 
//
void G_WorldDone (void) 
{ 
	Globals::g->gameaction = ga_worlddone; 

	if (Globals::g->secretexit) 
		Globals::g->players[Globals::g->consoleplayer].didsecret = true; 

	if ( Globals::g->gamemode == commercial )
	{
		if ( Globals::g->gamemission == doom2 || Globals::g->gamemission == pack_tnt || Globals::g->gamemission == pack_plut  ) {
			switch (Globals::g->gamemap)
			{
			case 15:
			case 31:
				if (!Globals::g->secretexit)
					break;
			case 6:
			case 11:
			case 20:
			case 30:
				F_StartFinale ();
				break;
			}
		}
		else if ( Globals::g->gamemission == pack_nerve ) {
			if ( Globals::g->gamemap == 8 ) {
				F_StartFinale();
			}
		}
		else if ( Globals::g->gamemission == pack_master ) {
			if ( Globals::g->gamemap == 21 ) {
				F_StartFinale();
			}
		}
		else {
			// DHM - NERVE :: Downloadable content needs to set these up if different than initial extended episode
			if ( Globals::g->gamemap == 8 ) {
				F_StartFinale();
			}
		}
	}
} 

void G_DoWorldDone (void) 
{        
	Globals::g->gamestate = GS_LEVEL;

	Globals::g->gamemap = Globals::g->wminfo.next+1;

	M_ClearRandom();

	for ( int i = 0; i < MAXPLAYERS; i++ ) {
		if ( Globals::g->playeringame[i] ) {
			Globals::g->players[i].usedown = Globals::g->players[i].attackdown = true;	// don't do anything immediately
		}
	}

	G_DoLoadLevel (); 
	Globals::g->gameaction = ga_nothing; 
	Globals::g->viewactive = true; 

} 



//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
void R_ExecuteSetViewSize (void);


void G_LoadGame (char* name) 
{ 
	strcpy (Globals::g->savename, name); 
	Globals::g->gameaction = ga_loadgame; 
} 

qboolean G_DoLoadGame () 
{ 
	int		i; 
	int		a,b,c;
	char	vcheck[VERSIONSIZE]; 

	loadingGame = true;

	Globals::g->gameaction = ga_nothing; 

	M_ReadFile (Globals::g->savename, &Globals::g->savebuffer); 

	waitingForWipe = true;

	// DHM - Nerve :: Clear possible net demo state
	Globals::g->netdemo = false;
	Globals::g->netgame = false;
	Globals::g->deathmatch = false;
	Globals::g->playeringame[1] = Globals::g->playeringame[2] = Globals::g->playeringame[3] = 0;
	Globals::g->respawnparm = false;
	Globals::g->fastparm = false;
	Globals::g->nomonsters = false;
	Globals::g->consoleplayer = 0;

	Globals::g->save_p = Globals::g->savebuffer + SAVESTRINGSIZE;

	// skip the description field 
	memset (vcheck,0,sizeof(vcheck)); 
	sprintf (vcheck,"version %i",VERSION); 
	if (strcmp ((char *)Globals::g->save_p, vcheck)) {
		loadingGame = false;
		waitingForWipe = false;

		return false;				// bad version
	}

	Globals::g->save_p += VERSIONSIZE; 

	Globals::g->gameskill = (skill_t)*Globals::g->save_p++; 
	Globals::g->gameepisode = *Globals::g->save_p++; 
	Globals::g->gamemission = *Globals::g->save_p++;
	Globals::g->gamemap = *Globals::g->save_p++; 
	for (i=0 ; i<MAXPLAYERS ; i++) 
		Globals::g->playeringame[i] = *Globals::g->save_p++; 

	// load a base level 
	G_InitNew (Globals::g->gameskill, Globals::g->gameepisode, Globals::g->gamemap ); 

	// get the times 
	a = *Globals::g->save_p++; 
	b = *Globals::g->save_p++; 
	c = *Globals::g->save_p++; 
	Globals::g->leveltime = (a<<16) + (b<<8) + c; 

	// dearchive all the modifications
	P_UnArchivePlayers (); 
	P_UnArchiveWorld (); 
	P_UnArchiveThinkers ();

	// specials are archived with thinkers
	//P_UnArchiveSpecials (); 

	if (*Globals::g->save_p != 0x1d) 
		I_Error ("Bad savegame");

	if (Globals::g->setsizeneeded)
		R_ExecuteSetViewSize ();

	// draw the pattern into the back screen
	R_FillBackScreen ();

	loadingGame = false;

	Z_Free(Globals::g->savebuffer);

	return true;
} 


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 unsigned char text string 
//
void
G_SaveGame
( int	slot,
 char*	description ) 
{ 
	Globals::g->savegameslot = slot; 
	strcpy (Globals::g->savedescription, description); 
	Globals::g->sendsave = true; 
	Globals::g->gameaction = ga_savegame;
} 

qboolean G_DoSaveGame (void) 
{ 
	char	name[100]; 
	char	name2[VERSIONSIZE]; 
	char*	description; 
	int		length; 
	int		i; 
	qboolean	bResult = true;

	if ( Globals::g->gamestate != GS_LEVEL ) {
		return false;
	}

	description = Globals::g->savedescription; 

	/*if( common->GetCurrentGame() == DOOM_CLASSIC ) {
		sprintf(name,"DOOM\\%s%d.dsg", SAVEGAMENAME,Globals::g->savegameslot );
	} else {
		if( DoomLib::expansionSelected == doom2 ) {
			sprintf(name,"DOOM2\\%s%d.dsg", SAVEGAMENAME,Globals::g->savegameslot );
		} else {
			sprintf(name,"DOOM2_NRFTL\\%s%d.dsg", SAVEGAMENAME,Globals::g->savegameslot );
		}

	}*/

	Globals::g->save_p = Globals::g->savebuffer = Globals::g->screens[1];

	memcpy (Globals::g->save_p, description, SAVESTRINGSIZE); 
	Globals::g->save_p += SAVESTRINGSIZE; 

	memset (name2,0,sizeof(name2)); 
	sprintf (name2,"version %i",VERSION); 
	memcpy (Globals::g->save_p, name2, VERSIONSIZE); 
	Globals::g->save_p += VERSIONSIZE; 

	*Globals::g->save_p++ = Globals::g->gameskill; 
	*Globals::g->save_p++ = Globals::g->gameepisode; 
	*Globals::g->save_p++ = Globals::g->gamemission;
	*Globals::g->save_p++ = Globals::g->gamemap; 

	for (i=0 ; i<MAXPLAYERS ; i++) {
		*Globals::g->save_p++ = Globals::g->playeringame[i];
	}

	*Globals::g->save_p++ = Globals::g->leveltime>>16; 
	*Globals::g->save_p++ = Globals::g->leveltime>>8; 
	*Globals::g->save_p++ = Globals::g->leveltime; 

	P_ArchivePlayers (); 
	P_ArchiveWorld (); 
	P_ArchiveThinkers ();

	// specials are archived with thinkers
	//P_ArchiveSpecials (); 

	*Globals::g->save_p++ = 0x1d;		// Globals::g->consistancy marker 

	length = Globals::g->save_p - Globals::g->savebuffer; 
	if (length > SAVEGAMESIZE) 
		I_Error ("Savegame buffer overrun");

	Globals::g->savebufferSize = length;
	
	M_WriteFile (name, Globals::g->savebuffer, length); 

	Globals::g->gameaction = ga_nothing; 
	Globals::g->savedescription[0] = 0;		 

	// draw the pattern into the back screen
	R_FillBackScreen ();

	return bResult;
} 


//
// G_InitNew
// Can be called by the startup code or the menu task,
// Globals::g->consoleplayer, Globals::g->displayplayer, Globals::g->playeringame[] should be set. 
//

void
G_DeferedInitNew
( skill_t	skill,
 int		episode,
 int		map) 
{ 
	Globals::g->d_skill = skill; 
	Globals::g->d_episode = episode; 
	Globals::g->d_map = map;

	//Globals::g->d_map = 30;

	Globals::g->gameaction = ga_newgame; 
} 


void G_DoNewGame (void) 
{
	Globals::g->demoplayback = false; 
	Globals::g->netdemo = false;
	Globals::g->netgame = false;
	Globals::g->deathmatch = false;
	Globals::g->playeringame[1] = Globals::g->playeringame[2] = Globals::g->playeringame[3] = 0;
	Globals::g->respawnparm = false;
	Globals::g->fastparm = false;
	Globals::g->nomonsters = false;
	Globals::g->consoleplayer = 0;
	G_InitNew (Globals::g->d_skill, Globals::g->d_episode, Globals::g->d_map ); 
	Globals::g->gameaction = ga_nothing; 
} 

// The sky texture to be used instead of the F_SKY1 dummy.


void
G_InitNew
( skill_t	skill,
 int		episode,
 int		map
 ) 
{ 
	int i; 
	//m_inDemoMode.SetBool( false );
	R_SetViewSize (Globals::g->screenblocks, Globals::g->detailLevel);

	if (Globals::g->paused) 
	{ 
		Globals::g->paused = false; 
		S_ResumeSound (); 
	} 

	if (skill > sk_nightmare) 
		skill = sk_nightmare;

	// This was quite messy with SPECIAL and commented parts.
	// Supposedly hacks to make the latest edition work.
	// It might not work properly.
	if (episode < 1)
		episode = 1; 

	if ( Globals::g->gamemode == retail )
	{
		if (episode > 4)
			episode = 4;
	}
	else if ( Globals::g->gamemode == shareware )
	{
		if (episode > 1) 
			episode = 1;	// only start episode 1 on shareware
	}  
	else
	{
		if (episode > 3)
			episode = 3;
	}

	if (map < 1) 
		map = 1;

	if (skill == sk_nightmare || Globals::g->respawnparm )
		Globals::g->respawnmonsters = true;
	else
		Globals::g->respawnmonsters = false;

	// force Globals::g->players to be initialized upon first level load         
	for (i=0 ; i<MAXPLAYERS ; i++) 
		Globals::g->players[i].playerstate = PST_REBORN; 

	Globals::g->usergame = true;                // will be set false if a demo 
	Globals::g->paused = false; 
	Globals::g->demoplayback = false;
	Globals::g->advancedemo = false;
	Globals::g->automapactive = false; 
	Globals::g->viewactive = true; 
	Globals::g->gameepisode = episode; 
	//Globals::g->gamemission = expansion->pack_type;
	Globals::g->gamemap = map; 
	Globals::g->gameskill = skill; 

	Globals::g->viewactive = true;

	// set the sky map for the episode
	if ( Globals::g->gamemode == commercial)
	{
		Globals::g->skytexture = R_TextureNumForName ("SKY3");

		if (Globals::g->gamemap < 12) {
			Globals::g->skytexture = R_TextureNumForName ("SKY1");
		}
		else if (Globals::g->gamemap < 21) {
			Globals::g->skytexture = R_TextureNumForName ("SKY2");
		}
	}
	else {
		switch (episode) 
		{ 
		case 1: 
			Globals::g->skytexture = R_TextureNumForName ("SKY1"); 
			break; 
		case 2: 
			Globals::g->skytexture = R_TextureNumForName ("SKY2"); 
			break; 
		case 3: 
			Globals::g->skytexture = R_TextureNumForName ("SKY3"); 
			break; 
		case 4:	// Special Edition sky
			Globals::g->skytexture = R_TextureNumForName ("SKY4");
			break;
		default:
			Globals::g->skytexture = R_TextureNumForName ("SKY1");
			break;
		}
	}

	G_DoLoadLevel( );
} 


//
// DEMO RECORDING 
// 

void G_ReadDemoTiccmd (ticcmd_t* cmd) 
{ 
	if (*Globals::g->demo_p == DEMOMARKER) 
	{
		// end of demo data stream 
		G_CheckDemoStatus (); 
		return; 
	}

	cmd->forwardmove = ((signed char)*Globals::g->demo_p++);
	cmd->sidemove = ((signed char)*Globals::g->demo_p++);

	if ( demoversion == VERSION ) {
		short *temp = (short *)(Globals::g->demo_p);
		cmd->angleturn = *temp;
		Globals::g->demo_p += 2;
	}
	else {
		// DHM - Nerve :: Old format
		cmd->angleturn = ((unsigned char)*Globals::g->demo_p++)<<8;
	}

	cmd->buttons = (unsigned char)*Globals::g->demo_p++;

#ifdef DEBUG_DEMOS
	// TESTING
	if ( demoDebugOn ) {
		testprndindex = (unsigned char)*Globals::g->demo_p++;
	}
#endif
} 


void G_WriteDemoTiccmd (ticcmd_t* cmd) 
{ 
	*Globals::g->demo_p++ = cmd->forwardmove; 
	*Globals::g->demo_p++ = cmd->sidemove;

	// NEW VERSION
	short *temp = (short *)(Globals::g->demo_p);
	*temp = cmd->angleturn;
	Globals::g->demo_p += 2;

	// OLD VERSION
	//*Globals::g->demo_p++ = (cmd->angleturn+128)>>8; 

	*Globals::g->demo_p++ = cmd->buttons;

	int cmdSize = 5;

#ifdef DEBUG_DEMOS_WRITE
	// TESTING
	*Globals::g->demo_p++ = Globals::g->prndindex;
	cmdSize++;
#endif

	Globals::g->demo_p -= cmdSize; 
	if (Globals::g->demo_p > Globals::g->demoend - (cmdSize * 4))
	{
		// no more space 
		G_CheckDemoStatus (); 
		return; 
	} 

	G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same 
} 



//
// G_RecordDemo 
// 
void G_RecordDemo (char* name) 
{ 
	//Globals::g->usergame = false; 
	strcpy( Globals::g->demoname, name ); 
	strcat( Globals::g->demoname, ".lmp" );

	Globals::g->demobuffer = new unsigned char[ MAXDEMOSIZE ];
	Globals::g->demoend = Globals::g->demobuffer + MAXDEMOSIZE;

	demoversion = VERSION;
	Globals::g->demorecording = true;
} 
 
 
void G_BeginRecording (void) 
{ 
	int             i;

	Globals::g->demo_p = Globals::g->demobuffer;

#ifdef DEBUG_DEMOS
#ifdef DEBUG_DEMOS_WRITE
	demoDebugOn = true;
	*Globals::g->demo_p++ = VERSION + 1;
#else
	*Globals::g->demo_p++ = VERSION;
#endif
#endif
	*Globals::g->demo_p++ = Globals::g->gameskill;
	*Globals::g->demo_p++ = Globals::g->gameepisode;
	*Globals::g->demo_p++ = Globals::g->gamemission;
	*Globals::g->demo_p++ = Globals::g->gamemap;
	*Globals::g->demo_p++ = Globals::g->deathmatch;
	*Globals::g->demo_p++ = Globals::g->respawnparm;
	*Globals::g->demo_p++ = Globals::g->fastparm;
	*Globals::g->demo_p++ = Globals::g->nomonsters;
	*Globals::g->demo_p++ = Globals::g->consoleplayer;

	for ( i=0 ; i<MAXPLAYERS ; i++ ) {
		*Globals::g->demo_p++ = Globals::g->playeringame[i];
	}

	for ( i=0 ; i<MAXPLAYERS ; i++ ) {
		// Archive player state to demo
		if ( Globals::g->playeringame[i] ) {
		    int* dest = (int *)Globals::g->demo_p;
			*dest++ = Globals::g->players[i].health;
			*dest++ = Globals::g->players[i].armorpoints;
			*dest++ = Globals::g->players[i].armortype;
			*dest++ = Globals::g->players[i].readyweapon;
			for ( int j = 0; j < NUMWEAPONS; j++ ) {
				*dest++ = Globals::g->players[i].weaponowned[j];
			}
			for ( int j = 0; j < NUMAMMO; j++ ) {
				*dest++ = Globals::g->players[i].ammo[j];
				*dest++ = Globals::g->players[i].maxammo[j];
			}
			Globals::g->demo_p = (unsigned char *)dest;
		}
	}
}

//
// G_PlayDemo 
//
void G_DeferedPlayDemo (char* name) 
{ 
	Globals::g->defdemoname = name; 
	Globals::g->gameaction = ga_playdemo; 
} 

void G_DoPlayDemo (void) 
{ 
	skill_t skill; 
	int             i, episode, map, mission; 

	Globals::g->gameaction = ga_nothing;

	// TODO: Networking
#if ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if ( gameLocal->IsSplitscreen() && DoomLib::GetPlayer() > 0 ) {
		return;
	}
#endif
	

	// DEMO Testing
	bool useOriginalDemo = true;

	if ( useOriginalDemo ) {
		int demolump = W_GetNumForName( Globals::g->defdemoname );
		int demosize = W_LumpLength( demolump );

		Globals::g->demobuffer = Globals::g->demo_p = new unsigned char[ demosize ];
		W_ReadLump( demolump, Globals::g->demobuffer );
	}

	// DHM - Nerve :: We support old and new demo versions
	demoversion = *Globals::g->demo_p++;

	skill = (skill_t)*Globals::g->demo_p++; 
	episode = *Globals::g->demo_p++;
	if ( demoversion == VERSION ) {
		mission =  *Globals::g->demo_p++;
	}
	else {
		mission = 0;
	}
	map = *Globals::g->demo_p++; 
	Globals::g->deathmatch = *Globals::g->demo_p++;
	Globals::g->respawnparm = *Globals::g->demo_p++;
	Globals::g->fastparm = *Globals::g->demo_p++;
	Globals::g->nomonsters = *Globals::g->demo_p++;
	Globals::g->consoleplayer = *Globals::g->demo_p++;

	for ( i=0 ; i<MAXPLAYERS ; i++ ) {
		Globals::g->playeringame[i] = *Globals::g->demo_p++;
	}

	Globals::g->netgame = false;
	Globals::g->netdemo = false; 
	if (Globals::g->playeringame[1]) 
	{ 
		Globals::g->netgame = true;
		Globals::g->netdemo = true; 
	}

	// don't spend a lot of time in loadlevel 
	Globals::g->precache = false;
	G_InitNew (skill, episode, map ); 
	R_SetViewSize (Globals::g->screenblocks + 1, Globals::g->detailLevel);
	//m_inDemoMode.SetBool( true );

	// JAF - Dont show messages when in Demo Mode. Globals::g->showMessages = false;
	Globals::g->precache = true; 

	// DHM - Nerve :: We now read in the player state from the demo
	if ( demoversion == VERSION ) {
		for ( i=0 ; i<MAXPLAYERS ; i++ ) {
			if ( Globals::g->playeringame[i] ) {
				int* src = (int *)Globals::g->demo_p;
				Globals::g->players[i].health = *src++;
				Globals::g->players[i].mo->health = Globals::g->players[i].health;
				Globals::g->players[i].armorpoints = *src++;
				Globals::g->players[i].armortype = *src++;
				Globals::g->players[i].readyweapon = (weapontype_t)*src++;
				for ( int j = 0; j < NUMWEAPONS; j++ ) {
					Globals::g->players[i].weaponowned[j] = *src++;
				}
				for ( int j = 0; j < NUMAMMO; j++ ) {
					Globals::g->players[i].ammo[j] = *src++;
					Globals::g->players[i].maxammo[j] = *src++;
				}
				Globals::g->demo_p = (unsigned char *)src;

				P_SetupPsprites( &Globals::g->players[i] );
			}
		}
	}

	Globals::g->usergame = false;
	Globals::g->demoplayback = true;
} 

//
// G_TimeDemo 
//
void G_TimeDemo (char* name) 
{ 	 
	Globals::g->nodrawers = M_CheckParm ("-nodraw"); 
	Globals::g->noblit = M_CheckParm ("-noblit"); 
	Globals::g->timingdemo = true; 
	Globals::g->singletics = true; 

	Globals::g->defdemoname = name; 
	Globals::g->gameaction = ga_playdemo; 
} 


/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/ 

qboolean G_CheckDemoStatus (void) 
{
	if (Globals::g->demoplayback) 
	{ 
		delete Globals::g->demobuffer;
		Globals::g->demobuffer = NULL;
		Globals::g->demo_p = NULL;
		Globals::g->demoend = NULL;

		Globals::g->demoplayback = false; 
		Globals::g->netdemo = false;
		Globals::g->netgame = false;
		Globals::g->deathmatch = false;
		Globals::g->playeringame[1] = Globals::g->playeringame[2] = Globals::g->playeringame[3] = 0;
		Globals::g->respawnparm = false;
		Globals::g->fastparm = false;
		Globals::g->nomonsters = false;
		Globals::g->consoleplayer = 0;
		D_AdvanceDemo (); 
		return true; 
	} 

	/*
	if (Globals::g->demorecording) { 
		*Globals::g->demo_p++ = DEMOMARKER; 

		if ( Globals::g->leveltime > (TICRATE * 9) ) {
			gameLocal->DoomStoreDemoBuffer( gameLocal->GetPortForPlayer( DoomLib::GetPlayer() ), Globals::g->demobuffer, Globals::g->demo_p - Globals::g->demobuffer );
		}

		delete Globals::g->demobuffer;
		Globals::g->demobuffer = NULL;
		Globals::g->demo_p = NULL;
		Globals::g->demoend = NULL;

		Globals::g->demorecording = false; 
    }
	*/

	return false; 
} 




