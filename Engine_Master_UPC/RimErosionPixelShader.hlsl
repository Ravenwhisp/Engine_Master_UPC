#include "NewCBuffers.hlsli"

Texture2D baseColorTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D emissiveTex : register(t3);

TextureCube irradianceTexture : register(t8);
TextureCube environmentTexture : register(t9);
Texture2D brdfTexture : register(t10);
Texture2D shadowMap : register(t11);
Texture2D brushTexture : register(t12);

SamplerState linearWrapSample : register(s0);
SamplerState pointWrapSample : register(s1);
SamplerState linearClampSample : register(s2);
SamplerState pointClampSample : register(s3);

cbuffer ErosionData : register(b5)
{
    float displacementAmount;
    float rimThreshold;
    float rimSoftness;
    float erosionIntensity;
    float brushScale;
    float brushOffsetX;
    float brushOffsetY;
    float preserveSilhouette;
    float4 erosionColor;
    float debugRimMask;
    float3 eroPad;
    float4 paintColor1;
    float4 paintColor2;
    float brushNormalStrength;
    float curvatureScale;
    float toonSharpness;
    float pad3;
};

float3 BlendNormals(float3 baseNormal, float3 detailNormal)
{
    // Reoriented Normal Mapping — robust version
    float3 t1 = float3(0, 1, 0);
    float3 u = normalize(cross(t1, baseNormal));
    if (length(u) < 0.001)
        u = float3(1, 0, 0);
    float3 v = cross(baseNormal, u);
    return normalize(detailNormal.x * u + detailNormal.y * v + detailNormal.z * baseNormal);
}

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float3 tangent : TANGENT,
            float2 coord : TEXCOORD, float2 brushUV : BRUSHUV, float rimFactor : RIMFACTOR) : SV_TARGET
{
    // ===== Build surface normal =====
    float3 surfaceNormal = normalize(normal);
    if (hasNormalTex != 0)
    {
        float3 tangentNormal = normalTex.Sample(linearWrapSample, coord).rgb;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);
        float3 tangentVector = normalize(tangent.xyz);
        float3 bitangentVector = cross(surfaceNormal, tangentVector);
        float3x3 TBN = float3x3(tangentVector, bitangentVector, surfaceNormal);
        surfaceNormal = normalize(mul(tangentNormal, TBN));
    }

    // ===== Sample brush texture =====
    float4 brushObj = brushTexture.Sample(linearWrapSample, brushUV);
    
    // Camera-relative UVs for erosion mask (from article)
    float2 cameraUV = (worldPos.xy - viewPos.xy) * brushScale;
    float erosionTex = brushTexture.Sample(linearWrapSample, cameraUV).a * 0.2;

    // ===== Build brush normal from RG channels =====
    float2 brushN2D = brushObj.rg * 2.0 - 1.0;
    float3 brushNormal = normalize(float3(brushN2D, 0.95));
    float3 blendedNormal = BlendNormals(surfaceNormal, brushNormal);
    blendedNormal = normalize(lerp(surfaceNormal, blendedNormal, brushNormalStrength));

    // ===== Paint color from B channel =====
    float3 paintColor = lerp(paintColor1.rgb, paintColor2.rgb, brushObj.b);

    float3 albedo = paintColor;
    if (hasBaseColorTex != 0)
    {
        float4 baseSample = baseColorTex.Sample(linearWrapSample, coord);
        if (baseSample.a < 0.5f)
            discard;
        albedo = lerp(baseSample.rgb, paintColor, 0.7);
    }

    // ===== Toon directional lighting =====
    float3 viewDir = normalize(viewPos - worldPos);
    float3 lightDir = normalize(directionalLights[0].direction);
    float NdotL = dot(blendedNormal, lightDir);
    float toonLight = NdotL > 0.6 ? 1.0 : (NdotL > 0.3 ? 0.4 : 0.15);
    float3 toonDiffuse = albedo * toonLight * directionalLights[0].color * directionalLights[0].intensity;

    // Additional directional lights (flat)
    float3 extraDirect = 0.0;
    for (uint i = 1; i < directionalCount; ++i)
    {
        float ndl = dot(blendedNormal, normalize(directionalLights[i].direction));
        extraDirect += albedo * saturate(ndl) * directionalLights[i].color * directionalLights[i].intensity * 0.5;
    }

    // ===== Specular reflection (liquid paint effect from article) =====
    float3 reflDir = reflect(-viewDir, blendedNormal);
    float reflDotL = dot(reflDir, lightDir);
    float2 specUV = float2(brushUV.y, brushUV.x);
    float3 reflBrush = brushTexture.Sample(linearWrapSample, specUV).rgb;
    float reflStep = saturate(reflDotL * 3.0);
    float3 paintReflection = lerp(paintColor1.rgb, paintColor2.rgb, reflBrush.r) * reflStep * 0.3;

    // ===== Ambient =====
    float3 ambient = albedo * ambientColor * ambientIntensity * 0.5;

    // ===== Emissive =====
    float3 emissive = 0;
    if (hasEmissiveTex != 0)
    {
        float3 emissiveSample = emissiveTex.Sample(linearWrapSample, coord);
        emissive = emissiveSample.rgb * emissiveColor;
    }

    // ===== Combine lighting =====
    float3 finalColor = toonDiffuse + extraDirect + ambient + paintReflection + emissive;

    // ===== Rim erosion mask =====
    float NdotV = abs(dot(blendedNormal, viewDir)) + 0.001;
    float fresnel = 1.0 - NdotV;

    // Curvature filter — exclude flat faces from erosion
    float curvature = abs(ddx(blendedNormal.x)) + abs(ddy(blendedNormal.x))
                    + abs(ddx(blendedNormal.y)) + abs(ddy(blendedNormal.y));
    float notFlat = saturate(curvature * curvatureScale);
    fresnel = max(fresnel, notFlat * 0.1);

    // Step erosion texture against fresnel
    float erosionMask = step(erosionTex, fresnel);

    // ===== Debug mode =====
    if (debugRimMask > 0.5f)
    {
        return float4(fresnel, 0.0, 0.0, 1.0);
    }

    // ===== Final color with erosion =====
    float3 erodedColor = finalColor * erosionColor.rgb;
    float3 resultColor = lerp(finalColor, erodedColor, erosionMask);

    return float4(saturate(resultColor), 1.0);
}
