#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <winerror.h>

// XInput Compat
// this section contains macros to define stub functions to be a fallback option
// when we cannot load xinput dll file.
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
		    if(SUCCEEDED(ErrorCode))
		    {
			OutputDebugStringA("[DSOUND] Primary Buffer Created\n");
		    }
		    else
		    {
			// Error
		    }
		}
		else
		{
		    // Error
		}
        
	    }
        
	    else
	    {
		// Error
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

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};
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
Win32RenderGradient(win32_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
    /* @Note(amirreza):
       Our BitmapMemory is a 1D space in memory,
       but we need to interpret it as a 2D Matrix of pixels,
    */
    
    uint8_t* Row = (uint8_t*)Buffer->Memory; // Pointer to each row in our BitmapMemory

    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
	uint32_t* Pixel = (uint32_t*)Row;
	for (int X = 0; X < Buffer->Width; ++X)
	{
	    // Note(Amirreza): Little Endian
	    // Memory : BB GG RR xx
	    // Register : xx RR GG BB
	    int Blue = X + XOffset;
	    int Green = Y + YOffset;
	    uint32_t Color = (Green << 8 | Blue);
	    *Pixel = Color;
	    ++Pixel;
	}
	Row += Buffer->Pitch;
    
    }
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

    
int WINAPI
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	PSTR CmdLine,
	int ShowCode)
{
    Win32LoadXInputLibrary();
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "ClockWorkWindowClass";
    if (RegisterClass(&WindowClass))
    {
	HWND WindowHandle = CreateWindowA(
	    WindowClass.lpszClassName,
	    "ClockWork",
	    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
	    0, 0, Instance, 0);
	if (WindowHandle)
	{
	    // Initilize Sound System
	    int SamplesPerSecond = 48000;
	    int ToneHz = 256;
	    uint32_t RunningSampleIndex = 0;
	    int SquareWavePeriod = SamplesPerSecond/ToneHz;
	    int HalfSquareWavePeriod = SquareWavePeriod / 2;
	    int BytesPerSample = sizeof(int16_t) * 2;
	    int DSoundBufferSize = SamplesPerSecond * BytesPerSample;
	    int ToneVolume = 2000;
	    
	    Win32InitDSound(WindowHandle, SamplesPerSecond, DSoundBufferSize);
	    GlobalDSoundSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

	    
	    // Initilize graphics render system
	    int XOffset = 0;
	    int YOffset = 0;
	    Running = true;
	    
	    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	    
	    while(Running) {
		// Get events from sources ( windows, Xinput, ...)
		MSG Message;
		while(PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE))
		{
		    if (Message.message == WM_QUIT) Running = false;
		    TranslateMessage(&Message);
		    DispatchMessage(&Message);
		}


		for (int ControllerIndex = 0;
		     ControllerIndex < XUSER_MAX_COUNT;
		     ++ControllerIndex)
		{
		    XINPUT_STATE State;
		    if(XInputGetState(ControllerIndex, &State) == ERROR_SUCCESS)
		    {
			XINPUT_GAMEPAD* Pad = &State.Gamepad;
			bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
			bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
			bool LeftThumb = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
			bool RightThumb = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
			bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
			bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
			bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
			bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

			if (AButton)
			{
			    ++YOffset;
			}
		    }
		    else
		    {
			// TODO(amirrez): ERROR: Controller is not available
		    }
		}



		// Update our state
                Win32RenderGradient(&GlobalBackBuffer, XOffset, YOffset);

		// NOTE(amirreza): this is just for sake of testing.
		DWORD PlayCursorPosition;
		DWORD WriteCursorPosition;
		
		if(SUCCEEDED(GlobalDSoundSecondaryBuffer->GetCurrentPosition(&PlayCursorPosition, &WriteCursorPosition)))
		{
		    DWORD ByteToLock = (RunningSampleIndex * BytesPerSample) % DSoundBufferSize;
		    DWORD BytesToWrite;

		    if(ByteToLock > PlayCursorPosition)
		    {
			BytesToWrite = DSoundBufferSize - ByteToLock;
			BytesToWrite += PlayCursorPosition;
		    }
		    else
		    {
			BytesToWrite = PlayCursorPosition - ByteToLock;
		    }

		    DWORD Region1Size;
		    LPVOID Region1;
		    DWORD Region2Size;
		    LPVOID Region2;
		    
		    if (SUCCEEDED(GlobalDSoundSecondaryBuffer->Lock(
				      ByteToLock,
				      BytesToWrite,
				      &Region1,
				      &Region1Size,
				      &Region2,
				      &Region2Size,
				      0)))
		    {
			int16_t* SampleOut = (int16_t*) Region1;
			DWORD Region1SampleCount = Region1Size / BytesPerSample;
			DWORD Region2SampleCount = Region2Size / BytesPerSample;
		
			for(DWORD SampleIndex = 0;
			    SampleIndex < Region1SampleCount;
			    ++SampleIndex)
			{
			    int16_t SampleValue = ((RunningSampleIndex / HalfSquareWavePeriod) % 2 == 0) ? ToneVolume : -ToneVolume;
			    *SampleOut++ = SampleValue;
			    *SampleOut++ = SampleValue;
			    ++RunningSampleIndex;
			}

			SampleOut = (int16_t*) Region2;
			for(DWORD SampleIndex = 0;
			    SampleIndex < Region2SampleCount;
			    ++SampleIndex)
			{
			    int16_t SampleValue = ((RunningSampleIndex / HalfSquareWavePeriod)%2 == 0) ? ToneVolume : -ToneVolume;
			    *SampleOut++ = SampleValue;
			    *SampleOut++ = SampleValue;
			    ++RunningSampleIndex;

			}
			GlobalDSoundSecondaryBuffer->Unlock(
			    &Region1,
			    Region1Size,
			    &Region2,
			    Region2Size
			    );

		    }
	
		}
		
		    
		// Draw pixels on our screen
		HDC DeviceContext = GetDC(WindowHandle);
		win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
		Win32DisplayBufferInDeviceContext(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
		ReleaseDC(WindowHandle, DeviceContext);
		++XOffset;
	    }
	}
	else
	{
	    // @TODO handle error
	}
    }
    else
    {
	// @TODO handle error
    }
    
}

