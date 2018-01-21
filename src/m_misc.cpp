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


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#include <ctype.h>


#include "doomdef.hpp"
#include "g_game.hpp"
#include "z_zone.hpp"

#include "m_swap.hpp"
#include "m_argv.hpp"

#include "w_wad.hpp"

#include "i_system.hpp"
#include "i_video.hpp"
#include "v_video.hpp"

#include "hu_stuff.hpp"

// State.
#include "doomstat.hpp"

// Data.
#include "dstrings.hpp"

#include "m_misc.hpp"
//#include "d3xp/Game_local.hpp"

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//

int
M_DrawText
( int		x,
  int		y,
  qboolean	direct,
  char*		string )
{
    int 	c;
    int		w;

    while (*string)
    {
	c = toupper(*string) - HU_FONTSTART;
	string++;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    x += 4;
	    continue;
	}
		
	w = SHORT (Globals::g->hu_font[c]->width);
	if (x+w > SCREENWIDTH)
	    break;
	if (direct)
	    V_DrawPatchDirect(x, y, 0, Globals::g->hu_font[c]);
	else
	    V_DrawPatch(x, y, 0, Globals::g->hu_font[c]);
	x+=w;
    }

    return x;
}


//
// M_WriteFile
//
bool M_WriteFile ( char const*	name, void*		source, int		length ) {
	
	//idFile *		handle = NULL;
	int		count;

	//handle = fileSystem->OpenFileWrite( name, "fs_savepath" );

//    if (handle == NULL )
//        return false;
//
//    count = handle->Write( source, length );
	//fileSystem->CloseFile( handle );

	if (count < length)
		return false;

	return true;
}


//
// M_ReadFile
//
int M_ReadFile ( char const*	name, unsigned char**	buffer ) {
	int count, length;
//    idFile * handle = NULL;
	unsigned char		*buf;

//    handle = fileSystem->OpenFileRead( name, false );

//    if (handle == NULL ) {
//        I_Error ("Couldn't read file %s", name);
//    }

//    length = handle->Length();
//
//    buf = ( unsigned char* )Z_Malloc ( handle->Length(), PU_STATIC, NULL);
//    count = handle->Read( buf, length );
//
//    if (count < length ) {
//        I_Error ("Couldn't read file %s", name);
//    }
//
//    fileSystem->CloseFile( handle );

	*buffer = buf;
	return length;
}

//
// Write a save game to the specified device using the specified game name.
//
static qboolean SaveGame( void* source, size_t length )
{
	return false;
}


bool M_WriteSaveGame( void* source, size_t length )
{
	return SaveGame( source, length );
}

int M_ReadSaveGame( unsigned char** buffer )
{
	return 0;
}


//
// DEFAULTS
//











// machine-independent sound params


// UNIX hack, to be removed.
#ifdef SNDSERV
#endif

#ifdef LINUX
#endif

extern const char* const temp_chat_macros[];







//
// M_SaveDefaults
//
void M_SaveDefaults (void)
{
/*
    int		i;
    int		v;
    FILE*	f;
	
    f = f o pen (Globals::g->defaultfile, "w");
    if (!f)
	return; // can't write the file, but don't complain
		
    for (i=0 ; i<Globals::g->numdefaults ; i++)
    {
	if (Globals::g->defaults[i].defaultvalue > -0xfff
	    && Globals::g->defaults[i].defaultvalue < 0xfff)
	{
	    v = *Globals::g->defaults[i].location;
	    fprintf (f,"%s\t\t%i\n",Globals::g->defaults[i].name,v);
	} else {
	    fprintf (f,"%s\t\t\"%s\"\n",Globals::g->defaults[i].name,
		     * (char **) (Globals::g->defaults[i].location));
	}
    }
	
    fclose (f);
*/
}


//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
    int		i;
    //int		len;
    //FILE*	f;
    //char	def[80];
    //char	strparm[100];
    //char*	newstring;
    //int		parm;
    //qboolean	isstring;
    
    // set everything to base values
    Globals::g->numdefaults = sizeof(Globals::g->defaults)/sizeof(Globals::g->defaults[0]);
    for (i=0 ; i < Globals::g->numdefaults ; i++)
		*Globals::g->defaults[i].location = Globals::g->defaults[i].defaultvalue;
    
    // check for a custom default file
    i = M_CheckParm ("-config");
    if (i && i < Globals::g->myargc-1)
    {
		Globals::g->defaultfile = Globals::g->myargv[i+1];
		I_Printf ("	default file: %s\n",Globals::g->defaultfile);
    }
    else
		Globals::g->defaultfile = Globals::g->basedefault;

/*
    // read the file in, overriding any set Globals::g->defaults
    f = f o pen (Globals::g->defaultfile, "r");
    if (f)
    {
		while (!feof(f))
		{
			isstring = false;
			if (fscanf (f, "%79s %[^\n]\n", def, strparm) == 2)
			{
				if (strparm[0] == '"')
				{
					// get a string default
					isstring = true;
					len = strlen(strparm);
					newstring = (char *)DoomLib::Z_Malloc(len, PU_STATIC, 0);
					strparm[len-1] = 0;
					strcpy(newstring, strparm+1);
				}
				else if (strparm[0] == '0' && strparm[1] == 'x')
					sscanf(strparm+2, "%x", &parm);
				else
					sscanf(strparm, "%i", &parm);
				
				for (i=0 ; i<Globals::g->numdefaults ; i++)
					if (!strcmp(def, Globals::g->defaults[i].name))
					{
						if (!isstring)
							*Globals::g->defaults[i].location = parm;
						else
							*Globals::g->defaults[i].location = (int) newstring;
						break;
					}
			}
		}
			
		fclose (f);
    }
*/
}


//
// SCREEN SHOTS
//




//
// WritePCXfile
//
void
WritePCXfile
( char*		filename,
  unsigned char*		data,
  int		width,
  int		height,
  unsigned char*		palette )
{
	I_Error( "depreciated" );
}


//
// M_ScreenShot
//
void M_ScreenShot (void)
{
/*
    int		i;
    unsigned char*	linear;
    char	lbmname[12];
    
    // munge planar buffer to linear
    linear = Globals::g->screens[2];
    I_ReadScreen (linear);
    
    // find a file name to save it to
    strcpy(lbmname,"DOOM00.pcx");
		
    for (i=0 ; i<=99 ; i++)
    {
		lbmname[4] = i/10 + '0';
		lbmname[5] = i%10 + '0';
		if (_access(lbmname,0) == -1)
			break;	// file doesn't exist
    }
    if (i==100)
		I_Error ("M_ScreenShot: Couldn't create a PCX");
    
    // save the pcx file
    WritePCXfile (lbmname, linear,
		  SCREENWIDTH, SCREENHEIGHT,
		  (unsigned char*)W_CacheLumpName ("PLAYPAL",PU_CACHE_SHARED));
	
    Globals::g->players[Globals::g->consoleplayer].message = "screen shot";
*/
}



