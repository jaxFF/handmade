#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

struct tile_map_position {
	// NOTE(jax): These are fixed point tile locations.    The high
	// bits are the tile chunk index, and the low bits are the tile
	// index in the chunk.
	uint32 AbsTileX;
	uint32 AbsTileY;

	// TODO(jax): Should these be from the center of a tile?
	// note(jax): Rename to offsetX and Y
	real32 RelTileX;
	real32 RelTileY;
};

struct tile_chunk_position {
	uint32 TileChunkX;
	uint32 TileChunkY;

	uint32 RelTileX;
	uint32 RelTileY;
};

struct tile_chunk {
	uint32* Tiles;
};

struct tile_map {
	uint32 ChunkShift;
	uint32 ChunkMask;
	uint32 ChunkDim;

	real32 TileSizeInMeters;
	int32 TileSizeInPixels;
	real32 MetersToPixels;

	// todo(jax): Beginner's sparseness
	uint32 TileChunkCountX; 
	uint32 TileChunkCountY;

	tile_chunk* TileChunks;
};

#endif