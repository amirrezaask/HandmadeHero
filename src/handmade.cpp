#include "handmade.h"
#include "stdint.h"

static void
GameRenderGradient(game_render_offscreen_buffer* Buffer, int BlueOffset)
{

    static int GreenOffset = 0;
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
	    int Blue = X + BlueOffset;
	    int Green = Y + GreenOffset;
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
GameUpdateAndRender(game_input *Input, game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer)
{
    int ToneHz = 256;
    static int BlueOffset = 0;

    game_controller_input* Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog)
    {
	BlueOffset += (int)4.0f*(Input0->EndX);
	ToneHz = 256 + (int)(128.0f*(Input0->EndY));
    }
    
    GameRenderGradient(Buffer, BlueOffset);
    GameOutputSound(SoundBuffer, ToneHz);
}
