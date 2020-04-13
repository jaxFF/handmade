/*
	todo(jax): THIS IS NOT A FINAL PLATFORM LAYER!!!

	- Saved game locations
	- Getting a handle to our own executable files
	- Asset loading path
	- Threading (launch a thread)
	- Raw Input (support multiple keyboards & finer mouse movements)
	- Sleep / timeBeginPeriod
	- ClipCursor() (multi-monitor support)
	- Fullscreen support
	- WM_SETCURSOR (control cursor visibility)
	- QueryCancelAutoplay
	- WM_ACTIVATEAPP (for when we are not the active app)
	- Blit speed improvements (BitBlit)
	- Hardware acceleration (OpenGL or Direct3D or BOTH???)
	- GetKeyboardLayout (for French kbs, international WASD support)

	Just a partial list of stuff!!!
*/

// todo(jax): Implement sine ourselves
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.h"
#include "handmade.cpp"

#include <malloc.h>
#include <windows.h>
#include <stdio.h> // for sprintf
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define XINPUTGETSTATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUTGETSTATE(x_input_get_state);

XINPUTGETSTATE(XInputGetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUTSETSTATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUTSETSTATE(x_input_set_state);

XINPUTSETSTATE(XInputSetStateStub) {
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECTSOUNDCREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECTSOUNDCREATE(direct_sound_create);

internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename) {
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize)) {
			// note(jax): Windows can't read files greater than 4GB via ReadFile. Since this
			// is debug code, we can assure that we won't be reading files that large here.
			// Assert(FileSize.QuadPart <= 0xFFFFFFFF);
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents) {
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)) {
					// note(jax): File read successfully
					Result.ContentsSize = FileSize32;
				} else { // note(jax): Allocation failure!!!
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			} else {
				// todo(jax): Logging
			}
		} else {
			// todo(jax): Logging
		}

		CloseHandle(FileHandle);
	} else {
		// todo(jax): Logging
	}

	return Result;
}

internal void DEBUGPlatformFreeFileMemory(void* Memory) {
	if (Memory) {
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory) {
	bool32 Result = false;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
			// note(jax): File read successfully
			Result = (BytesWritten == MemorySize);
		} else { // note(jax): Allocation failure!!!

		}

		CloseHandle(FileHandle);
	} else {
		// todo(jax): Logging
	}

	return Result;
}

internal void Win32LoadXInput() {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");	
	if (!XInputLibrary) { 
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}

	if (!XInputLibrary) { 
		XInputLibrary = LoadLibraryA("xinput1_4.dll");
	}

	if (XInputLibrary) {
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	} else {

	}
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
	// note(jax): Load the library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary) {
		// note(jax): Get a DirectSound object
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
			WAVEFORMATEX WaveFormat = { };
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(DSBUFFERDESC);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error)) {
						// note(jax): finally set the format!
						OutputDebugStringA("Primary Buffer was set.\n");
					} else {

					}
				} else {

				}

				BufferDescription = {};
				BufferDescription.dwSize = sizeof(DSBUFFERDESC);
				BufferDescription.dwFlags = 0;
				BufferDescription.dwBufferBytes = BufferSize;
				BufferDescription.lpwfxFormat = &WaveFormat;

				HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
				if (SUCCEEDED(Error)) {
					OutputDebugStringA("Secondary Buffer was created.\n");
				}
				else {

				}
			} else {

			}

		}
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight) {
	StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Instance, UINT Message, WPARAM wParam, LPARAM lParam) {
	LRESULT Result = 0;

	switch (Message) {
	case WM_ACTIVATEAPP: {
	} break;

	case WM_DESTROY: {
		// todo(jax): Handle this as an error -- recreate window?? how about swapping renderers mid game?
		GlobalRunning = false;
	} break;

	case WM_CLOSE: {
		// todo(jax): Handle this with a message to user
		GlobalRunning = false;
	} break;

#if 0
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP: {
		Assert(!"Keyboard input came in through a non-dispatch message!");
	} break;
#endif

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
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

		EndPaint(Instance, &Paint);
	} break;

	default: {
		Result = DefWindowProcA(Instance, Message, wParam, lParam);
	} break;
	}

	return Result;
}

internal void Win32ClearSoundBuffer(win32_sound_output* SoundOutput) {
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0))) { 
		int8* DestSample = (int8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
			*DestSample++ = 0;
		}

		DestSample = (int8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer* SourceBuffer) {
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16* DestSample = (int16*)Region1;
		int16* SourceSample = SourceBuffer->SampleOut;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16*)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown) {
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldState, DWORD ButtonBit, game_button_state* NewState) {
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown == !NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold) {
	real32 Result = 0;
	if (Value < -DeadZoneThreshold) {
		Result = (real32)Value / 32768.0f;
	} else if (Value > DeadZoneThreshold) {
		Result = (real32)Value / 32767.0f;
	}

	return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input* KeyboardController) {
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
		switch (Message.message) {
			case WM_QUIT: {
				GlobalRunning = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP: {
				uint32 VKCode = (uint32)Message.wParam;
				bool WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool IsDown = ((Message.lParam & (1 << 31)) == 0);

				if (WasDown != IsDown) {
					if (VKCode == 'W') {
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if (VKCode == 'A') {
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if (VKCode == 'S') {
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if (VKCode == 'D') {
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if (VKCode == 'Q') {
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E') {
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_UP) {
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_DOWN) {
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
					}
					else if (VKCode == VK_LEFT) {
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if (VKCode == VK_RIGHT) {
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE) {
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
					else if (VKCode == VK_SPACE) {
						Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
					}
				}

				bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
				if ((VKCode == VK_F4) && AltKeyWasDown) {
					GlobalRunning = false;
				}
			} break;

			default: {
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			} break;
		}
	}
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int nCmdShow) {
	LARGE_INTEGER PerfFreq;
	QueryPerformanceFrequency(&PerfFreq);
	uint64 PerfCounterFrequency = PerfFreq.QuadPart;

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&WindowClass)) {
		HWND Window = CreateWindowExA(0, 
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
			HDC DeviceContext = GetDC(Window);
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 40000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.tSine = 0;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif
			
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(4);

			// todo(jax): Handle various memory footprints (USING SYSTEM_METRICS)
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);
				int64 LastCycleCount = __rdtsc();
				while (GlobalRunning) {
					// todo(jax): Zeroing macro
					// todo(jax): We can't zero everything because the up/down state will
					// be wrong!!!
					game_controller_input* OldKeyboardController = GetController(OldInput, 0);
					game_controller_input* NewKeyboardController = GetController(NewInput, 0);
					game_controller_input ZeroController = {};
					*NewKeyboardController = ZeroController;
					NewKeyboardController->IsConnected = true;
					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex) {
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(NewKeyboardController);

					// todo(jax): Need to avoid polling disconnected controllers to avoid
					// xinput frame rate hit on older library revisions.
					// todo(jax): Should we poll this more frequently
					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1)) {
						MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
					}

					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex) {
						DWORD OurControllerIndex = ControllerIndex + 1;
						game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input* NewController = GetController(NewInput, OurControllerIndex);

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
							NewController->IsConnected = true;
							// note(jaxon): Controller is plugged in
							// todo(jaxon): See if ControllerState.dwPacketNumber increments too rapidly
							XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

							// note(jax): Consider applying different linear response curve transformations on
							// the deadzone, like Modern Warfare does.
							// https://docs.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput#dead-zone
							NewController->IsAnalog = true;
							NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
								NewController->StickAverageY = 1.0f;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
								NewController->StickAverageY = -1.0f;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
								NewController->StickAverageX = -1.0f;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
								NewController->StickAverageX = 1.0f;
							}

							// todo(jax): Min/max macros!!!

							real32 Threshold = 0.5f;
							Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);
							Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);					
							Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);
							Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);

							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);						
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
							Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK, &NewController->Back);
						} else {
							NewController->IsConnected = false;
						}
					}

					XINPUT_VIBRATION Vibration;
					Vibration.wLeftMotorSpeed = 60000;
					Vibration.wRightMotorSpeed = 60000;
					XInputSetState(0, &Vibration);

					DWORD ByteToLock = 0;
					DWORD TargetCursor;
					DWORD BytesToWrite = 0;
					DWORD PlayCursor;
					DWORD WriteCursor;
					bool32 SoundIsValid = false;
					// todo(jax): Tighten up sound logic so that we know where we should be
					// writing to and can anticipate the time spent in the game update.
					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
						ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
						
						TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
						if (ByteToLock > TargetCursor) {
							BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						} else {
							BytesToWrite = TargetCursor - ByteToLock;
						}
						
						SoundIsValid = true;
					}
					
					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.SampleOut = Samples;

					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBackbuffer.Memory;
					Buffer.Width = GlobalBackbuffer.Width;
					Buffer.Height = GlobalBackbuffer.Height;
					Buffer.Pitch = GlobalBackbuffer.Pitch;
					GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

					// note(jax): DirectSound output test
					if (SoundIsValid) {
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}

					win32_window_dimension Dimension = GetWindowDimension(Window);
					Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);

					uint64 EndCycleCount = __rdtsc();

					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					real64 MSPerFrame = ((1000 * (real64)CounterElapsed) / (real64)PerfCounterFrequency);
					real64 FPS = (real64)PerfCounterFrequency / (real64)CounterElapsed;
					real64 MCPF = (real64)(CyclesElapsed / (1000 * 1000));

	#if 1
					char CharBuffer[256];
					sprintf(CharBuffer, "%.02fms, %.02fFPS, %.02fmc/frame\n", MSPerFrame, FPS, MCPF); // MSPerFrame, FPS, MegacyclesPerFrame
					OutputDebugStringA(CharBuffer);
	#endif

					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;

					game_input* Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
				}

				ReleaseDC(Window, DeviceContext);	
			} else {
				// note(jax): GameMemory failed
			}
		} else {

		}
	}

    return 0;
}