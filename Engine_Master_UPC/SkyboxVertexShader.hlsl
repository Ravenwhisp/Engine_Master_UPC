cbuffer SkyboxParams : register(b0)
{
    float4x4 vp;
    uint flipX;
    uint flipZ;
    uint padding[2];
};

struct VSOut
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VSOut main(float3 position : POSITION)
{
    VSOut output;
    output.texCoord = position;
    
    float4 clip = mul(float4(position, 1.0f), vp);
    output.position = clip.xyww;

    if (flipX != 0)
    {
        output.texCoord.x = -output.texCoord.x;
    }
    if (flipZ != 0)
    {
        output.texCoord.z = -output.texCoord.z;
    }

    return output;
}