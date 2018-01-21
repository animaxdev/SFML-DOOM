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


#include "m_menu.hpp"
#include "i_system.hpp"
#include "i_video.hpp"
#include "i_net.hpp"
#include "g_game.hpp"
#include "doomdef.hpp"
#include "doomstat.hpp"

#include "doomlib.hpp"
#include "Main.hpp"
//#include "d3xp/Game_local.hpp"


void I_GetEvents( controller_t * );
void D_ProcessEvents (void); 
void G_BuildTiccmd (ticcmd_t *cmd, int newTics ); 
void D_DoAdvanceDemo (void);

extern bool globalNetworking;

//
// NETWORKING
//
// Globals::g->gametic is the tic about to (or currently being) run
// Globals::g->maketic is the tick that hasn't had control made for it yet
// Globals::g->nettics[] has the maketics for all Globals::g->players 
//
// a Globals::g->gametic cannot be run until Globals::g->nettics[] > Globals::g->gametic for all Globals::g->players
//




#define NET_TIMEOUT	1 * TICRATE





//
//
//
int NetbufferSize (void)
{
    // hrm...
	//int size = (int)&(((doomdata_t *)0)->cmds[Globals::g->netbuffer->numtics]);

	//return size;
    return 0;
}

//
// Checksum 
//
unsigned NetbufferChecksum (void)
{
	unsigned		c;
	int		i,l;

	c = 0x1234567;

    /*
	if ( globalNetworking ) {
		l = (NetbufferSize () - (int)&(((doomdata_t *)0)->retransmitfrom))/4;
		for (i=0 ; i<l ; i++)
			c += ((unsigned *)&Globals::g->netbuffer->retransmitfrom)[i] * (i+1);
	}*/

	return c & NCMD_CHECKSUM;
}

//
//
//
int ExpandTics (int low)
{
	int	delta;

	delta = low - (Globals::g->maketic&0xff);

	if (delta >= -64 && delta <= 64)
		return (Globals::g->maketic&~0xff) + low;
	if (delta > 64)
		return (Globals::g->maketic&~0xff) - 256 + low;
	if (delta < -64)
		return (Globals::g->maketic&~0xff) + 256 + low;

	I_Error ("ExpandTics: strange value %i at Globals::g->maketic %i",low,Globals::g->maketic);
	return 0;
}



//
// HSendPacket
//
void
HSendPacket
(int	node,
 int	flags )
{
	Globals::g->netbuffer->checksum = NetbufferChecksum () | flags;

	if (!node)
	{
		Globals::g->reboundstore = *Globals::g->netbuffer;
		Globals::g->reboundpacket = true;
		return;
	}

	if (Globals::g->demoplayback)
		return;

	if (!Globals::g->netgame)
		I_Error ("Tried to transmit to another node");

	Globals::g->doomcom.command = CMD_SEND;
	Globals::g->doomcom.remotenode = node;
	Globals::g->doomcom.datalength = NetbufferSize ();

	if (Globals::g->debugfile)
	{
		int		i;
		int		realretrans;
		if (Globals::g->netbuffer->checksum & NCMD_RETRANSMIT)
			realretrans = ExpandTics (Globals::g->netbuffer->retransmitfrom);
		else
			realretrans = -1;

		fprintf (Globals::g->debugfile,"send (%i + %i, R %i) [%i] ",
			ExpandTics(Globals::g->netbuffer->starttic),
			Globals::g->netbuffer->numtics, realretrans, Globals::g->doomcom.datalength);

		for (i=0 ; i < Globals::g->doomcom.datalength ; i++)
			fprintf (Globals::g->debugfile,"%i ",((unsigned char *)Globals::g->netbuffer)[i]);

		fprintf (Globals::g->debugfile,"\n");
	}

	I_NetCmd ();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
qboolean HGetPacket (void)
{	
	if (Globals::g->reboundpacket)
	{
		*Globals::g->netbuffer = Globals::g->reboundstore;
		Globals::g->doomcom.remotenode = 0;
		Globals::g->reboundpacket = false;
		return true;
	}

	if (!Globals::g->netgame)
		return false;

	if (Globals::g->demoplayback)
		return false;

	Globals::g->doomcom.command = CMD_GET;
	I_NetCmd ();

	if (Globals::g->doomcom.remotenode == -1)
		return false;

	if (Globals::g->doomcom.datalength != NetbufferSize ())
	{
		if (Globals::g->debugfile)
			fprintf (Globals::g->debugfile,"bad packet length %i\n",Globals::g->doomcom.datalength);
		return false;
	}

	// ALAN NETWORKING -- this fails a lot on 4 player split debug!!
	// TODO: Networking
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if ( !gameLocal->IsSplitscreen() && NetbufferChecksum() != (Globals::g->netbuffer->checksum&NCMD_CHECKSUM) )
	{
		if (Globals::g->debugfile) {
			fprintf (Globals::g->debugfile,"bad packet checksum\n");
		}

		return false;
	}
#endif

	if (Globals::g->debugfile)
	{
		int		realretrans;
		int	i;

		if (Globals::g->netbuffer->checksum & NCMD_SETUP)
			fprintf (Globals::g->debugfile,"setup packet\n");
		else
		{
			if (Globals::g->netbuffer->checksum & NCMD_RETRANSMIT)
				realretrans = ExpandTics (Globals::g->netbuffer->retransmitfrom);
			else
				realretrans = -1;

			fprintf (Globals::g->debugfile,"get %i = (%i + %i, R %i)[%i] ",
				Globals::g->doomcom.remotenode,
				ExpandTics(Globals::g->netbuffer->starttic),
				Globals::g->netbuffer->numtics, realretrans, Globals::g->doomcom.datalength);

			for (i=0 ; i < Globals::g->doomcom.datalength ; i++)
				fprintf (Globals::g->debugfile,"%i ",((unsigned char *)Globals::g->netbuffer)[i]);
			fprintf (Globals::g->debugfile,"\n");
		}
	}
	return true;	
}


//
// GetPackets
//

void GetPackets (void)
{
	int		netconsole;
	int		netnode;
	ticcmd_t	*src, *dest;
	int		realend;
	int		realstart;

	while ( HGetPacket() )
	{
		if (Globals::g->netbuffer->checksum & NCMD_SETUP)
			continue;		// extra setup packet

		netconsole = Globals::g->netbuffer->player & ~PL_DRONE;
		netnode = Globals::g->doomcom.remotenode;

		// to save unsigned chars, only the low unsigned char of tic numbers are sent
		// Figure out what the rest of the unsigned chars are
		realstart = ExpandTics (Globals::g->netbuffer->starttic);		
		realend = (realstart+Globals::g->netbuffer->numtics);

		// check for exiting the game
		if (Globals::g->netbuffer->checksum & NCMD_EXIT)
		{
			if (!Globals::g->nodeingame[netnode])
				continue;
			Globals::g->nodeingame[netnode] = false;
			Globals::g->playeringame[netconsole] = false;
			strcpy (Globals::g->exitmsg, "Player 1 left the game");
			Globals::g->exitmsg[7] += netconsole;
			Globals::g->players[Globals::g->consoleplayer].message = Globals::g->exitmsg;

			if( Globals::g->demorecording ) {
				G_CheckDemoStatus();
			}
			continue;
		}

		// check for a remote game kill
/*
		if (Globals::g->netbuffer->checksum & NCMD_KILL)
			I_Error ("Killed by network driver");
*/

		Globals::g->nodeforplayer[netconsole] = netnode;

		// check for retransmit request
		if ( Globals::g->resendcount[netnode] <= 0 
			&& (Globals::g->netbuffer->checksum & NCMD_RETRANSMIT) )
		{
			Globals::g->resendto[netnode] = ExpandTics(Globals::g->netbuffer->retransmitfrom);
			if (Globals::g->debugfile)
				fprintf (Globals::g->debugfile,"retransmit from %i\n", Globals::g->resendto[netnode]);
			Globals::g->resendcount[netnode] = RESENDCOUNT;
		}
		else
			Globals::g->resendcount[netnode]--;

		// check for out of order / duplicated packet		
		if (realend == Globals::g->nettics[netnode])
			continue;

		if (realend < Globals::g->nettics[netnode])
		{
			if (Globals::g->debugfile)
				fprintf (Globals::g->debugfile,
				"out of order packet (%i + %i)\n" ,
				realstart,Globals::g->netbuffer->numtics);
			continue;
		}

		// check for a missed packet
		if (realstart > Globals::g->nettics[netnode])
		{
			// stop processing until the other system resends the missed tics
			if (Globals::g->debugfile)
				fprintf (Globals::g->debugfile,
				"missed tics from %i (%i - %i)\n",
				netnode, realstart, Globals::g->nettics[netnode]);
			Globals::g->remoteresend[netnode] = true;
			continue;
		}

		// update command store from the packet
		{
			int		start;

			Globals::g->remoteresend[netnode] = false;

			start = Globals::g->nettics[netnode] - realstart;		
			src = &Globals::g->netbuffer->cmds[start];

			while (Globals::g->nettics[netnode] < realend)
			{
				dest = &Globals::g->netcmds[netconsole][Globals::g->nettics[netnode]%BACKUPTICS];
				Globals::g->nettics[netnode]++;
				*dest = *src;
				src++;
			}
		}
	}
}


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//

void NetUpdate (  )
{
	int             nowtime;
	int             newtics;
	int				i,j;
	int				realstart;
	int				gameticdiv;

	// check time
	nowtime = I_GetTime ()/Globals::g->ticdup;
	newtics = nowtime - Globals::g->gametime;
	Globals::g->gametime = nowtime;

	if (newtics <= 0) 	// nothing new to update
		goto listen; 

	if (Globals::g->skiptics <= newtics)
	{
		newtics -= Globals::g->skiptics;
		Globals::g->skiptics = 0;
	}
	else
	{
		Globals::g->skiptics -= newtics;
		newtics = 0;
	}


	Globals::g->netbuffer->player = Globals::g->consoleplayer;

	// build new ticcmds for console player
	gameticdiv = Globals::g->gametic/Globals::g->ticdup;
	for (i=0 ; i<newtics ; i++)
	{
		//I_GetEvents( Globals::g->I_StartTicCallback () );
		D_ProcessEvents ();
		if (Globals::g->maketic - gameticdiv >= BACKUPTICS/2-1) {
			printf( "Out of room for ticcmds: maketic = %d, gameticdiv = %d\n", Globals::g->maketic, gameticdiv );
			break;          // can't hold any more
		}

		//I_Printf ("mk:%i ",Globals::g->maketic);

		// Grab the latest tech5 command

		G_BuildTiccmd (&Globals::g->localcmds[Globals::g->maketic%BACKUPTICS], newtics );
		Globals::g->maketic++;
	}


	if (Globals::g->singletics)
		return;         // singletic update is syncronous

	// send the packet to the other Globals::g->nodes
	for (i=0 ; i < Globals::g->doomcom.numnodes ; i++) {

		if (Globals::g->nodeingame[i])	{
			Globals::g->netbuffer->starttic = realstart = Globals::g->resendto[i];
			Globals::g->netbuffer->numtics = Globals::g->maketic - realstart;
			if (Globals::g->netbuffer->numtics > BACKUPTICS)
				I_Error ("NetUpdate: Globals::g->netbuffer->numtics > BACKUPTICS");

			Globals::g->resendto[i] = Globals::g->maketic - Globals::g->doomcom.extratics;

			for (j=0 ; j< Globals::g->netbuffer->numtics ; j++)
				Globals::g->netbuffer->cmds[j] = 
				Globals::g->localcmds[(realstart+j)%BACKUPTICS];

			if (Globals::g->remoteresend[i])
			{
				Globals::g->netbuffer->retransmitfrom = Globals::g->nettics[i];
				HSendPacket (i, NCMD_RETRANSMIT);
			}
			else
			{
				Globals::g->netbuffer->retransmitfrom = 0;
				HSendPacket (i, 0);
			}
		}
	}

	// listen for other packets
listen:
	GetPackets ();
}



//
// CheckAbort
//
void CheckAbort (void)
{
	// DHM - Time starts at 0 tics when starting a multiplayer game, so we can
	// check for timeouts easily.  If we're still waiting after N seconds, abort.
	if ( I_GetTime() > NET_TIMEOUT ) {
		// TOOD: Show error & leave net game.
		printf( "NET GAME TIMED OUT!\n" );
		//gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,L"Timed out waiting for match start.") );


		D_QuitNetGame();

		//session->QuitMatch();
		//common->Dialog().AddDialog( GDM_OPPONENT_CONNECTION_LOST, DIALOG_ACCEPT, NULL, NULL, false );
	}
}


//
// D_ArbitrateNetStart
//
bool D_ArbitrateNetStart (void)
{
	int		i;

	Globals::g->autostart = true;
	if (Globals::g->doomcom.consoleplayer)
	{
		// listen for setup info from key player
		CheckAbort ();
		if (!HGetPacket ())
			return false;
		if (Globals::g->netbuffer->checksum & NCMD_SETUP)
		{
			printf( "Received setup info\n" );

			if (Globals::g->netbuffer->player != VERSION)
				I_Error ("Different DOOM versions cannot play a net game!");
			Globals::g->startskill = (skill_t)(Globals::g->netbuffer->retransmitfrom & 15);
			Globals::g->deathmatch = (Globals::g->netbuffer->retransmitfrom & 0xc0) >> 6;
			Globals::g->nomonsters = (Globals::g->netbuffer->retransmitfrom & 0x20) > 0;
			Globals::g->respawnparm = (Globals::g->netbuffer->retransmitfrom & 0x10) > 0;
			// VV original xbox doom :: don't do this.. it will be setup from the launcher
			//Globals::g->startmap = Globals::g->netbuffer->starttic & 0x3f;
			//Globals::g->startepisode = Globals::g->netbuffer->starttic >> 6;
			return true;
		}
		return false;
	}
	else
	{
		// key player, send the setup info
		CheckAbort ();
		for (i=0 ; i < Globals::g->doomcom.numnodes ; i++)
		{
			printf( "Sending setup info to node %d\n", i );

			Globals::g->netbuffer->retransmitfrom = Globals::g->startskill;
			if (Globals::g->deathmatch)
				Globals::g->netbuffer->retransmitfrom |= (Globals::g->deathmatch<<6);
			if (Globals::g->nomonsters)
				Globals::g->netbuffer->retransmitfrom |= 0x20;
			if (Globals::g->respawnparm)
				Globals::g->netbuffer->retransmitfrom |= 0x10;
			Globals::g->netbuffer->starttic = Globals::g->startepisode * 64 + Globals::g->startmap;
			Globals::g->netbuffer->player = VERSION;
			Globals::g->netbuffer->numtics = 0;
			HSendPacket (i, NCMD_SETUP);
		}

		while (HGetPacket ())
		{
			Globals::g->gotinfo[Globals::g->netbuffer->player&0x7f] = true;
		}

		for (i=1 ; i < Globals::g->doomcom.numnodes ; i++) {
			if (!Globals::g->gotinfo[i])
				break;
		}

		if (i >= Globals::g->doomcom.numnodes)
			return true;

		return false;
	}
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame (void)
{
	int             i;

	for (i=0 ; i<MAXNETNODES ; i++)
	{
		Globals::g->nodeingame[i] = false;
		Globals::g->nettics[i] = 0;
		Globals::g->remoteresend[i] = false;	// set when local needs tics
		Globals::g->resendto[i] = 0;		// which tic to start sending
	}

	// I_InitNetwork sets Globals::g->doomcom and Globals::g->netgame
	I_InitNetwork ();
#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
	if (Globals::g->doomcom.id != DOOMCOM_ID)
		I_Error ("Doomcom buffer invalid!");
#endif

	Globals::g->netbuffer = &Globals::g->doomcom.data;
	Globals::g->consoleplayer = Globals::g->displayplayer = Globals::g->doomcom.consoleplayer;
}

bool D_PollNetworkStart()
{
	int             i;
	if (Globals::g->netgame)
	{
		if (D_ArbitrateNetStart () == false)
			return false;
	}

	I_Printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
		Globals::g->startskill, Globals::g->deathmatch, Globals::g->startmap, Globals::g->startepisode);

	// read values out of Globals::g->doomcom
	Globals::g->ticdup = Globals::g->doomcom.ticdup;
	Globals::g->maxsend = BACKUPTICS/(2*Globals::g->ticdup)-1;
	if (Globals::g->maxsend<1)
		Globals::g->maxsend = 1;

	for (i=0 ; i < Globals::g->doomcom.numplayers ; i++)
		Globals::g->playeringame[i] = true;
	for (i=0 ; i < Globals::g->doomcom.numnodes ; i++)
		Globals::g->nodeingame[i] = true;

	I_Printf ("player %i of %i (%i Globals::g->nodes)\n",
		Globals::g->consoleplayer+1, Globals::g->doomcom.numplayers, Globals::g->doomcom.numnodes);

	return true;
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other Globals::g->players
//
void D_QuitNetGame (void)
{
	int i;

	if ( (!Globals::g->netgame && !Globals::g->usergame) || Globals::g->consoleplayer == -1 || Globals::g->demoplayback || Globals::g->netbuffer == NULL )
		return;

	// send a quit packet to the other nodes
	Globals::g->netbuffer->player = Globals::g->consoleplayer;
	Globals::g->netbuffer->numtics = 0;

	for ( i=1; i < Globals::g->doomcom.numnodes; i++ ) {
		if ( Globals::g->nodeingame[i] ) {
			HSendPacket( i, NCMD_EXIT );
		}
	}
	DoomLib::SendNetwork();

	for (i=1 ; i<MAXNETNODES ; i++)
	{
		Globals::g->nodeingame[i] = false;
		Globals::g->nettics[i] = 0;
		Globals::g->remoteresend[i] = false;	// set when local needs tics
		Globals::g->resendto[i] = 0;		// which tic to start sending
	}

	//memset (&Globals::g->doomcom, 0, sizeof(Globals::g->doomcom) );

	// Reset singleplayer state
	Globals::g->doomcom.id = DOOMCOM_ID;
	Globals::g->doomcom.ticdup = 1;
	Globals::g->doomcom.extratics = 0;
	Globals::g->doomcom.numplayers = Globals::g->doomcom.numnodes = 1;
	Globals::g->doomcom.deathmatch = false;
	Globals::g->doomcom.consoleplayer = 0;
	Globals::g->netgame = false;

	Globals::g->netbuffer = &Globals::g->doomcom.data;
	Globals::g->consoleplayer = Globals::g->displayplayer = Globals::g->doomcom.consoleplayer;

	Globals::g->ticdup = Globals::g->doomcom.ticdup;
	Globals::g->maxsend = BACKUPTICS/(2*Globals::g->ticdup)-1;
	if (Globals::g->maxsend<1)
		Globals::g->maxsend = 1;

	for (i=0 ; i < Globals::g->doomcom.numplayers ; i++)
		Globals::g->playeringame[i] = true;
	for (i=0 ; i < Globals::g->doomcom.numnodes ; i++)
		Globals::g->nodeingame[i] = true;
}



//
// TryRunTics
//
bool TryRunTics (  )
{
	int		i;
	int		lowtic_node = -1;

	// get real tics		
	Globals::g->trt_entertic = I_GetTime ()/Globals::g->ticdup;
	Globals::g->trt_realtics = Globals::g->trt_entertic - Globals::g->oldtrt_entertics;
	Globals::g->oldtrt_entertics = Globals::g->trt_entertic;

	// get available tics
	NetUpdate ( );

    Globals::g->trt_lowtic = std::numeric_limits<int>::max();
	Globals::g->trt_numplaying = 0;

	for (i=0 ; i < Globals::g->doomcom.numnodes ; i++) {

		if (Globals::g->nodeingame[i]) {
			Globals::g->trt_numplaying++;

			if (Globals::g->nettics[i] < Globals::g->trt_lowtic) {
				Globals::g->trt_lowtic = Globals::g->nettics[i];
				lowtic_node = i;
			}
		}
	}

	Globals::g->trt_availabletics = Globals::g->trt_lowtic - Globals::g->gametic/Globals::g->ticdup;

	// decide how many tics to run
	if (Globals::g->trt_realtics < Globals::g->trt_availabletics-1) {
		Globals::g->trt_counts = Globals::g->trt_realtics+1;
	} else if (Globals::g->trt_realtics < Globals::g->trt_availabletics) {
		Globals::g->trt_counts = Globals::g->trt_realtics;
	} else {
		Globals::g->trt_counts = Globals::g->trt_availabletics;
	}

	if (Globals::g->trt_counts < 1) {
		Globals::g->trt_counts = 1;
	}

	Globals::g->frameon++;

	if (Globals::g->debugfile) {
		fprintf (Globals::g->debugfile, "=======real: %i  avail: %i  game: %i\n", Globals::g->trt_realtics, Globals::g->trt_availabletics,Globals::g->trt_counts);
	}

	if ( !Globals::g->demoplayback )
	{	
		// ideally Globals::g->nettics[0] should be 1 - 3 tics above Globals::g->trt_lowtic
		// if we are consistantly slower, speed up time
		for (i=0 ; i<MAXPLAYERS ; i++) {
			if (Globals::g->playeringame[i]) {
				break;
			}
		}

		if (Globals::g->consoleplayer == i) {
			// the key player does not adapt
		}
		else {
			if (Globals::g->nettics[0] <= Globals::g->nettics[Globals::g->nodeforplayer[i]])	{
				Globals::g->gametime--;
				//OutputDebugString("-");
			}

			Globals::g->frameskip[Globals::g->frameon&3] = (Globals::g->oldnettics > Globals::g->nettics[Globals::g->nodeforplayer[i]]);
			Globals::g->oldnettics = Globals::g->nettics[0];

			if (Globals::g->frameskip[0] && Globals::g->frameskip[1] && Globals::g->frameskip[2] && Globals::g->frameskip[3]) {
				Globals::g->skiptics = 1;
				//OutputDebugString("+");
			}
		}
	}

	// wait for new tics if needed
	if (Globals::g->trt_lowtic < Globals::g->gametic/Globals::g->ticdup + Globals::g->trt_counts)	
	{
		int lagtime = 0;

		if (Globals::g->trt_lowtic < Globals::g->gametic/Globals::g->ticdup) {
			I_Error ("TryRunTics: Globals::g->trt_lowtic < gametic");
		}

		if ( Globals::g->lastnettic == 0 ) {
			Globals::g->lastnettic = Globals::g->trt_entertic;
		}
		lagtime = Globals::g->trt_entertic - Globals::g->lastnettic;

		// Detect if a client has stopped sending updates, remove them from the game after 5 secs.
		/*if ( common->IsMultiplayer() && (!Globals::g->demoplayback && Globals::g->netgame) && lagtime >= TICRATE ) {

			if ( lagtime > NET_TIMEOUT ) {

				if ( lowtic_node == Globals::g->nodeforplayer[Globals::g->consoleplayer] ) {

#ifdef ID_ENABLE_DOOM_CLASSIC_NETWORKING
#ifndef __PS3__
					gameLocal->showFatalErrorMessage( XuiLookupStringTable(globalStrings,L"You have been disconnected from the match.") );
					gameLocal->Interface.QuitCurrentGame();
#endif
#endif
				} else {
					if (Globals::g->nodeingame[lowtic_node]) {
						int i, consoleNum = lowtic_node;

						for ( i=0; i < Globals::g->doomcom.numnodes; i++ ) {
							if ( Globals::g->nodeforplayer[i] == lowtic_node ) {
								consoleNum = i;
								break;
							}
						}

						Globals::g->nodeingame[lowtic_node] = false;
						Globals::g->playeringame[consoleNum] = false;
						strcpy (Globals::g->exitmsg, "Player 1 left the game");
						Globals::g->exitmsg[7] += consoleNum;
						Globals::g->players[Globals::g->consoleplayer].message = Globals::g->exitmsg;

						// Stop a demo record now, as playback doesn't support losing players
						G_CheckDemoStatus();
					}
				}
			}
		} */

		return false;
	}

	Globals::g->lastnettic = 0;

	// run the count * Globals::g->ticdup dics
	while (Globals::g->trt_counts--)
	{
		for (i=0 ; i < Globals::g->ticdup ; i++)
		{
			if (Globals::g->gametic/Globals::g->ticdup > Globals::g->trt_lowtic) {
				I_Error ("gametic(%d) greater than trt_lowtic(%d), trt_counts(%d)", Globals::g->gametic, Globals::g->trt_lowtic, Globals::g->trt_counts );
				return false;
			}

			if (Globals::g->advancedemo) {
				D_DoAdvanceDemo ();
			}

			M_Ticker ();
			G_Ticker ();
			Globals::g->gametic++;

			// modify command for duplicated tics
			if (i != Globals::g->ticdup-1)
			{
				ticcmd_t	*cmd;
				int			buf;
				int			j;

				buf = (Globals::g->gametic/Globals::g->ticdup)%BACKUPTICS; 
				for (j=0 ; j<MAXPLAYERS ; j++)
				{
					cmd = &Globals::g->netcmds[j][buf];
					if (cmd->buttons & BT_SPECIAL)
						cmd->buttons = 0;
				}
			}
		}

		NetUpdate ( );	// check for new console commands
	}

	return true;
}

