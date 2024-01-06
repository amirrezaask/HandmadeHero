#include "handmade.h"
struct game_render_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BlueOffset;
    int GreenOffset;
};

static void
GameRender(game_render_offscreen_buffer* Buffer)
{
    // NOTE(amirreza): Our BitmapMemory is a 1D space in memory, but we need to interpret it as a 2D Matrix of pixels,
    
    uint8_t* Row = (uint8_t*)Buffer->Memory; // Pointer to each row in our BitmapMemory

    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
	uint32_t* Pixel = (uint32_t*)Row;
	for (int X = 0; X < Buffer->Width; ++X)
	{
	    // Note(Amirreza): Little Endian
	    // Memory : BB GG RR xx
	    // Register : xx RR GG BB
	    int Blue = X + Buffer->BlueOffset;
	    int Green = Y + Buffer->GreenOffset;
	    uint32_t Color = (Green << 8 | Blue);
	    *Pixel = Color;
	    ++Pixel;
	}
	Row += Buffer->Pitch;
    
    }
}
