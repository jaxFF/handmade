#include <windows.h>
#include <stdint.h>
#include <xinput.h>

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

struct win32_window_dimension {
	int Width;
	int Height;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

#define XINPUTGETSTATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUTGETSTATE(x_input_get_state);

XINPUTGETSTATE(XInputGetStateStub) {
	return 0;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUTSETSTATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUTSETSTATE(x_input_set_state);

XINPUTSETSTATE(XInputSetStateStub) {
	return 0;
}

global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput() {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (XInputLibrary) {
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal win32_window_dimension GetWindowDimension(HWND Window) {
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
	case WM_ACTIVATEAPP: {
	} break;

	case WM_DESTROY: {
		// todo(jax): Handle this as an error -- recreate window?? how about swapping renderers mid game?
		Running = false;
	} break;

	case WM_CLOSE: {
		// todo(jax): Handle this with a message to user
		Running = false;
	} break;


	case WM_SYSKEYDOWN: {

	} break;

	case WM_SYSKEYUP: {

	} break;

	case WM_KEYDOWN: {

	} break;

	case WM_KEYUP: {
		uint32 VKCode = wParam;
		bool WasDown = ((lParam & (1 << 30)) != 0);
		bool IsDown = ((lParam & (1 << 31)) == 0);

		if (VKCode == 'W') {

		} else if (VKCode == 'A') {

		} else if (VKCode == 'S') {

		} else if (VKCode == 'D') {

		} else if (VKCode == 'Q') {

		} else if (VKCode == 'E') {

		} else if (VKCode == VK_UP) {

		} else if (VKCode == VK_DOWN) {

		} else if (VKCode == VK_LEFT) {

		} else if (VKCode == VK_RIGHT) {

		} else if (VKCode == VK_ESCAPE) {

		} else if (VKCode == VK_SPACE) {
			if (IsDown) {
				OutputDebugStringA("DOWN\n");
			}
		}
	} break;

	// todo(jax): RAW INPUT
	case WM_INPUT: {

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
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

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

				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
						// note(jaxon): controller is plugged in
						// todo(jaxon): See if ControllerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickLX = Pad->sThumbLX;
						int16 StickLY = Pad->sThumbLY;

						if (AButton) {
							++YOffset;
						}
					} else {

					}
				}

				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0, &Vibration);

				RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);

				win32_window_dimension Dimension = GetWindowDimension(Window);
				Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height, 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
			}
		} else {

		}
	}

    return 0;
}