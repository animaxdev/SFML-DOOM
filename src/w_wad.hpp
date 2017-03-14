#pragma once

#include <vector>
#include <fstream>

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct filelump_t
///
/// \brief A filelump t.
///
/// \author Jonny
/// \date 26/02/2017
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    int			filepos;
    int			size;
    char		name[8];

} filelump_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct lumpinfo_t
///
/// \brief A lumpinfo t.
///
/// \author Jonny
/// \date 26/02/2017
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    std::string	name = std::string(8, '0');
    std::vector<char> data;
    int		size;
} lumpinfo_t;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \class  WadManager
///
/// \brief  Manages the WAD files loaded in the game.
///
/// \author Jonny
/// \date   26/02/2017
////////////////////////////////////////////////////////////////////////////////////////////////////

class WadManager
{
public:

    int W_NumLumps();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static void WadManager::addFile(const std::string& fileName);
    ///
    /// \brief  Adds a file.
    ///         
    ///          All files are optional, but at least one file must be
    ///         found (PWAD, if all required lumps are present). Files with a .wad extension are
    ///         wadlink files with multiple lumps. Other files are single lumps with the base
    ///         filename for the lump name.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  fileName    Filename of the file.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void         addFile(const std::string& fileName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static void WadManager::initMultipleFiles(std::vector<std::string> filenames);
    ///
    /// \brief  Init multiple files.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  filenames   The filenames.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void         initMultipleFiles(std::vector<std::string> filenames);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static int WadManager::checkNumForName(const std::string& name);
    ///
    /// \brief  Check number for name.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  name    The name.
    ///
    /// \return An int.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static int	        checkNumForName(const std::string& name);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static int WadManager::getNumForName(const std::string& name);
    ///
    /// \brief  Gets number for name.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  name    The name.
    ///
    /// \return The number for name.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static int	        getNumForName(const std::string& name);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static std::string WadManager::getNameForNum(int);
    ///
    /// \brief  Gets name for number.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  parameter1  The first parameter.
    ///
    /// \return The name for number.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static std::string  getNameForNum(int);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static int WadManager::getLumpLength(int lump);
    ///
    /// \brief  Gets lump length.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  lump    The lump.
    ///
    /// \return The lump length.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static int	        getLumpLength(int lump);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static void* WadManager::getLump(int lump);
    ///
    /// \brief  Gets a lump.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  lump    The lump.
    ///
    /// \return Null if it fails, else the lump.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void*        getLump(int lump);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \fn static void* WadManager::getLump(const std::string& name);
    ///
    /// \brief  Gets a lump.
    ///
    /// \author Jonny
    /// \date   26/02/2017
    ///
    /// \param  name    The name.
    ///
    /// \return Null if it fails, else the lump.
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    static void*        getLump(const std::string& name);

private: 
    /// \brief  Number of lumps.
    static int                      numlumps;
    /// \brief  The lumps.
    static std::vector<lumpinfo_t>	lumpinfo;
};