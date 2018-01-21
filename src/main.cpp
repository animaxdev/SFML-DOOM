//
//  main.cpp
//  SFML-DOOM
//
//  Created by Jonny Paton on 20/01/2018.
//

#include "doomlib.hpp"
#include "globaldata.hpp"

#include <SFML/Graphics.hpp>

constexpr int DOOMCLASSIC_BYTES_PER_PIXEL = 4;
constexpr int windowWidth = 640;
constexpr int windowHeight = 400;

int main(int argc, char **argv)
{
    sf::RenderWindow window(sf::VideoMode(640,400),"SFML-DOOM");
    
    // Pixel array
    std::array<sf::Uint8, windowWidth * DOOMCLASSIC_BYTES_PER_PIXEL * windowHeight * DOOMCLASSIC_BYTES_PER_PIXEL> doomClassicImageData;
    
    // Texture to display pixels
    sf::Texture tex;
    tex.create(windowWidth,windowHeight);
    
    // Sprite to display texture
    sf::Sprite sprite;
    sprite.setTexture(tex);
    
    while(window.isOpen())
    {
        static int doomTics = 0;
        
        if( DoomLib::expansionDirty ) {
            
            // re-Initialize the Doom Engine.
            DoomLib::Interface.Shutdown();
            DoomLib::Interface.Startup( 1, false );
            DoomLib::expansionDirty = false;
        }
        
        
        if ( DoomLib::Interface.Frame( doomTics ) )
        {
            Globals *data = (Globals*)DoomLib::GetGlobalData( 0 );
            
            std::array< unsigned int, 256 > palette;
            std::copy( data->XColorMap, data->XColorMap + palette.size(), palette.data() );
            
            // Do the palette lookup.
            for ( int row = 0; row < window.getSize().y; ++row ) {
                for ( int column = 0; column < window.getSize().x; ++column ) {
                    const int doomScreenPixelIndex = row * window.getSize().x + column;
                    const unsigned char paletteIndex = data->screens[0][doomScreenPixelIndex];
                    const unsigned int paletteColor = palette[paletteIndex];
                    const unsigned char red = (paletteColor & 0xFF000000) >> 24;
                    const unsigned char green = (paletteColor & 0x00FF0000) >> 16;
                    const unsigned char blue = (paletteColor & 0x0000FF00) >> 8;
                    
                    const int imageDataPixelIndex = row * window.getSize().x * DOOMCLASSIC_BYTES_PER_PIXEL + column * DOOMCLASSIC_BYTES_PER_PIXEL;
                    doomClassicImageData[imageDataPixelIndex]        = red;
                    doomClassicImageData[imageDataPixelIndex + 1]    = green;
                    doomClassicImageData[imageDataPixelIndex + 2]    = blue;
                    doomClassicImageData[imageDataPixelIndex + 3]    = 255;
                }
            }
        }
        
        
        doomTics++;
    }
    
}
