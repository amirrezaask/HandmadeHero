#include <windows.h>
#include <stdio.h>

static bool Running;

static BITMAPINFO BitmapInfo;
static void *BitmapMem;
static HBITMAP BitmapHandle;
static HDC BitmapDeviceContext;


void Win32ResizeDIBSection(int Width, int Height)
{
    if (BitmapHandle)
    {
	DeleteObject(BitmapHandle);
    }
    else
    {
	BitmapDeviceContext = CreateCompatibleDC(0);
    }

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMem, 0, 0);
}

void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(
		  DeviceContext,
		  X, Y, Width, Height,
		  X, Y, Width, Height,
		  &BitmapMem,
		  &BitmapInfo,
		  DIB_RGB_COLORS,
		  SRCCOPY
		  );
}

LRESULT CALLBACK MainWindowCallback(
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
	    int X = Paint.rcPaint.left;
	    int Y = Paint.rcPaint.top;
	    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
	    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
	    static DWORD Operation = WHITENESS;
	    PatBlt(DeviceContext, X, Y, Width, Height, Operation);
	    if (Operation == WHITENESS)
	    {
			
		OutputDebugStringA("Whiteness\n");
		Operation = BLACKNESS;
	    }
	    else
	    {
		OutputDebugStringA("Blackness\n");
		Operation = WHITENESS;
	    }
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
	    MSG Msg;
	    while(Running) {
		BOOL GetMessageResult = GetMessageA(&Msg, WindowHandle, 0, 0) > 0;
		if (GetMessageResult > 0)
		{
		    TranslateMessage(&Msg);
		    DispatchMessage(&Msg);
		}
		else break;
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

