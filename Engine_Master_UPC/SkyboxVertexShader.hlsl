cbuffer SkyboxParams : register(b0)
{
    float4x4 vp;
    uint flipX;
    uint flipZ;
};

struct VSOut
{
    float3 direction : TEXCOORD0;
    float4 position : SV_POSITION;
};

VSOut main(float3 position : POSITION)
{
    VSOut output;

    if (flipX != 0)
        position.x = -position.x;
    if (flipZ != 0)
        position.z = -position.z;

    output.direction = position;

    float4 clip = mul(float4(position, 1.0f), vp);
    output.position = float4(clip.x, clip.y, clip.w, clip.w);

    return output;
}