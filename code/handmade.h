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

// todo(jax): Services that the platform layer provides to the game.

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
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	union {
		game_button_state Buttons[6];
		struct {
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input {
	// todo(jax): Insert clock values here?

	game_controller_input Controllers[4];
};

struct game_memory {
	bool32 IsInitalized;
	uint64 PermanentStorageSize;
	void* PermanentStorage; // note(jax): REQUIRED to be cleared to zero at startup!!!

	uint64 TransientStorageSize;
	void* TransientStorage; // note(jax): REQUIRED to be cleared to zero at startup!!!
};

void GameUpdateAndRender(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer);

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