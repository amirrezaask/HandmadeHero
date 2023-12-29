#include <windows.h>
#include <stdio.h>

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
    case WM_DESTROY:
	{
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
	    MSG Msg;
	    while(GetMessageA(&Msg, WindowHandle, 0, 0) > 0)
	    {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
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

