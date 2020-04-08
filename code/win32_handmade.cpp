#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Instance, UINT Message, WPARAM wParam, LPARAM lParam) {
	LRESULT Result = 0;

	switch (Message) {
	case WM_CREATE: {
	} break;

	case WM_SIZE: {
	} break;

	case WM_DESTROY: {
	} break;

	case WM_CLOSE: {
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

		static DWORD Op = WHITENESS;
		PatBlt(DeviceContext, X, Y, Width, Height, Op);
		Op = (Op == WHITENESS) ? BLACKNESS : WHITENESS;

		EndPaint(Instance, &Paint);
	} break;

	default: {
		Result = DefWindowProc(Instance, Message, wParam, lParam);
	} break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int nCmdShow) {
	MessageBox(0, "Is this thing working?", "Handmade Hero", MB_OK|MB_ICONINFORMATION);

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND WindowHandle = CreateWindowEx(0, 
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

		if (WindowHandle) {
			for (;;) {
				MSG Message;
				BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);
				if (MessageResult > 0) {
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				} else {
					break;
				}
			}
		} else {

		}
	}

    return 0;
}