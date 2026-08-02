Texture2D<float2> inputMinMax : register(t0);

RWStructuredBuffer<float4x4>
    outputLightViewProjection : register(u0);

cbuffer ShadowFrustumParams : register(b0)
{
    float4x4 inverseView;
    float4x4 cameraProjection;

    float3 lightDirection;
    float sunDistance;

    float minOrthoSize;
    float3 padding;
};

float LinearizeViewDepth(float deviceDepth)
{
    /*
     * Right-handed projection:
     *
     * deviceDepth =
     *     (viewZ * projection._33 + projection._43) /
     *     (-viewZ)
     *
     * El resultado devuelto es Z en view space, por tanto negativo
     * para geometría situada delante de la cámara.
     */
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

                /*
                 * Reconstrucción del punto en view space para una
                 * proyección perspectiva right-handed.
                 */
                float3 viewPoint;

                viewPoint.x =
                    ndcX * distanceValue / xScale;

                viewPoint.y =
                    ndcY * distanceValue / yScale;

                viewPoint.z = -distanceValue;

                float4 worldPoint =
                    mul(float4(viewPoint, 1.0f), inverseView);

                corners[cornerIndex] =
                    worldPoint.xyz / worldPoint.w;

                ++cornerIndex;
            }
        }
    }
}

float4 ComputeBoundingSphere(
    float3 corners[8])
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

[numthreads(1, 1, 1)]
void main()
{
    float2 minMaxDepth =
        inputMinMax.Load(int3(0, 0, 0));

    /*
     * min > max es el marcador producido por la reducción cuando
     * no se encontró geometría válida.
     *
     * Todavía no consumiremos este buffer en el ShadowMapPass.
     * El fallback definitivo se añadirá en el commit 6.
     */
    if (minMaxDepth.x > minMaxDepth.y)
    {
        outputLightViewProjection[0] =
            float4x4(
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);

        return;
    }

    float nearViewZ =
        LinearizeViewDepth(minMaxDepth.x);

    float farViewZ =
        LinearizeViewDepth(minMaxDepth.y);

    float nearDistance =
        max(-nearViewZ, 0.0001f);

    float farDistance =
        max(-farViewZ, nearDistance + 0.0001f);

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

    float3 up = float3(0.0f, 1.0f, 0.0f);

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
        max(sphere.w * 2.0f, minOrthoSize);

    float4x4 lightProjection =
        BuildOrthographicRH(
            orthoSize,
            orthoSize,
            0.0f,
            sphere.w * 2.0f + sunDistance);

    outputLightViewProjection[0] =
        mul(lightView, lightProjection);
}