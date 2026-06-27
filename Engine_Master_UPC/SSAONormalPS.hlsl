struct PixelInput
{
    float4 position : SV_POSITION;
    float3 viewNormal : NORMAL;
};

float4 main(PixelInput input) : SV_TARGET
{
    float3 normal = normalize(input.viewNormal);

    // Encode from [-1, 1] to [0, 1]
    return float4(normal * 0.5f + 0.5f, 1.0f);
}