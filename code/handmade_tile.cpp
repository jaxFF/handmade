inline void RecanonicalizeCoord(tile_map* TileMap, uint32* Tile, real32* TileRel) {
	// TODO(jax): Need to do something that doesn't use the divide/multiply method
	// for recanonicaling because this can end up rounding back on the tile
	// you just came from.

	// NOTE(jax): The tile_map is assumed to be torodial, if you step off one end you
	// come back on the other!
	int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSizeInMeters);
	*Tile += Offset;
	*TileRel -= Offset*TileMap->TileSizeInMeters;

	Assert(*TileRel >= -0.5f*TileMap->TileSizeInMeters);
	Assert(*TileRel <= 0.5f*TileMap->TileSizeInMeters);
}

inline tile_map_position RecanonicalizePosition(tile_map* TileMap, tile_map_position Pos) {
	tile_map_position Result = Pos;

	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.RelTileX);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.RelTileY);

	return Result;
}

inline tile_chunk* GetTileChunk(tile_map* TileMap, uint32 TileChunkX, uint32 TileChunkY) {
	tile_chunk* TileChunk = 0;
	if (TileChunkX < TileMap->TileChunkCountX && TileChunkY < TileMap->TileChunkCountY) {
		TileChunk = &TileMap->TileChunks[TileChunkY*TileMap->TileChunkCountX + TileChunkX];
	}

	return TileChunk;
}

inline uint32 GetTileValueUnchecked(tile_map* TileMap, tile_chunk* TileChunk, uint32 TileX, uint32 TileY) {
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim); 
	Assert(TileY < TileMap->ChunkDim);

	uint32 TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
	return TileChunkValue;
}

inline void SetTileValueUnchecked(tile_map* TileMap, tile_chunk* TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim); 
	Assert(TileY < TileMap->ChunkDim);

	TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

inline uint32 GetTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY) {
	uint32 TileChunkValue = 0;

	if (TileChunk) {
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}

	return TileChunkValue;
}

inline void SetTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY, uint32 TileValue) {
	if (TileChunk) {
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}

inline tile_chunk_position GetChunkPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY) {
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.RelTileX = AbsTileX & TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & TileMap->ChunkMask;

	return Result;
}

internal uint32 GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY) {
	bool32 Empty = false;

	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

	return TileChunkValue;
}

internal bool32 IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos) {
	uint32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
	bool32 Empty = (TileChunkValue == 0);

	return Empty;
}

internal void SetTileValue(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue) {
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    
    // TODO(jax): On-demand tile chunk creation
    Assert(TileChunk);
    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}