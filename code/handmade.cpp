#include "handmade.h"

internal void GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz) {
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16* SampleOut = SoundBuffer->SampleOut;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		GameState->tSine += 2.8f*Pi32*1.0f/(real32)WavePeriod;
		if (GameState->tSine > 2.0f*Pi32) {
			GameState->tSine -= 2.0f*Pi32;
		}
	}
}

internal void RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset) {
	uint8* Row = (uint8*)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer->Width; ++X) {
			uint8 Blue = (uint8)(X + BlueOffset);
			uint8 Green = (uint8)(Y + GreenOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state* GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->IsInitalized) {
		char* Filename = __FILE__;

		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents) {
			Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			Memory->DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 256;
		GameState->tSine = 0;

		// todo(jax): This may be more appropriate to do in the platform layer.
		Memory->IsInitalized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog) {
			GameState->ToneHz = 256 + (int)(128.0f*Controller->StickAverageX);
			GameState->BlueOffset += (int)(4.0f*Controller->StickAverageY);
		} else {
			if (Controller->MoveLeft.EndedDown) {
				GameState->BlueOffset -= 1;
			}

			if (Controller->MoveRight.EndedDown) {
				GameState->BlueOffset += 1;
			}
		}

		if (Controller->ActionDown.EndedDown){
			GameState->GreenOffset += 1;
		}
	}

    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
	game_state* GameState = (game_state*)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

#if HANDMADE_WIN32
#include "windows.h"
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	return TRUE;
}
#endif