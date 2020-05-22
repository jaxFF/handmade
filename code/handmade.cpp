#include "handmade.h"
#include "handmade_intrinsics.h"

internal void GameOutputSound(thread_context* Thread, game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz) {
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	
	int16* SampleOut = SoundBuffer->SampleOut;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
		int16 SampleValue = 0;
#endif
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

#if 0
		GameState->tSine += 2.8f*Pi32*1.0f/(real32)WavePeriod;
		if (GameState->tSine > 2.0f*Pi32) {
			GameState->tSine -= 2.0f*Pi32;
		}
#endif
	}
}

internal void DrawRectangle(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, real32 R, real32 G, real32 B) {
	// todo(jax): Floating point color tomorrow!!!
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);

	if (MinX < 0) {
		MinX = 0;
	}

	if (MinY < 0) {
		MinY = 0;
	}

	if (MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}

	if (MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}

	uint32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) | (RoundReal32ToUInt32(G * 255.0f) << 8) | (RoundReal32ToUInt32(B * 255.0f) << 0));

	uint8* EndOfBuffer = ((uint8*)Buffer->Memory + Buffer->Pitch*Buffer->Height);
	uint8* Row = ((uint8*)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			*Pixel++ = Color;
		}

		Row += Buffer->Pitch;
	}
}

inline tile_chunk* GetTileChunk(world* World, int32 TileChunkX, int32 TileChunkY) {
	tile_chunk* TileChunk = 0;
	if (TileChunkX < World->TileChunkCountX && TileChunkY < World->TileChunkCountY) {
		TileChunk = &World->TileChunks[TileChunkY*World->TileChunkCountX + TileChunkX];
	}

	return TileChunk;
}

inline uint32 GetTileValueUnchecked(world* World, tile_chunk* TileChunk, uint32 TileX, uint32 TileY) {
	Assert(TileChunk);
	Assert(TileX < World->ChunkDim); 
	Assert(TileY < World->ChunkDim);

	uint32 TileChunkValue = TileChunk->Tiles[TileY*World->ChunkDim + TileX];
	return TileChunkValue;
}

inline uint32 GetTileValue(world* World, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY) {
	uint32 TileChunkValue = 0;

	if (TileChunk) {
		TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

inline void RecanonicalizeCoord(world* World, uint32* Tile, real32* TileRel) {
	// TODO(jax): Need to do something that doesn't use the divide/multiply method
	// for recanonicaling because this can end up rounding back on the tile
	// you just came from.

	// NOTE(jax): The world is assumed to be torodial, if you step off one end you
	// come back on the other!
	int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSizeInMeters);
	*Tile += Offset;
	*TileRel -= Offset*World->TileSizeInMeters;

	Assert(*TileRel >= 0);
	Assert(*TileRel <= World->TileSizeInMeters);
}

inline world_position RecanonicalizePosition(world* World, world_position Pos) {
	world_position Result = Pos;

	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.RelTileX);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.RelTileY);

	return Result;
}

inline tile_chunk_position GetChunkPositionFor(world* World, uint32 AbsTileX, uint32 AbsTileY) {
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> World->ChunkShift;
	Result.TileChunkY = AbsTileY >> World->ChunkShift;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;

	return Result;
}

internal uint32 GetTileValue(world* World, uint32 AbsTileX, uint32 AbsTileY) {
	bool32 Empty = false;

	tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
	tile_chunk* TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

	return TileChunkValue;
}

internal bool32 IsWorldPointEmpty(world* World, world_position CanPos) {
	uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
	bool32 Empty = (TileChunkValue == 0);

	return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
	uint32 TempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	};

	world World;
	// NOTE(jax): This is set to using 256x256 tile chunks.
	World.ChunkShift = 8;
	World.ChunkMask = 0xFF;
	World.ChunkDim = 256;

	World.TileChunkCountX = 1;
	World.TileChunkCountY = 1;

	tile_chunk TileChunk;
	TileChunk.Tiles = (uint32*)TempTiles;
	World.TileChunks = &TileChunk;

	World.TileSizeInMeters = 1.4f;
	World.TileSizeInPixels = 60;
	World.MetersToPixels = (real32)World.TileSizeInPixels/(real32)World.TileSizeInMeters;

	real32 PlayerHeight = 1.4f;
	real32 PlayerWidth = 0.75f*PlayerHeight;

	real32 LowerLeftX = -(real32)World.TileSizeInPixels/2;
	real32 LowerLeftY = (real32)Buffer->Height;

	game_state* GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->IsInitalized) {
		GameState->PlayerP.AbsTileX = 1;
		GameState->PlayerP.AbsTileY = 3;
		GameState->PlayerP.RelTileX = 5.0f;
		GameState->PlayerP.RelTileY = 5.0f;
		Memory->IsInitalized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog) {
		} else {
			// NOTE(jax): Use digital movement tuning
			real32 dPlayerX = 0.0f; // pixels/second
			real32 dPlayerY = 0.0f; // pixels/second
			if (Controller->MoveUp.EndedDown) {
				dPlayerY = 1.0f;
			}			

			if (Controller->MoveDown.EndedDown) {
				dPlayerY = -1.0f;
			}

			if (Controller->MoveLeft.EndedDown) {
				dPlayerX = -1.0f;
			}

			if (Controller->MoveRight.EndedDown) {
				dPlayerX = 1.0f;
			}
			dPlayerX *= 3.0f;
			dPlayerY *= 3.0f;

			// TODO(jax): Diagonal will be faster! Fix once we have vectors :)
			world_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.RelTileX += Input->dtForFrame * dPlayerX;
			NewPlayerP.RelTileY += Input->dtForFrame * dPlayerY;
			NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);
			// TODO(jax): Delta function that auto-recanonicalizes
			
			world_position PlayerLeft = NewPlayerP;
			PlayerLeft.RelTileX -= 0.5f*PlayerWidth;
			PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

			world_position PlayerRight = NewPlayerP;
			PlayerRight.RelTileX += 0.5f*PlayerWidth;
			PlayerRight = RecanonicalizePosition(&World, PlayerRight);

			if (IsWorldPointEmpty(&World, NewPlayerP) &&
			IsWorldPointEmpty(&World, PlayerLeft) &&
			IsWorldPointEmpty(&World, PlayerRight)) {
				GameState->PlayerP = NewPlayerP;
			}
		}
	}

	DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 1.0f, 0.0f, 0.1f);

	real32 CenterX = 0.5f*(real32)Buffer->Width;
	real32 CenterY = 0.5f*(real32)Buffer->Height;

	for (int32 RelRow = -10; RelRow < 10; ++RelRow) {
		for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn) {
			uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
			uint32 Row = GameState->PlayerP.AbsTileY + RelRow;

			uint32 TileID = GetTileValue(&World, Column, Row);
			real32 Gray = 0.5f;
			if (TileID == 1) {
				Gray = 1.0f;
			}

			if ((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) {
				Gray = 0.0f;
			}

			real32 MinX = CenterX + ((real32)RelColumn)*World.TileSizeInPixels;
			real32 MinY = CenterY - ((real32)RelRow)*World.TileSizeInPixels;
			real32 MaxX = MinX + World.TileSizeInPixels;
			real32 MaxY = MinY - World.TileSizeInPixels;
			DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
		}
	}

	real32 PlayerR = 1.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0.0f;

	real32 PlayerLeft = CenterX + World.MetersToPixels*GameState->PlayerP.RelTileX - 0.5f*World.MetersToPixels*PlayerWidth;
	real32 PlayerTop = CenterY - World.MetersToPixels*GameState->PlayerP.RelTileY - World.MetersToPixels*PlayerHeight;
	DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + World.MetersToPixels*PlayerWidth, PlayerTop + World.MetersToPixels*PlayerHeight, PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
	game_state* GameState = (game_state*)Memory->PermanentStorage;
	GameOutputSound(Thread, GameState, SoundBuffer, 400);
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
