#include "handmade.h"
#include "stdint.h"

static void
GameRenderGradient(game_render_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
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
GameUpdateAndRender(game_memory* Memory, game_input *Input, game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state* GameState = (game_state *) Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
	GameState->ToneHz = 256;
	GameState->BlueOffset = 0;
	GameState->GreenOffset = 0;
	Memory->IsInitialized = true;
    }
    

    game_controller_input* Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog)
    {
	GameState->BlueOffset += (int)4.0f*(Input0->EndX);
	GameState->ToneHz = 256 + (int)(128.0f*(Input0->EndY));
    }
    
    GameRenderGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
    GameOutputSound(SoundBuffer, GameState->ToneHz);
}
