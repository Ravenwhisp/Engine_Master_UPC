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
    uint32_t layer; // to indicate the order which particles between overlapped emitters will be drawn in

    EmitterRender::RenderMode renderMode = EmitterRender::RenderMode::BILLBOARD;
    Vector2 uvScale; // to determine size of a texture tile

    Vector3 HDRColorAndIntensity; // HDR color + intensity for bloom post-processing

    std::vector <ParticleCommand> particles;

    EmitterRender::BlendMode blendMode = EmitterRender::BlendMode::ALPHA;
};

struct ShaderParticleData { // for Structured buffet, so 16 bytes alignment not necessary <- XMFLOAT is probably unnecessary
    
    XMFLOAT4X4 worldPosition;
    XMFLOAT4 colorAndAlpha;
    XMFLOAT2 sheetOffset;
};

struct ShaderEmissorData { // passed in the root signature => 16 bytes alignment necessary

	Vector3 hdrColorAndIntensity;
	float padding; // to align to 16 bytes
	Vector2 uvScale;
	float padding2[2]; // to align to 16 bytes
};

struct ShaderAllEmissorsData { // doesn't need to be aligned, because goes to ring buffer which is aligned to 256 bytes, and the padding (at the end) is done in the ring buffer allocation
    XMFLOAT4X4 viewProjection;
    Vector2 depthRange;
};