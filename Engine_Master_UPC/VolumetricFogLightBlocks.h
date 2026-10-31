#ifndef VOLUMETRIC_FOG_LIGHT_BLOCKS_H
#define VOLUMETRIC_FOG_LIGHT_BLOCKS_H

// Shared by allocation, culling and lighting. One bit per supported point light:
// no per-block truncation or overflow, even when every light overlaps a block.
#define FOG_LIGHT_BLOCK_X 8
#define FOG_LIGHT_BLOCK_Y 8
#define FOG_LIGHT_BLOCK_Z 4
#define FOG_POINT_LIGHT_CAPACITY 256
#define FOG_POINT_LIGHT_MASK_WORDS (FOG_POINT_LIGHT_CAPACITY / 32)

#endif
