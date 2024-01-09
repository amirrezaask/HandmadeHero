#if !defined(HANDMADE_H)


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct game_button_state
{
    int HalfTransitionCount;
    bool EndedDown;
};


struct game_controller_input
{
    bool IsAnalog;
    
    game_button_state Up;
    game_button_state Down;
    game_button_state Left;
    game_button_state Right;
    game_button_state LeftShoulder;
    game_button_state RightShoulder;

    float StartX;
    float StartY;

    float MinX;
    float MinY;

    float MaxX;
    float MaxY;

    float EndX;
    float EndY;
};


struct game_input
{
    game_controller_input Controllers[4];
};
  
struct game_render_offscreen_buffer
{
    void* Memory;
    int Width;
    int Height;
    int Pitch;
};


struct game_sound_output_buffer
{
    int16_t* Samples;
    int SampleCount;
    int SamplesPerSecond;
};


void GameOutputSound(game_sound_output_buffer* Buffer);
void GameUpdateAndRender(game_input* Input, game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer);

#define HANDMADE_H
#endif
