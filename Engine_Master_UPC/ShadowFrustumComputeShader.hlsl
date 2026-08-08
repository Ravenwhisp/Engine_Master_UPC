Texture2D<float2> inputMinMax : register(t0);

#define MAX_SHADOW_CASCADES 4
#define CASCADE_FIT_TO_SCENE 0
#define CASCADE_FIT_TO_CASCADE 1

struct ShadowDataOutput
{
    // Full fitted frustum used by the existing shadow pipeline.
    float4x4 lightViewProjection;

    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    float paddingShadow;

    float2 shadowMapTexelSize;
    uint pcfEnabled;
    uint pcfRadius;

    // CSM
    uint cascadeCount;
    uint cascadeFitMode;
    float2 cascadePadding;

    float4 cascadeFarDistances;

    float4x4 cascadeLightViewProjection[MAX_SHADOW_CASCADES];
};

RWStructuredBuffer<ShadowDataOutput>
    outputShadowData : register(u0);

cbuffer ShadowFrustumParams : register(b0)
{
    float4x4 inverseView;
    float4x4 cameraProjection;

    float3 lightDirection;
    float sunDistance;

    float minOrthoSize;
    float3 padding;

    float shadowBias;
    float shadowStrength;
    uint shadowsEnabled;
    uint pcfEnabled;

    uint pcfRadius;
    float shadowMapTexelSizeX;
    float shadowMapTexelSizeY;
    float paddingSettings;
    
    uint cascadeCount;
    uint cascadeFitMode;
    float cascadeSplit0;
    float cascadeSplit1;

    float cascadeSplit2;
    float3 cascadePadding;
};

float LinearizeViewDepth(float deviceDepth)
{
    float denominator =
        deviceDepth + cameraProjection._33;

    if (abs(denominator) < 0.000001f)
    {
        denominator =
            denominator < 0.0f
            ? -0.000001f
            : 0.000001f;
    }

    return -cameraProjection._43 / denominator;
}

void BuildFrustumCorners(
    float nearDistance,
    float farDistance,
    out float3 corners[8])
{
    const float xScale = cameraProjection._11;
    const float yScale = cameraProjection._22;

    uint cornerIndex = 0;

    [unroll]
    for (uint depthIndex = 0; depthIndex < 2; ++depthIndex)
    {
        const float distanceValue =
            depthIndex == 0
            ? nearDistance
            : farDistance;

        [unroll]
        for (uint yIndex = 0; yIndex < 2; ++yIndex)
        {
            const float ndcY =
                yIndex == 0 ? -1.0f : 1.0f;

            [unroll]
            for (uint xIndex = 0; xIndex < 2; ++xIndex)
            {
                const float ndcX =
                    xIndex == 0 ? -1.0f : 1.0f;

                float3 viewPoint;

                viewPoint.x =
                    ndcX * distanceValue / xScale;

                viewPoint.y =
                    ndcY * distanceValue / yScale;

                viewPoint.z = -distanceValue;

                float4 worldPoint =
                    mul(
                        float4(viewPoint, 1.0f),
                        inverseView);

                corners[cornerIndex] =
                    worldPoint.xyz / worldPoint.w;

                ++cornerIndex;
            }
        }
    }
}

float4 ComputeBoundingSphere(float3 corners[8])
{
    float3 center = 0.0f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        center += corners[i];
    }

    center /= 8.0f;

    float radius = 0.0f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        radius = max(
            radius,
            distance(center, corners[i]));
    }

    radius = max(
        radius,
        minOrthoSize * 0.5f);

    return float4(center, radius);
}

float4x4 BuildLookAtRH(
    float3 eye,
    float3 target,
    float3 up)
{
    float3 zAxis = normalize(eye - target);
    float3 xAxis = normalize(cross(up, zAxis));
    float3 yAxis = cross(zAxis, xAxis);

    return float4x4(
        xAxis.x, yAxis.x, zAxis.x, 0.0f,
        xAxis.y, yAxis.y, zAxis.y, 0.0f,
        xAxis.z, yAxis.z, zAxis.z, 0.0f,
        -dot(xAxis, eye),
        -dot(yAxis, eye),
        -dot(zAxis, eye),
        1.0f);
}

float4x4 BuildOrthographicRH(
    float width,
    float height,
    float nearPlane,
    float farPlane)
{
    const float inverseDepthRange =
        1.0f / (nearPlane - farPlane);

    return float4x4(
        2.0f / width, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / height, 0.0f, 0.0f,
        0.0f, 0.0f, inverseDepthRange, 0.0f,
        0.0f, 0.0f,
        nearPlane * inverseDepthRange,
        1.0f);
}

float4x4 BuildIdentityMatrix()
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
}

float4x4 BuildLightViewProjection(
    float nearDistance,
    float farDistance)
{
    float3 corners[8];

    BuildFrustumCorners(
        nearDistance,
        farDistance,
        corners);

    float4 sphere =
        ComputeBoundingSphere(corners);

    float3 normalizedLightDirection =
        normalize(lightDirection);

    float3 eye =
        sphere.xyz -
        normalizedLightDirection *
        (sphere.w + sunDistance);

    float3 up =
        float3(0.0f, 1.0f, 0.0f);

    if (abs(normalizedLightDirection.y) > 0.95f)
    {
        up = float3(0.0f, 0.0f, 1.0f);
    }

    float4x4 lightView =
        BuildLookAtRH(
            eye,
            sphere.xyz,
            up);

    float orthoSize =
        max(
            sphere.w * 2.0f,
            minOrthoSize);

    float4x4 lightProjection =
        BuildOrthographicRH(
            orthoSize,
            orthoSize,
            0.0f,
            sphere.w * 2.0f + sunDistance);

    return mul(
        lightView,
        lightProjection);
}

ShadowDataOutput BuildShadowOutput(
    float4x4 lightViewProjection,
    uint enabled)
{
    ShadowDataOutput output;

    output.lightViewProjection =
        lightViewProjection;

    output.shadowBias =
        shadowBias;

    output.shadowStrength =
        shadowStrength;

    output.shadowsEnabled =
        enabled;

    output.paddingShadow =
        0.0f;

    output.shadowMapTexelSize =
        float2(
            shadowMapTexelSizeX,
            shadowMapTexelSizeY);

    output.pcfEnabled =
        pcfEnabled;

    output.pcfRadius =
        pcfRadius;

    output.cascadeCount =
        clamp(
            cascadeCount,
            1u,
            (uint) MAX_SHADOW_CASCADES);

    output.cascadeFitMode =
        cascadeFitMode;

    output.cascadePadding =
        float2(0.0f, 0.0f);

    output.cascadeFarDistances =
        float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4x4 identityMatrix =
    BuildIdentityMatrix();

    output.cascadeLightViewProjection[0] =
    identityMatrix;

    output.cascadeLightViewProjection[1] =
    identityMatrix;

    output.cascadeLightViewProjection[2] =
    identityMatrix;

    output.cascadeLightViewProjection[3] =
    identityMatrix;

    return output;
}

[numthreads(1, 1, 1)]
void main()
{
    float2 minMaxDepth =
        inputMinMax.Load(int3(0, 0, 0));

    if (minMaxDepth.x > minMaxDepth.y)
    {
        float4x4 identityMatrix =
            BuildIdentityMatrix();

        outputShadowData[0] =
            BuildShadowOutput(
                identityMatrix,
                0u);

        return;
    }

    float nearViewZ =
        LinearizeViewDepth(
            minMaxDepth.x);

    float farViewZ =
        LinearizeViewDepth(
            minMaxDepth.y);

    float nearDistance =
        max(
            -nearViewZ,
            0.0001f);

    float farDistance =
        max(
            -farViewZ,
            nearDistance + 0.0001f);

    // Preserve the current full fitted shadow frustum.
    float4x4 fullLightViewProjection =
        BuildLightViewProjection(
            nearDistance,
            farDistance);

    ShadowDataOutput output =
        BuildShadowOutput(
            fullLightViewProjection,
            shadowsEnabled);

    uint activeCascadeCount =
        output.cascadeCount;

    float fittedDepthRange =
        farDistance - nearDistance;


    // Cascade 0
    float cascade0FarFraction =
        activeCascadeCount > 1
        ? cascadeSplit0
        : 1.0f;

    float cascade0NearDistance =
        nearDistance;

    float cascade0FarDistance =
        nearDistance +
        fittedDepthRange *
        cascade0FarFraction;

    cascade0FarDistance =
        max(
            cascade0FarDistance,
            cascade0NearDistance + 0.0001f);

    output.cascadeFarDistances.x =
        cascade0FarDistance;

    output.cascadeLightViewProjection[0] =
        BuildLightViewProjection(
            cascade0NearDistance,
            cascade0FarDistance);


    // Cascade 1
    if (activeCascadeCount > 1)
    {
        float cascade1FarFraction =
            activeCascadeCount > 2
            ? cascadeSplit1
            : 1.0f;

        float cascade1NearDistance =
            output.cascadeFitMode ==
                CASCADE_FIT_TO_CASCADE
            ? cascade0FarDistance
            : nearDistance;

        float cascade1FarDistance =
            nearDistance +
            fittedDepthRange *
            cascade1FarFraction;

        cascade1FarDistance =
            max(
                cascade1FarDistance,
                cascade1NearDistance + 0.0001f);

        output.cascadeFarDistances.y =
            cascade1FarDistance;

        output.cascadeLightViewProjection[1] =
            BuildLightViewProjection(
                cascade1NearDistance,
                cascade1FarDistance);


        // Cascade 2
        if (activeCascadeCount > 2)
        {
            float cascade2FarFraction =
                activeCascadeCount > 3
                ? cascadeSplit2
                : 1.0f;

            float cascade2NearDistance =
                output.cascadeFitMode ==
                    CASCADE_FIT_TO_CASCADE
                ? cascade1FarDistance
                : nearDistance;

            float cascade2FarDistance =
                nearDistance +
                fittedDepthRange *
                cascade2FarFraction;

            cascade2FarDistance =
                max(
                    cascade2FarDistance,
                    cascade2NearDistance + 0.0001f);

            output.cascadeFarDistances.z =
                cascade2FarDistance;

            output.cascadeLightViewProjection[2] =
                BuildLightViewProjection(
                    cascade2NearDistance,
                    cascade2FarDistance);


            // Cascade 3
            if (activeCascadeCount > 3)
            {
                float cascade3NearDistance =
                    output.cascadeFitMode ==
                        CASCADE_FIT_TO_CASCADE
                    ? cascade2FarDistance
                    : nearDistance;

                float cascade3FarDistance =
                    farDistance;

                cascade3FarDistance =
                    max(
                        cascade3FarDistance,
                        cascade3NearDistance + 0.0001f);

                output.cascadeFarDistances.w =
                    cascade3FarDistance;

                output.cascadeLightViewProjection[3] =
                    BuildLightViewProjection(
                        cascade3NearDistance,
                        cascade3FarDistance);
            }
        }
    }

    outputShadowData[0] =
        output;
}