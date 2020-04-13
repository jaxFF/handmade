#ifndef HANDMADE_H

/*
	note(jax):

	HANDMADE_INTERNAL:
		0 - Build for public release
		1 - Build for developer only

	HANDMADE_SLOW:
		0 - No slow code allowed
		1 - Slow code permitted
*/

// note(jax): Platform-independent way to perform an assertion.
// Flat out writes to zero memory to crash the program.
#if HANDMADE_SLOW
#define Assert(Expression) if (!(Expression)) { *(int*)0=0; }
#else
#define Assert(Expression)
#endif

// todo(jax): Should these always be 64-bit?
#define Kilobytes(Value) (((uint64)Value) * 1024LL)
#define Megabytes(Value) (Kilobytes((uint64)Value) * 1024LL)
#define Gigabytes(Value) (Megabytes((uint64)Value) * 1024LL)
#define Terabytes(Value) (Gigabytes((uint64)Value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
// todo(jax): swap, min, max ... macros???

inline uint32 SafeTruncateUInt64(uint64 Value) {
	// todo(jax): Defines for min/max values UInt32Max
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;
}

// note(jax): Services that the platform layer provides to the game.
#if HANDMADE_INTERNAL
/* important(jax):

	These are NOT for doing anything in the shipped game - they are
	blocking and the write doesn't protect against lost data!
*/
struct debug_read_file_result {
	uint32 ContentsSize;
	void* Contents;
};

internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename);
internal void DEBUGPlatformFreeFileMemory(void* Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory);
#endif

// note(jax): Services that the game provides to the platform layer.
// (this may expand in the future -- sound on seperate thread, etc..)

// FOUR THINGS - timings, controllor/kb input, bitmap buffer to use, sound buffer to use

// todo(jax): In the future, rendering _specifically_ will become a three-tiered abstraction!!!
struct game_offscreen_buffer {
	void* Memory; // note(jax): Pixels are always 32-bits wide, so Memory Order BB GG RR XX
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel = 4;
};

struct game_sound_output_buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16* SampleOut;
};

struct game_button_state {
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input {
	bool32 IsConnected;
	bool32 IsAnalog;
	real32 StickAverageX;
	real32 StickAverageY;

	union {
		game_button_state Buttons[12];
		struct {
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Start;
			game_button_state Back;
		};
	};
};

struct game_input {
	// todo(jax): Insert clock values here?
	game_controller_input Controllers[5];
};

inline game_controller_input* GetController(game_input* Input, int ControllerIndex) {
	Assert(ControllerIndex < ArrayCount(Input->Controllers));

	game_controller_input* Result = &Input->Controllers[ControllerIndex];
	return Result;
}

struct game_memory {
	bool32 IsInitalized;
	uint64 PermanentStorageSize;
	void* PermanentStorage; // note(jax): REQUIRED to be cleared to zero at startup!!!

	uint64 TransientStorageSize;
	void* TransientStorage; // note(jax): REQUIRED to be cleared to zero at startup!!!
};

internal void GameUpdateAndRender(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer);

//
//
//

struct game_state {
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};

#define HANDMADE_H
#endif