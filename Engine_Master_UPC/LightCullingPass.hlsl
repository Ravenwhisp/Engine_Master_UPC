// This is redefined here instead of reusing LightingCBuffers.hlsli because there's extra registers we don't need there and the layout changes
#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 112
#define MAX_SPOT_LIGHTS 16

#define MAX_LIGHTS_PER_TILE_PER_TYPE 32

struct DirectionalLight
{
    float3 direction;
    float pad0;
    float3 color;
    float intensity;
};

struct PointLight
{
    float3 position;
    float radius;
    float3 color;
    float intensity;
};

struct SpotLight
{
    float3 position;
    float radius;

    float3 direction;
    float pad0;

    float3 color;
    float intensity;

    float cosineInnerAngle;
    float cosineOuterAngle;
    float2 pad1;
    
    float4 boundingSphere;
};

cbuffer LightsCB : register(b0)
{
    float3 ambientColor;
    float ambientIntensity;

    uint directionalCount;
    uint pointCount;
    uint spotCount;
    uint paddingCounts;

    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS]; // could remove this to save a bit of bandwidth since these aren't culled, I just copy-pasted from LightingCBuffer.hlsli, but I'd rather not have 2 different CB structures
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
};

cbuffer LightCullingCB : register(b1)
{
    float4x4 view; // View Matrix
    float4x4 proj; // Projection Matrix
    float4x4 invProj; // Inverse Projection Matrix (needed to calculate depth in view space)
    uint2 screenSize;
    uint2 padding;
};

#define TILE_SIZE 8

groupshared uint minZ;
groupshared uint maxZ;
groupshared uint pointsInTile;
groupshared uint spotsInTile;

Texture2D depthTex : register(t0);
RWStructuredBuffer<int> lightIndexes : register(u0);

float getZInView(float z)
{
    // D3D12 NDC Z is [0, 1]
    float4 clipPos = float4(0.0f, 0.0f, z, 1.0f);

    float4 viewPos = mul(clipPos, invProj);
    viewPos /= viewPos.w;

    return viewPos.z;
}

float getDistanceFromPlane(float3 planeNormal, float3 pos)
{
    return dot(planeNormal, pos);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void main(uint2 globalIndex : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint localIndex : SV_GroupIndex)
{
    // 1. Calculate min and max depth for cell
    float depth;
    bool validPixel = globalIndex.x < screenSize.x && globalIndex.y < screenSize.y; // Only if pixel is inside screen (for resolutions that are not multiples of TILE_SIZE, some pixels will be outside of the screen)
    if (validPixel)
    {
        depth = depthTex.Load(int3(globalIndex, 0));
    }
    
    if (localIndex == 0) // only 1 thread initializes variables
    {
        minZ = 0xffffffff;
        maxZ = 0;
        pointsInTile = 0;
        spotsInTile = 0;
    }
    
    GroupMemoryBarrierWithGroupSync(); // wait so all threads have variables initialized
    if (validPixel)
    {
        InterlockedMin(minZ, asuint(depth));
        InterlockedMax(maxZ, asuint(depth));
    }
    GroupMemoryBarrierWithGroupSync(); // wait again so from now on minZ and maxZ are guaranteed to be correct
    
    
    // 2.   For each light, run frustum test to see if it intersects tile
    // 2.1. Calculate frustum planes for tile
    float3 planePoints[4];
    float2 tilePosition = groupID.xy;
    float width = screenSize.x;
    float height = screenSize.y;
    planePoints[0] = float2(tilePosition.x * TILE_SIZE / float(width), tilePosition.y * TILE_SIZE / float(height));
    planePoints[1] = float2((tilePosition.x + 1) * TILE_SIZE / float(width), tilePosition.y * TILE_SIZE / float(height));
    planePoints[2] = float2((tilePosition.x + 1) * TILE_SIZE / float(width), (tilePosition.y + 1) * TILE_SIZE / float(height));
    planePoints[3] = float2(tilePosition.x * TILE_SIZE / float(width), (tilePosition.y + 1) * TILE_SIZE / float(height));

    // 2.2. Convert to NDC
    for (uint i = 0; i < 4; i++)
    {
        planePoints[i].x = planePoints[i].x * 2.0 - 1.0;
        planePoints[i].y = (1.0 - planePoints[i].y) * 2.0 - 1.0;
    }
    
    // 2.3. Compute x, y, and z of frustum in view space. Formula from the Light Culling slides
    float a = proj._11;
    float b = proj._22;
    float c = proj._33;
    float d = proj._43;
    float zView = -d / c;
    for (uint i = 0; i < 4; i++)
    {
        planePoints[i].x = (planePoints[i].x * -zView) / a;
        planePoints[i].y = (planePoints[i].y * -zView) / b;
        planePoints[i].z = zView;
    }

    // 2.4. Compute the frustum planes (they will be in view space now)
    float3 planes[4];
    planes[0] = normalize(cross(planePoints[0], planePoints[1]));
    planes[1] = normalize(cross(planePoints[1], planePoints[2]));
    planes[2] = normalize(cross(planePoints[2], planePoints[3]));
    planes[3] = normalize(cross(planePoints[3], planePoints[0]));
    // We don't need to compute front and back planes since normal is just Z and -Z. However they do have a D which is near and far distance (depth)
    
    // 2.5. Compute the actual frustum test
    uint threadsInTile = TILE_SIZE * TILE_SIZE;
    float minZInView = getZInView(asfloat(minZ));
    float maxZInView = getZInView(asfloat(maxZ));
    
    
    uint tilesX = (screenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    uint tileIndex = groupID.y * tilesX + groupID.x;
    
    // 2.5.1. Point Lights
    for (uint i = localIndex; i < pointCount; i += threadsInTile)
    {
        float3 lightPosInView = mul(float4(pointLights[i].position, 1.0), view).xyz;
        float radius = pointLights[i].radius;
        
        if (getDistanceFromPlane(planes[0], lightPosInView) < radius &&
            getDistanceFromPlane(planes[1], lightPosInView) < radius &&
            getDistanceFromPlane(planes[2], lightPosInView) < radius &&
            getDistanceFromPlane(planes[3], lightPosInView) < radius &&
            (minZInView - lightPosInView.z) > -radius &&
            (maxZInView - lightPosInView.z) < radius)
        {
            // Passed, we need to add to the StructuredBuffer with the lights
            uint index;
            InterlockedAdd(pointsInTile, 1, index);
            if (pointsInTile < MAX_LIGHTS_PER_TILE_PER_TYPE)
            {
                uint tileBufferIndex = tileIndex * MAX_LIGHTS_PER_TILE_PER_TYPE * 2;
                lightIndexes[tileBufferIndex + index] = i;
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (localIndex == 0 && pointsInTile < MAX_LIGHTS_PER_TILE_PER_TYPE) // Mark last one with -1 if there are less than the max
    {
        uint lastIndex = tileIndex * MAX_LIGHTS_PER_TILE_PER_TYPE * 2 + pointsInTile;
        lightIndexes[lastIndex] = -1;
    }
    
    
    // 2.5.2. Spotlights
    for (uint i = localIndex; i < spotCount; i += threadsInTile)
    {
        float4 boundingSphere = spotLights[i].boundingSphere;
        float3 lightPos = float3(boundingSphere.x, boundingSphere.y, boundingSphere.z);
        float3 lightPosInView = mul(float4(lightPos, 1.0), view).xyz;
        float radius = boundingSphere.w;
        
        if (getDistanceFromPlane(planes[0], lightPosInView) < radius &&
            getDistanceFromPlane(planes[1], lightPosInView) < radius &&
            getDistanceFromPlane(planes[2], lightPosInView) < radius &&
            getDistanceFromPlane(planes[3], lightPosInView) < radius &&
            (minZInView - lightPosInView.z) > -radius &&
            (maxZInView - lightPosInView.z) < radius)
        {
            // Passed, we need to add to the StructuredBuffer with the lights
            uint index;
            InterlockedAdd(spotsInTile, 1, index);
            if (spotsInTile < MAX_LIGHTS_PER_TILE_PER_TYPE)
            {
                uint tileBufferIndex = tileIndex * MAX_LIGHTS_PER_TILE_PER_TYPE * 3;
                lightIndexes[tileBufferIndex + index] = i;
            }
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (localIndex == 0 && spotsInTile < MAX_LIGHTS_PER_TILE_PER_TYPE) // Mark last one with -1 if there are less than the max
    {
        uint lastIndex = tileIndex * MAX_LIGHTS_PER_TILE_PER_TYPE * 3 + spotsInTile;
        lightIndexes[lastIndex] = -1;
    }
}