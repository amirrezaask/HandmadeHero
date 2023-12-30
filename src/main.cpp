#include <windows.h>
#include <stdio.h>
#include <stdint.h>

static bool Running;

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixel = 4;

void
Win32RenderGradient(int XOffset, int YOffset)
{
    /* @Note(amirreza):
      Our BitmapMemory is a 1D space in memory,
      but we need to interpret it as a 2D Matrix of pixels,
     */
    
    uint8_t* Row = (uint8_t*)BitmapMemory; // Pointer to each row in our BitmapMemory
    int Pitch = BitmapWidth*BytesPerPixel; // Amount of byte offset that we need to go from start of the row to start of next row.

    for (int Y = 0; Y < BitmapHeight; ++Y)
	{
	    uint32_t* Pixel = (uint32_t*)Row;
	    for (int X = 0; X < BitmapWidth; ++X)
		{
		    //Note(Amirreza): Little Endian
		    // Memory : BB GG RR xx
		    // Register : xx RR GG BB
		    int Blue = X + XOffset;
		    int Green = Y + YOffset;
		    uint32_t Color = (Green << 16 | Blue);
		    *Pixel = Color;
		    ++Pixel;
		}
	    Row += Pitch;
    
	}
}

void
Win32ResizeDIBSection(int Width, int Height)
{

    // Free last memory allocated by us
    if (BitmapMemory)
    {
	VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;
    
    // Create Bitmap info, basically metadata about our bitmap buffer
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    // Allocate actual memory for bitmap.
    int BitmapMemorySize = 4 * BitmapWidth * BitmapHeight;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

void
Win32UpdateWindow(HDC DeviceContext, RECT* rect)
{
    int WindowWidth = rect->right - rect->left;
    int WindowHeight = rect->bottom - rect->top;
    
    int result = StretchDIBits(
		  DeviceContext,
		  0, 0, WindowWidth, WindowHeight,
		  0, 0, BitmapWidth, BitmapHeight,
		  BitmapMemory,
		  &BitmapInfo,
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
	    RECT rect;
	    GetClientRect(Window, &rect);
	    int X = rect.left;
	    int Y = rect.top;
	    int Width = rect.right - rect.left;
	    int Height = rect.bottom - rect.top;
	    Win32ResizeDIBSection(Width, Height);
	    OutputDebugStringA("WM_SIZE\n");
	} break;
    case WM_PAINT:
	{
	    PAINTSTRUCT Paint;
	    HDC DeviceContext = BeginPaint(Window, &Paint);
	    RECT rect;
	    GetClientRect(Window, &rect);
	    int Width = rect.right - rect.left;
	    int Height = rect.bottom - rect.top;
	    Win32UpdateWindow(DeviceContext, &rect);
	    EndPaint(Window, &Paint);
	    
	} break;
    default:
	
	Result = DefWindowProcA(Window, Message, WParam, LParam);
    }

    return (Result);
}

    
int WINAPI WinMain(HINSTANCE Instance,
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

		Win32RenderGradient(XOffset, YOffset);

		HDC DeviceContext = GetDC(WindowHandle);
		RECT rect;
		GetClientRect(WindowHandle, &rect);
		Win32UpdateWindow(DeviceContext, &rect);
		ReleaseDC(WindowHandle, DeviceContext);
		++XOffset;
		++YOffset;
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

