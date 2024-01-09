#if !defined(WIN32_HANMADEHERO_H)
struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32_t RunningSampleIndex;
    int BytesPerSample;
    int DSoundBufferSize;
    int LatencySampleCount;
};


#define WIN32_HANMADEHERO_H
#endif



