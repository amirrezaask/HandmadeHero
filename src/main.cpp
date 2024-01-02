#include <windows.h>
#include <stdint.h>
#include <xinput.h>


#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* Vibration)
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)

typedef X_INPUT_SET_STATE(x_input_set_state);
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}

X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}

static x_input_get_state* DyXInputGetState = XInputGetStateStub;
static x_input_set_state *DyXInputSetState = XInputSetStateStub;

#define XInputGetState DyXInputGetState
#define XInputSetState DyXInputSetState

static void
Win32LoadXInputLibrary()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (XInputLibrary)
    {
	XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
	XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
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
	    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	    Running = true;
	    int XOffset = 0;
	    int YOffset = 0;
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

