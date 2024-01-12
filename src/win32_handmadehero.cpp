/*
TODO(amirreza): Make this into an actual platform layer:
     - Saved game locations
     - Getting a handle to our own exe
     - Asset loading path
     - Threading
     - Raw Input (support for multiple keyboards)
     - Sleep/TimeBeginPeriod
     - ClipCursor (multi monitor support)
     - Fullscreen support
     - WM_SETCURSOR(Control cursor visibility)
     - QueryCancelAutoplay
     - WM_ACTIVATEAPP (For when we are not active application)
     - Blit speed improvements (BitBlit)
     - Hardware accelaration(OpenGL or Direct3D or Both ??)
     - GetKeyboardLayout (for french keyboards, international "WASD" support)
*/


#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <winerror.h>
#include <math.h>
#include <malloc.h>

#define Pi32 3.141592653589f

#include "win32_handmadehero.h"
#include "handmade.h"
#include "handmade.cpp"

// NOTE(amirreza): this section contains macros to define stub functions to be a fallback option when we cannot load xinput dll file.
#define X_INPUT_SET_STATE_SIGNATURE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* Vibration)
#define X_INPUT_GET_STATE_SIGNATURE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)

typedef X_INPUT_SET_STATE_SIGNATURE(x_input_set_state_t);
typedef X_INPUT_GET_STATE_SIGNATURE(x_input_get_state_t);

X_INPUT_SET_STATE_SIGNATURE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}

X_INPUT_GET_STATE_SIGNATURE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}

static x_input_get_state_t* DyXInputGetState = XInputGetStateStub;
static x_input_set_state_t* DyXInputSetState = XInputSetStateStub;

#define XInputGetState DyXInputGetState
#define XInputSetState DyXInputSetState





static debug_read_file_result
DEBUGPlatformReadEntireFile(char* FileName)
{
    debug_read_file_result Result = {};
    HANDLE FileHandle =  CreateFileA(__FILE__, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
	LARGE_INTEGER FileSize;
	if(GetFileSizeEx(FileHandle, &FileSize))
	{

	    Result.Contents = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	    if(Result.Contents)
	    {
		DWORD BytesRead;
		if(ReadFile(FileHandle, Result.Contents, FileSize.QuadPart, &BytesRead, 0) && (BytesRead == FileSize.QuadPart))
		{
		    Result.ContentsSize = FileSize.QuadPart;
		}
		else
		{
		    //TODO(amirreza): error reading file
		    DEBUGPlatformFreeFileMemory(Result.Contents);
		    Result.Contents = 0;
		}
	    }
	}
	CloseHandle(FileHandle);
    }
    else
    {
	//TODO(amirreza): error creating file handle.
    }

    
    return(Result);
}

static void
DEBUGPlatformFreeFileMemory(void *Memory)
{
    if(Memory)
    {
	VirtualFree(Memory, 0, MEM_RELEASE);
    }

}

static bool
DEBUGPlatformWriteEntireFile(char *FileName, void* Memory, uint32_t MemorySize)
{
    bool Result;
    HANDLE FileHandle =  CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
	DWORD BytesWritten;
	if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
	{
	    Result = (BytesWritten == MemorySize);
	}
	else
	{
	    //TODO(amirreza): error reading file
	}
	CloseHandle(FileHandle);

    }
    else
    {
	//TODO(amirreza): error creating file handle.
    }
    
    return Result;
}

static void
Win32LoadXInputLibrary()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
	XInputGetState = (x_input_get_state_t*)GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state_t*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}
////////////////////////////////////////////////////////////////
// NOTE(amirreza): 
// DirectSound Compat
// Same thing as xinput dll, macros to define stub functions to act as fallback for DirectSound calls.
static LPDIRECTSOUNDBUFFER GlobalDSoundSecondaryBuffer;

#define DIRECT_SOUND_CREATE_SIGNATURE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE_SIGNATURE(direct_sound_create_t);

static void
Win32InitDSound(HWND Window, int32_t BufferSize, int32_t SamplesPerSec)
{
    // Get library
    HMODULE DSoundLib = LoadLibraryA("dsound.dll");
    if (DSoundLib)
    {
	direct_sound_create_t* DirectSoundCreate = (direct_sound_create_t*) GetProcAddress(DSoundLib, "DirectSoundCreate");
	LPDIRECTSOUND DirectSound;
	if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
	    WAVEFORMATEX WAVFormat = {};
	    WAVFormat.wFormatTag = WAVE_FORMAT_PCM;
	    WAVFormat.nChannels = 2;
	    WAVFormat.wBitsPerSample = 16;
	    WAVFormat.nSamplesPerSec = SamplesPerSec;
	    WAVFormat.cbSize = 0;

	    WAVFormat.nBlockAlign = (WAVFormat.nChannels * WAVFormat.wBitsPerSample) / 8;
	    WAVFormat.nAvgBytesPerSec = WAVFormat.nSamplesPerSec*WAVFormat.nBlockAlign;

	    if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	    {
		// create a primary buffer
		LPDIRECTSOUNDBUFFER PrimaryBuffer;
		DSBUFFERDESC BufferDescription = {};
		BufferDescription.dwSize = sizeof(BufferDescription);
		BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
		if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
		{
		    HRESULT ErrorCode = PrimaryBuffer->SetFormat(&WAVFormat);
		    if (!SUCCEEDED(ErrorCode))
		    {
			//TODO(amirreza): handle error
		    }
		    OutputDebugStringA("[DSOUND] Primary Buffer Created\n");

		}
		else
		{
		    // TODO(amirreza): Error
		}
        
	    }
        
	    else
	    {
		// TODO(amirreza): Error
	    }
	    // Created Primary Buffer
	    // Now we create a secondary buffer
	    DSBUFFERDESC BufferDescription = {};
	    BufferDescription.dwSize = sizeof(BufferDescription);
	    BufferDescription.dwFlags = 0;
	    BufferDescription.dwBufferBytes = BufferSize;
	    BufferDescription.lpwfxFormat = &WAVFormat;
	    HRESULT ErrorCode = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalDSoundSecondaryBuffer, 0);
	    if (SUCCEEDED(ErrorCode))
	    {
		// And now we play
		OutputDebugStringA("[DSOUND] Secondary Buffer Created\n");

	    }
	    else
	    {
	    }

       
	}
	else
	{

	}
    }
}


static bool Running;

static win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension
{
    int Height;
    int Width;
};


static win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT rect;
    GetClientRect(Window, &rect);
    int X = rect.left;
    int Y = rect.top;
    Result.Width = rect.right - rect.left;
    Result.Height = rect.bottom - rect.top;
    return (Result);
}



static void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{

    // Free last memory allocated by us
    if (Buffer->Memory)
    {
	VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    
    // Create Bitmap info, basically metadata about our bitmap buffer
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // Allocate actual memory for bitmap.
    int BitmapMemorySize = 4 * Buffer->Width * Buffer->Height;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->Width*Buffer->BytesPerPixel; // Amount of byte offset that we need to go from start of the row to start of next row.
}

static void
Win32DisplayBufferInDeviceContext(win32_offscreen_buffer* Buffer, HDC DeviceContext, int Width, int Height)
{
    int result = StretchDIBits(
	DeviceContext,
	0, 0, Width, Height,
	0, 0, Buffer->Width, Buffer->Height,
	Buffer->Memory,
	&Buffer->Info,
	DIB_RGB_COLORS,
	SRCCOPY
	);
}

LRESULT
CALLBACK MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0;
    switch (Message)
    {
    case WM_CREATE:
    {
	OutputDebugStringA("WM_CREATE\n");
    } break;
    case WM_CLOSE:
    {
	Running = false;
	OutputDebugStringA("WM_CLOSE\n");
    }
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
	uint32_t VKCode = WParam;
	bool WasDownBeforeThisMessage = ((LParam & (1 << 30)) != 0);
	bool IsDownNow = ((LParam & (1 << 31)) == 0);
	if (IsDownNow != WasDownBeforeThisMessage)
	{
	    if (VKCode == 'W')
	    {
		OutputDebugStringA("W key: ");
		if (IsDownNow) OutputDebugStringA(" is down ");
		if (WasDownBeforeThisMessage) OutputDebugStringA("was down");
		OutputDebugStringA("\n");
	    }
	    else if (VKCode == 'D')
	    {

	    }
	    else if (VKCode == 'A')
	    {

	    }
	    else if (VKCode == 'S')
	    {

	    }
	    else if (VKCode == 'Q')
	    {

	    }
	    else if (VKCode == 'E')
	    {

	    }
	    else if (VKCode == VK_DOWN)
	    {

	    }
	    else if (VKCode == VK_UP)
	    {

	    }
	    else if (VKCode == VK_LEFT)
	    {

	    }
	    else if (VKCode == VK_RIGHT)
	    {

	    }
	    else if (VKCode == VK_SPACE)
	    {

	    }
	    else if (VKCode == VK_ESCAPE)
	    {

	    }
	    bool AltKeyIsDown = (LParam & (1 << 29)) != 0;
	    if (AltKeyIsDown && VKCode == VK_F4)
	    {
		Running = false;
	    }
        
	}
    } break;
    case WM_DESTROY:
    {
	Running = false;
	OutputDebugStringA("WM_DESTROY\n");
    } break;
    case WM_SIZE:
    {
	OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_PAINT:
    {
	PAINTSTRUCT Paint;
	HDC DeviceContext = BeginPaint(Window, &Paint);
	win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32DisplayBufferInDeviceContext(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
	EndPaint(Window, &Paint);
        
    } break;
    default:
    
	Result = DefWindowProcA(Window, Message, WParam, LParam);
    }

    return (Result);
}


// TODO(amirreza): This has problems and many times LOCK method actually fails, that's the cause of the clicks we here in the sound.
static void
Win32FillSoundBuffer(win32_sound_output* SoundOutput,
		     DWORD ByteToLock,
		     DWORD BytesToWrite,
		     game_sound_output_buffer* SoundBuffer
    )
{

    DWORD Region1Size;
    void* Region1;
    DWORD Region2Size;
    void* Region2;
    HRESULT ErrorCode = GlobalDSoundSecondaryBuffer->Lock(
	ByteToLock, BytesToWrite,
	&Region1, &Region1Size,
	&Region2, &Region2Size,
	0);
    if (SUCCEEDED(ErrorCode))
    {
	DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
	int16_t* DestSamples = (int16_t*) Region1;
	int16_t* SourceSamples = SoundBuffer->Samples;
	for(DWORD SampleIndex = 0;
	    SampleIndex < Region1SampleCount;
	    ++SampleIndex)
	{
	    *DestSamples++ = *SourceSamples++;
	    *DestSamples++ = *SourceSamples++;
	    ++SoundOutput->RunningSampleIndex;
	}

	DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;

	DestSamples = (int16_t*) Region2;
	for(DWORD SampleIndex = 0;
	    SampleIndex < Region2SampleCount;
	    ++SampleIndex)
	{
	    *DestSamples++ = *SourceSamples++;
	    *DestSamples++ = *SourceSamples++;
	    ++SoundOutput->RunningSampleIndex;
	}
	GlobalDSoundSecondaryBuffer->Unlock(
	    &Region1,
	    Region1Size,
	    &Region2,
	    Region2Size
	    );
    }
	
}

static void
Win32ClearBuffer(win32_sound_output* SoundOutput)
{
    void* Region1;
    DWORD Region1Size;
    void* Region2;
    DWORD Region2Size;
    
    HRESULT ErrorCode = GlobalDSoundSecondaryBuffer->Lock(
	0, SoundOutput->DSoundBufferSize,
	&Region1, &Region1Size,
	&Region2, &Region2Size,
	0);

    if(SUCCEEDED(ErrorCode))
    {
	uint8_t *DestSample = (uint8_t*) Region1;
	for (int ByteIndex=0;
	     ByteIndex < Region1Size;
	     ++ByteIndex)
	{
	    *DestSample++ = 0;
	}
	DestSample = (uint8_t*) Region2;
	
	for (int ByteIndex=0;
	     ByteIndex < Region2Size;
	     ++ByteIndex)
	{
	    *DestSample++ = 0;
	}

	GlobalDSoundSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
};


static void
Win32ProcessDigitalXInputButton(WORD XInputButtonState, game_button_state* OldState, DWORD ButtonBit,
				game_button_state* NewState)
{
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown == NewState->EndedDown) ? 1: 0;
}

    
int WINAPI
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	PSTR CmdLine,
	int ShowCode)
{
    LARGE_INTEGER PerformanceFrequencyCount;
    QueryPerformanceFrequency(&PerformanceFrequencyCount);
    
    Win32LoadXInputLibrary();
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";
    if (RegisterClass(&WindowClass))
    {
	HWND WindowHandle = CreateWindowA(
	    WindowClass.lpszClassName,
	    "HandmadeHero",
	    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    0, 0, Instance, 0);
	if (WindowHandle)
	{
	    // Initilize Sound System
	    win32_sound_output SoundOutput = {};
	    SoundOutput.SamplesPerSecond = 48000;
	    SoundOutput.RunningSampleIndex = 0;
	    SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
	    SoundOutput.DSoundBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
	    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
	    Win32InitDSound(WindowHandle, SoundOutput.DSoundBufferSize, SoundOutput.SamplesPerSecond);
	    int16_t* SoundSamplesMemory = (int16_t*) VirtualAlloc(0, SoundOutput.DSoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
	    LPVOID BaseAddress = (LPVOID)Terrabytes((uint64_t)2);

#else
	    LPVOID BaseAddress = 0;
#endif
	    
	    game_memory GameMemory = {};
	    GameMemory.PermanentStorageSize = Megabytes(64);
	    GameMemory.TransientStorageSize = Gigabytes((uint64_t) 4);

	    uint64_t TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	    GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	    GameMemory.TransientStorage = (((uint8_t*)GameMemory.PermanentStorage) + TotalSize);


	    if ((GameMemory.PermanentStorage == 0) || (SoundSamplesMemory == 0) || (GameMemory.TransientStorage == 0))
	    {
		//TODO(amirreza): log error
		return 0;
	    }
	    // game input buffers
	    game_input Input[2];
	    game_input* NewInput = &Input[0];
	    game_input* OldInput = &Input[1];

	    // Fill buffer for first time and play sound
	    Win32ClearBuffer(&SoundOutput);
	    GlobalDSoundSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	    
	    // Initilize graphics render system
	    int XOffset = 0;
	    int YOffset = 0;
	    Running = true;
	    
	    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	    
	    LARGE_INTEGER LastPerformanceCounter;
	    QueryPerformanceCounter(&LastPerformanceCounter);
	    uint64_t LastCycleCount = __rdtsc();

	    while(Running) {
		
		// Get events from sources ( windows, Xinput, ...)
		MSG Message;
		while(PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE))
		{
		    if (Message.message == WM_QUIT) Running = false;
		    TranslateMessage(&Message);
		    DispatchMessage(&Message);
		}


		int MaxControllerCount = XUSER_MAX_COUNT;
		if(MaxControllerCount > ArrayCount(NewInput->Controllers))
		{
		    MaxControllerCount = ArrayCount(NewInput->Controllers);
		}
		for (int ControllerIndex = 0;
		     ControllerIndex < MaxControllerCount;
		     ++ControllerIndex)
		{

		    game_controller_input* OldController = &OldInput->Controllers[ControllerIndex];
		    game_controller_input* NewController = &NewInput->Controllers[ControllerIndex];
		    
		    XINPUT_STATE State;
		    if(XInputGetState(ControllerIndex, &State) == ERROR_SUCCESS)
		    {
			XINPUT_GAMEPAD* Pad = &State.Gamepad;
			bool Up             = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			bool Down           = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			bool Left           = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			bool Right          = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
			bool LeftThumb      = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
			bool RightThumb     = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

			NewController->IsAnalog = true;
			NewController->StartX = OldController->EndX;
			NewController->StartY = OldController->EndY;

			float X;
			if (Pad->sThumbLX < 0)
			{
			    X = (float)Pad->sThumbLX / 32768.0f;
			}
			else
			{
			    X = (float)Pad->sThumbLX / 32767.0f;
			}

			NewController->MinX = NewController->MaxX = NewController->EndX = X;

			float Y;
			if (Pad->sThumbLY < 0)
			{
			    Y = (float)Pad->sThumbLY / 32768.0f;
			}
			else
			{
			    Y = (float)Pad->sThumbLY / 32767.0f;
			}
			
			NewController->MinY = NewController->MaxY = NewController->EndY = Y;

			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->Down,          XINPUT_GAMEPAD_A,              &NewController->Down);
			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->Right,         XINPUT_GAMEPAD_B,              &NewController->Right);
			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->Left,          XINPUT_GAMEPAD_X,              &NewController->Left);
			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->Up,            XINPUT_GAMEPAD_Y,              &NewController->Up);
			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->LeftShoulder,  XINPUT_GAMEPAD_LEFT_SHOULDER,  &NewController->LeftShoulder);
			Win32ProcessDigitalXInputButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

			bool LeftShoulder   = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			bool RightShoulder  = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			bool AButton        = (Pad->wButtons & XINPUT_GAMEPAD_A);
			bool BButton        = (Pad->wButtons & XINPUT_GAMEPAD_B);
			bool XButton        = (Pad->wButtons & XINPUT_GAMEPAD_X);
			bool YButton        = (Pad->wButtons & XINPUT_GAMEPAD_Y);
			bool Start          = (Pad->wButtons & XINPUT_GAMEPAD_START);
			bool Back           = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

		    }
		    else
		    {
			// TODO(amirreza): ERROR: Controller is not available
		    }
		}


		DWORD PlayCursor;
		DWORD WriteCursor;
		DWORD ByteToLock;
		DWORD BytesToWrite;
		DWORD TargetCursor;
		bool SoundIsValid = false;
		if(SUCCEEDED(GlobalDSoundSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
		{
		    ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.DSoundBufferSize);
		    TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.DSoundBufferSize);
		    if (ByteToLock > TargetCursor)
		    {
			BytesToWrite = (SoundOutput.DSoundBufferSize - ByteToLock);
			BytesToWrite += TargetCursor;
		    }
		    else
		    {
			BytesToWrite = TargetCursor - ByteToLock;
		    }
		    SoundIsValid = true;

		}

		// update and render
		game_sound_output_buffer SoundBuffer;
		SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
		SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
		SoundBuffer.Samples = SoundSamplesMemory;

		
		game_render_offscreen_buffer GameBuffer;
		GameBuffer.Memory = GlobalBackBuffer.Memory;
		GameBuffer.Width = GlobalBackBuffer.Width;
		GameBuffer.Height = GlobalBackBuffer.Height;
		GameBuffer.Pitch = GlobalBackBuffer.Pitch;
		GameUpdateAndRender(&GameMemory, NewInput, &GameBuffer, &SoundBuffer);

		if(SoundIsValid)
		{
		    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
		}
		
		// Draw pixels on our screen
		HDC DeviceContext = GetDC(WindowHandle);
		win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
		Win32DisplayBufferInDeviceContext(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
		ReleaseDC(WindowHandle, DeviceContext);
		++XOffset;

		uint64_t EndCycleCount = __rdtsc();

		LARGE_INTEGER EndPerformanceCounter;
		QueryPerformanceCounter(&EndPerformanceCounter);

		int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
		int64_t CounterElapsed = EndPerformanceCounter.QuadPart - LastPerformanceCounter.QuadPart;
		// CounterElapsed(COUNT)/PerformanceFrequencyCount(COUNT/SEC) => SEC
		
		int32_t MSPerFrame = (int32_t)(((1000*CounterElapsed) / PerformanceFrequencyCount.QuadPart));
		// COUNT/SEC % COUNT/FRAME => FRAME/SEC
		int32_t FPS = (PerformanceFrequencyCount.QuadPart/CounterElapsed);
		int32_t MCPF = (int32_t)(CyclesElapsed/(1000*1000));
		
		// char Buffer[256];
		// wsprintf(Buffer, "ms/frame: %dms, FPS: %d, %dMc/f\n", MSPerFrame, FPS, MCPF);
		// OutputDebugStringA(Buffer);

		LastPerformanceCounter = EndPerformanceCounter;
		LastCycleCount = EndCycleCount;

		game_input* Temp = NewInput;
		NewInput = OldInput;
		OldInput = Temp;
	    }
	}
	else
	{
	    // TODO(amirreza): handle error
	}
    }
    else
    {
	// TODO(amirreza): handle error
    }


    return 0;
}

