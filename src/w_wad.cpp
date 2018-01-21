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


#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>

#include "doomtype.hpp"
#include "m_swap.hpp"
#include "i_system.hpp"
#include "z_zone.hpp"

//#include "idlib/precompiled.hpp"

#ifdef __GNUG__
#pragma implementation "w_wad.hpp"
#endif
#include "w_wad.hpp"

#include "ResourcePath.hpp"



//
// GLOBALS
//

std::vector<lumpinfo_t> lumpinfo;
int			            numlumps;
std::vector<void*>		lumpcache;



int filelength (FILE* handle) 
{ 
	// DHM - not used :: development tool (loading single lump not in a WAD file)
	return 0;
}


void
ExtractFileBase
( const char*		path,
  char*		dest )
{
	const char*	src;
	int		length;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path
		&& *(src-1) != '\\'
		&& *(src-1) != '/')
	{
		src--;
	}
    
	// copy up to eight characters
	memset (dest,0,8);
	length = 0;

	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error ("Filename base of %s >8 chars",path);

		*dest++ = toupper((int)*src++);
	}
}





//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

const char*			reloadname;


void W_AddFile ( const std::string& filename)
{
    wadinfo_t		header;
    int		        i;
    std::shared_ptr<std::ifstream>&   file = Globals::g->wadFileHandles[ Globals::g->numWadFiles ];
    int			    length;
    int			    startlump;
    std::vector<filelump_t>	fileinfo( 1 );
    
    if (!file)
        file = std::make_shared<std::ifstream>();
    
    // open the file and add to directory
    file->open(resourcePath() + filename);
    if (!file->good())
    {
        I_Printf (" couldn't open %s\n",filename.c_str());
        return;
    }

    I_Printf (" adding %s\n",filename.c_str());
    startlump = numlumps;
	
    if (  filename.substr(filename.length() - 3) == "wad" )
    {
        // single lump file
        fileinfo[0].filepos = 0;
        fileinfo[0].size = 0;
        ExtractFileBase (filename.c_str(), fileinfo[0].name);
        numlumps++;
    }
    else
    {
        // WAD file
        file->read( reinterpret_cast<char*>(&header), sizeof( header ) );
        std::string id(header.identification,4);
        if (id != "IWAD")
        {
            // Homebrew levels?
            if ( id !=  "PWAD" )
            {
                I_Error ("Wad file %s doesn't have IWAD "
                         "or PWAD id\n", filename.c_str());
            }
            
            // ???modifiedgame = true;
        }
        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
        length = header.numlumps*sizeof(filelump_t);
        fileinfo.resize(header.numlumps);
        file->seekg(  header.infotableofs );
        file->read( reinterpret_cast<char*>(&fileinfo[0]), length );
        numlumps += header.numlumps;
    }
    
    
    // Fill in lumpinfo
    lumpinfo.resize(numlumps);
    
    filelump_t * filelumpPointer = &fileinfo[0];
    for (auto& lump : lumpinfo)
    {
        lump.handle = file;
        lump.position = LONG(filelumpPointer->filepos);
        lump.size = LONG(filelumpPointer->size);
        std::memcpy (lump.name.data(), filelumpPointer->name, 8);
        filelumpPointer++;
    }
    ++Globals::g->numWadFiles;
}




//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload (void)
{
	// DHM - unused development tool
}

//
// W_FreeLumps
// Frees all lump data
//
void W_FreeLumps() {
	if ( !lumpcache.empty() ) {
		for ( int i = 0; i < numlumps; i++ ) {
			if ( lumpcache[i] ) {
				Z_Free( lumpcache[i] );
			}
		}

        lumpcache.clear();
	}

	if ( !lumpinfo.empty()) {
        lumpinfo.clear();
		numlumps = 0;
	}
}

//
// W_FreeWadFiles
// Free this list of wad files so that a new list can be created
//
void W_FreeWadFiles() {
	for (int i = 0 ; i < MAXWADFILES ; i++) {
		wadfiles[i] = NULL;
        if ( Globals::g->wadFileHandles[i] ) {
            Globals::g->wadFileHandles[i].reset();
        }
	}
	Globals::g->numWadFiles = 0;
	extraWad = 0;
}



//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (const char** filenames)
{
	int		size;

	if (lumpinfo.empty())
	{
		// open all the files, load headers, and count lumps
		numlumps = 0;

		for ( ; *filenames ; filenames++)
		{
			W_AddFile (*filenames);
		}
		
		if (!numlumps)
			I_Error ("W_InitMultipleFiles: no files found");

		// set up caching
		size = numlumps * sizeof(void*);
        lumpcache.resize(size);
	}
    else
    {
		// set up caching
		size = numlumps * sizeof(void*);
        lumpcache.resize(size);
	}
}


void W_Shutdown( void ) {
/*
	for (int i = 0 ; i < MAXWADFILES ; i++) {
		if ( Globals::g->wadFileHandles[i] ) {
			doomFiles->FClose( Globals::g->wadFileHandles[i] );
		}
	}

	if ( lumpinfo != NULL ) {
		free( lumpinfo );
		lumpinfo = NULL;
	}
*/
	W_FreeLumps();
	W_FreeWadFiles();
}

//
// W_NumLumps
//
int W_NumLumps (void)
{
    return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (const std::string& name)
{
    //convert the name to upper case first
    auto n = name;
    std::transform(n.begin(), n.end(),n.begin(), ::toupper);
    // We search in reverse, so the most recently added patches are found first
    for (int i=lumpinfo.size()-1;i>=0;i--)
    {
        // Only check as many chars as there are in the parameter
        auto* l = lumpinfo[i].name.data();
        bool match = true;
        for (auto c : n)
        {
            if (c != *l)
            {
                match = false;
                break;
            }
            l++;
        }
        if (match)
            return i;
    }

    // TFB. Not found.
    return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName ( const char* name)
{
    int	i;

    i = W_CheckNumForName ( name);
    
    if (i == -1)
      I_Error ("W_GetNumForName: %s not found!", name);
      
    return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
    if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

    return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void
W_ReadLump
( int		lump,
  void*		dest )
{
    lumpinfo_t*	l;
    std::weak_ptr<std::ifstream> file;
	
    if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

    l = &lumpinfo[lump];
	
    file = l->handle;
	
    file.lock()->seekg( l->position );
    file.lock()->read( reinterpret_cast<char*>(dest), l->size );

    if (!file.lock()->good())
		I_Error ("W_ReadLump: error reading lump %i",lump);	
}




//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( int		lump,
  int		tag )
{
#ifdef RANGECHECK
	if (lump >= numlumps)
	I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
#endif

	if (!lumpcache[lump])
	{
		unsigned char*	ptr;
		// read the lump in
		//I_Printf ("cache miss on lump %i\n",lump);
		ptr = (unsigned char*)DoomLib::Z_Malloc(W_LumpLength (lump), tag, &lumpcache[lump]);
		W_ReadLump (lump, lumpcache[lump]);
	}

	return lumpcache[lump];
}



//
// W_CacheLumpName
//
void*
W_CacheLumpName
( const char*		name,
  int		tag )
{
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}


void W_Profile (void)
{
}



