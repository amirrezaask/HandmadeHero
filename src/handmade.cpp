#include "handmade.h"
#include "stdint.h"


static void
GameRenderGradient(game_render_offscreen_buffer* Buffer)
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


static void 
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
    static float tSine;
    int16_t ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    
    int16_t* SoundOut = (int16_t*) SoundBuffer->Samples;
    
    for(int SampleIndex = 0;
	SampleIndex < SoundBuffer->SampleCount;
	++SampleIndex)
    {
	float SineValue = sinf(tSine);
	int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
	*SoundOut++ = SampleValue;
	*SoundOut++ = SampleValue;

	tSine += 2.0f*Pi32*1.0f/(float)WavePeriod;
    }

}

static void
GameUpdateAndRender(game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer, int ToneHz)
{
    GameRenderGradient(Buffer);
    GameOutputSound(SoundBuffer, ToneHz);
}
