#pragma once

#include "Globals.h"

class Texture;

struct ParticleEmitterCommand
{
    Texture* texture = nullptr;
    std::vector <ParticleCommand> particles;
};

struct ParticleCommand
{
    Vector3 position;
    Vector2 scale;
    float rotationZ;
    Vector4 colorAndAlpha;
};

struct shaderParticleData {
    
    Matrix worldPosition;
    Vector4 colorAndAlpha;
    // UINT frame = 0; <- to align as well
};

struct shaderEmissorData {

    UINT xTiles;
    UINT yTiles;
    UINT padding[2];
};