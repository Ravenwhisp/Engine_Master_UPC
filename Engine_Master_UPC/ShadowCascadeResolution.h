#ifndef SHADOW_CASCADE_RESOLUTION_H
#define SHADOW_CASCADE_RESOLUTION_H
// Shared by C++ allocation and HLSL: N, N/2, N/4, N/8
#define SHADOW_CASCADE_DIVISOR(index) ((index) == 0 ? 1 : ((index) == 1 ? 2 : ((index) == 2 ? 4 : 8)))
#endif
