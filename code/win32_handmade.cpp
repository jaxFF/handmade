#include <windows.h>
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer {
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel = 4;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension {
	int Width;
	int Height;
};

win32_window_dimension GetWindowDimension(HWND Window) {
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void RenderWeirdGradient(win32_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset) {
	uint8* Row = (uint8*)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer->Width; ++X) {
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = ((Green << 8 ) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

// Resize or initalize a Device Independent Buffer
internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height) {
	// todo(jax): Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if (Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

	// tood(jax): Probably want to clear this to black.
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight, int X, int Y, int Width, int Height) {
	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Instance, UINT Message, WPARAM wParam, LPARAM lParam) {
	LRESULT Result = 0;

	switch (Message) {
	case WM_CREATE: {
	} break;

	case WM_SIZE: {
	} break;

	case WM_DESTROY: {
		// todo(jax): Handle this as an error -- recreate window?? how about swapping renderers mid game?
		Running = false;
	} break;

	case WM_CLOSE: {
		// todo(jax): Handle this with a message to user
		Running = false;
	} break;

	case WM_ACTIVATEAPP: {
	} break;

	case WM_PAINT: {
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Instance, &Paint);

		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

		win32_window_dimension Dimension = GetWindowDimension(Instance);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height, X, Y, Width, Height);

		EndPaint(Instance, &Paint);
	} break;

	default: {
		Result = DefWindowProc(Instance, Message, wParam, lParam);
	} break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int nCmdShow) {
	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND Window = CreateWindowEx(0, 
			WindowClass.lpszClassName, 
			"Handmade Hero x64", 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
			0, 
			0, 
			1280, 
			720, 
			0, 
			0, 
			Instance, 
			0);

		if (Window) {
			int XOffset = 0;
			int YOffset = 0;
			Running = true;
			while (Running) {
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if (Message.message == WM_QUIT)
						Running = false;

					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height, 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
				YOffset += 2;
			}
		} else {

		}
	}

    return 0;
}