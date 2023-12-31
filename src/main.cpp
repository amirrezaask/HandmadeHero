#include <windows.h>
#include <stdint.h>

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
Win32RenderGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    /* @Note(amirreza):
      Our BitmapMemory is a 1D space in memory,
      but we need to interpret it as a 2D Matrix of pixels,
     */
    
    uint8_t* Row = (uint8_t*)Buffer.Memory; // Pointer to each row in our BitmapMemory

    for (int Y = 0; Y < Buffer.Height; ++Y)
	{
	    uint32_t* Pixel = (uint32_t*)Row;
	    for (int X = 0; X < Buffer.Width; ++X)
		{
		    //Note(Amirreza): Little Endian
		    // Memory : BB GG RR xx
		    // Register : xx RR GG BB
		    int Blue = X + XOffset;
		    int Green = Y + YOffset;
		    uint32_t Color = (Green << 8 | Blue);
		    *Pixel = Color;
		    ++Pixel;
		}
	    Row += Buffer.Pitch;
    
	}
}

static void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
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
Win32DisplayBufferInDeviceContext(HDC DeviceContext, int Width, int Height, win32_offscreen_buffer Buffer)
{
    int result = StretchDIBits(
		  DeviceContext,
		  0, 0, Width, Height,
		  0, 0, Buffer.Width, Buffer.Height,
		  Buffer.Memory,
		  &Buffer.Info,
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
	    Win32DisplayBufferInDeviceContext(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
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

    WNDCLASS WindowClass = {};
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
		MSG Message;
		while(PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE))
		{
		    if (Message.message == WM_QUIT) Running = false;
		    TranslateMessage(&Message);
		    DispatchMessage(&Message);
		}

		Win32RenderGradient(GlobalBackBuffer, XOffset, YOffset);

		HDC DeviceContext = GetDC(WindowHandle);
		win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
		Win32DisplayBufferInDeviceContext(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackBuffer);
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

