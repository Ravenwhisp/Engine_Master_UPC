#pragma once

#include "Globals.h"
#include "EmitterRender.h"

class Texture;


struct ParticleCommand
{
    Vector3 position;
    Vector2 scale;
    float rotationZ;
    Vector4 colorAndAlpha;

    Vector2 sheetOffset; // offset for current frame animation (based on texture tile)
};

struct ParticleEmitterCommand
{
    Texture* texture = nullptr;
    int layer; // to indicate the order which particles between overlapped emitters will be drawn in

    EmitterRender::RenderMode renderMode = EmitterRender::RenderMode::BILLBOARD;

    Vector2 uvScale; // to determine size of a texture tile
    std::vector <ParticleCommand> particles;
};

struct shaderParticleData { // WARNING: align to 16 bytes! (add padding when required; if something doesn't fit in the alignment space, you will have to add it) <- XMFLOAT is probably unnecessary
    
    XMFLOAT4X4 worldPosition;
    XMFLOAT4 colorAndAlpha;
    XMFLOAT2 sheetOffset;
};

struct shaderEmissorData {

    UINT xTiles;
    UINT yTiles;
    UINT padding[2];
};