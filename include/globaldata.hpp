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

#ifndef _GLOBAL_DATA_H
#define _GLOBAL_DATA_H


#include "doomtype.hpp"
#include "d_net.hpp"
#include "m_fixed.hpp"
#include "info.hpp"
#include "sounds.hpp"
#include "r_defs.hpp"
#include "z_zone.hpp"
#include "d_player.hpp"
#include "m_cheat.hpp"
#include "doomlib.hpp"
#include "d_main.hpp"
#include "hu_lib.hpp"
#include "hu_stuff.hpp"
#include "p_spec.hpp"
#include "p_local.hpp"
#include "r_bsp.hpp"
#include "st_stuff.hpp"
#include "st_lib.hpp"
#include "w_wad.hpp"
#include "dstrings.hpp"

#include "defs.hpp"
#include "structs.hpp"
#include <array>
#include <fstream>
#include <iosfwd>

struct Globals
{
	void InitGlobals();
#include "vars.hpp"
    
    static std::unique_ptr<Globals> g;
};

#define GLOBAL( type, name ) type name
#define GLOBAL_ARRAY( type, name, count ) type name[count]

extern void localCalculateAchievements(bool epComplete);


#endif
