cbuffer MediumConstants : register(b0)
{
    float density;
    float scatteringCoefficient;
    float extinctionCoefficient;
    float padding0;

    uint gridWidth;
    uint gridHeight;
    uint gridDepth;
    uint padding1;
};

RWTexture3D<float4> mediumVolume : register(u0);

[numthreads(8, 8, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (dispatchThreadID.x >= gridWidth || dispatchThreadID.y >= gridHeight || dispatchThreadID.z >= gridDepth)
        return;

    float effectiveScattering = density * scatteringCoefficient;
    float effectiveExtinction = density * extinctionCoefficient;

    mediumVolume[dispatchThreadID] = float4(effectiveScattering.xxx, effectiveExtinction);
}