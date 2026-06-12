#include "LightingCBuffers.hlsli"

struct VSOut
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};
 
VSOut main(uint vertexID : SV_VertexID)
{
    VSOut output;
    float2 positions[3] =
    {
        float2(-1, -1),
        float2(-1, 3),
        float2(3, -1)
    };
    float2 uvs[3] =
    {
        float2(0, 1),
        float2(0, -1),
        float2(2, 1)
    };
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = uvs[vertexID];
    return output;
}