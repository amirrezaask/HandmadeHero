#if !defined(HANDMADE_H)


struct game_render_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BlueOffset;
    int GreenOffset;
};


struct game_sound_output_buffer
{
    int16_t* Samples;
    int SampleCount;
    int SamplesPerSecond;
};

void GameOutputSound(game_sound_output_buffer* Buffer);

void GameUpdateAndRender(game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer, int ToneHz);

#define HANDMADE_H
#endif
