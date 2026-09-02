#ifndef LIGHT_TILE_CULLING_COMMON_HLSLI
#define LIGHT_TILE_CULLING_COMMON_HLSLI

// keep in sync with LightCullingPass::TILE_SIZE
static const uint TILE_SIZE = 8;
static const uint MAX_LIGHTS_PER_TILE = 64;

// tileCount must come from SceneData, not screen size - they can disagree on resize
uint GetTileIndex(uint2 pixelCoord, uint2 tileCount)
{
    const uint2 tileCoord = min(pixelCoord / TILE_SIZE, max(tileCount, 1u) - 1u);
    return tileCoord.y * tileCount.x + tileCoord.x;
}

#endif
