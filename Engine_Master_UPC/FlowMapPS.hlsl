cbuffer ModelData : register(b2)
{
    float4x4 model;
    float4x4 normalMat;
    float3 diffuseColor;
    bool hasDiffuseTex;
    float metallicFactor;
    float roughnessFactor;
    bool hasMetallicRoughnessTex;
    float normalFactor;
    uint hasNormalTex;
    float3 emissiveColor;
    uint hasEmissiveTex;
    float3 modelPadding;
};

cbuffer FlowMapData : register(b6)
{
    float2 flowDirection;
    float2 flowTiling;
    float2 flowOffset;
    float flowStrength;
    uint flowSource;
    uint flowEnabled;
    uint flowTechnique;
    float flowPhase;
    float flowExaggeration;
    uint flowPaddingScalar;
    float2 flowPadding;
};

Texture2D diffuseTexture : register(t0);
Texture2D metallicRoughnessTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D emissiveTexture : register(t3);
Texture2D flowTexture : register(t13);

SamplerState linearWrapSampler : register(s0);

struct PSOutput
{
    float4 diffuse : SV_Target0;
    float4 metalRoughness : SV_Target1;
    float4 normal : SV_Target2;
    float4 position : SV_Target3;
    float4 emissive : SV_Target4;
};

float3 decodeNormal(float4 sampleValue, float normalScale)
{
    float3 tangentNormal = normalize(sampleValue.rgb * 2.0f - 1.0f);
    // Older material assets store an unspecified normal strength as zero.
    // Treat that value as the standard full-strength normal map.
    tangentNormal.xy *= normalScale > 0.0f ? normalScale : 1.0f;
    return normalize(tangentNormal);
}

float2 getFlowVector(float2 coord)
{
    if (flowSource == 1)
        return flowTexture.Sample(linearWrapSampler, coord).rg * 2.0f - 1.0f;

    return flowDirection;
}

PSOutput main(float4 position : SV_POSITION, float3 worldPos : TEXCOORD0,
              float2 coord : TEXCOORD1, float3 normal : TEXCOORD2,
              float3 tangent : TEXCOORD3)
{
    PSOutput output;
    const bool waterTechnique = flowTechnique == 1;
    const bool waterActive = waterTechnique && flowEnabled != 0;

    float2 flowVector = float2(0.0f, 0.0f);
    float phase0 = 0.0f;
    float phase1 = 0.0f;
    float blend = 0.0f;
    float strength = flowStrength * flowExaggeration;

    if (waterActive)
    {
        flowVector = getFlowVector(coord);
        phase0 = frac(flowPhase);
        phase1 = frac(flowPhase + 0.5f);
        // Each sample fades out when its phase wraps, hiding the UV jump.
        blend = abs(0.5f - phase0) / 0.5f;
    }

    // In water mode every material texture is advected along the flow vector
    // using two phase-shifted samples that cross-fade to avoid popping.
    float2 matUV0 = coord + (waterActive ? flowVector * phase0 * strength : 0.0f);
    float2 matUV1 = coord + (waterActive ? flowVector * phase1 * strength : 0.0f);

    float3 albedo = diffuseColor;
    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    float ao = 1.0f;
    float3 finalNormal = normalize(normal);
    float3 emissive = 0.0f;

    if (hasDiffuseTex != 0)
    {
        float4 s0 = diffuseTexture.Sample(linearWrapSampler, matUV0);
        float4 s1 = diffuseTexture.Sample(linearWrapSampler, matUV1);
        float4 sampleColor = waterActive ? lerp(s0, s1, blend) : s0;
        if (sampleColor.a < 0.5f)
            discard;
        albedo *= sampleColor.rgb;
    }

    if (hasMetallicRoughnessTex != 0)
    {
        float4 m0 = metallicRoughnessTexture.Sample(linearWrapSampler, matUV0);
        float4 m1 = metallicRoughnessTexture.Sample(linearWrapSampler, matUV1);
        float4 sampleM = waterActive ? lerp(m0, m1, blend) : m0;
        ao = sampleM.r;
        roughness = 1.0f - clamp(sampleM.g, 0.04f, 1.0f);
        metallic = saturate(sampleM.b * metallicFactor);
    }

    if (waterActive)
    {
        float3 tangentNormal;
        if (hasNormalTex != 0)
        {
            float3 normal0 = decodeNormal(
                normalTexture.Sample(linearWrapSampler, matUV0),
                normalFactor);
            float3 normal1 = decodeNormal(
                normalTexture.Sample(linearWrapSampler, matUV1),
                normalFactor);
            tangentNormal = normalize(lerp(normal0, normal1, blend));
        }
        else
        {
            // No material normal map (for example a flat purple normal), so
            // synthesize animated ripple detail directly from the flow vector.
            // This makes the flow effect visible even on a static normal.
            float wave0 = sin((coord.x * flowVector.x + coord.y * flowVector.y) * 40.0f + phase0 * 6.2831853f);
            float wave1 = sin((coord.x * flowVector.x + coord.y * flowVector.y) * 40.0f + phase1 * 6.2831853f);
            float wave = lerp(wave0, wave1, blend);

            float3 ripple = float3(flowVector.y, -flowVector.x, 0.0f) * wave;
            ripple *= strength;
            tangentNormal = normalize(float3(ripple.x, ripple.y, 1.0f));
        }

        float3 tangentVector = normalize(tangent);
        float3 bitangentVector = normalize(cross(finalNormal, tangentVector));
        finalNormal = normalize(mul(tangentNormal,
            float3x3(tangentVector, bitangentVector, finalNormal)));
    }
    else if (hasNormalTex != 0)
    {
        float3 tangentNormal = decodeNormal(
            normalTexture.Sample(linearWrapSampler, coord), normalFactor);
        float3 tangentVector = normalize(tangent);
        float3 bitangentVector = normalize(cross(finalNormal, tangentVector));
        finalNormal = normalize(mul(tangentNormal,
            float3x3(tangentVector, bitangentVector, finalNormal)));
    }

    if (hasEmissiveTex != 0)
    {
        float4 e0 = emissiveTexture.Sample(linearWrapSampler, matUV0);
        float4 e1 = emissiveTexture.Sample(linearWrapSampler, matUV1);
        float4 sampleE = waterActive ? lerp(e0, e1, blend) : e0;
        emissive = sampleE.rgb * emissiveColor;
    }

    output.diffuse = float4(albedo, 1.0f);
    output.metalRoughness = float4(ao, roughness, metallic, 0.0f);
    output.normal = float4(finalNormal, 0.0f);
    output.position = float4(worldPos, 0.0f);
    output.emissive = float4(emissive, 0.0f);
    return output;
}
