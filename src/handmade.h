/*
  HANDMADE_SLOW: slow code is allowed
  HANDMADE_INTERNAL: Internal build for me not public
 */

#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define Assert(Expr) \
    if (!(Expr)) { *(int*) 0 = 0; }
#else
#define Assert(Expr)
#endif

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
    uint32_t ContentsSize;
    void* Contents;
	
};
static debug_read_file_result DEBUGPlatformReadEntireFile(char* FileName);
static void  DEBUGPlatformFreeFileMemory(void *Memory);
static bool  DEBUGPlatformWriteEntireFile(char *FilName, void* Memory, uint32_t MemorySize);
#endif


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(Size) ((Size) * 1024)
#define Megabytes(Size) ((Size) * 1024 * 1024)
#define Gigabytes(Size) ((Size) * 1024 * 1024 * 1024)
#define Terrabytes(Size) ((Size) * 1024 * 1024 * 1024 * 1024)

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
    //TODO(amirreza): clock here.
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

struct game_memory
{
    bool IsInitialized;
    uint64_t PermanentStorageSize;
    void* PermanentStorage;
    
    uint64_t TransientStorageSize;
    void* TransientStorage;
    
};

void GameOutputSound(game_sound_output_buffer* Buffer);
void GameUpdateAndRender(game_memory* Memory, game_input* Input, game_render_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer);

struct game_state
{
    int ToneHz;
    int BlueOffset;
    int GreenOffset;
};

#define HANDMADE_H
#endif
