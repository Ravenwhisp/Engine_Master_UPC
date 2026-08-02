Texture2D<float> inputDepth : register(t0);
RWTexture2D<float2> outputMinMax : register(u0);

cbuffer ReductionParams : register(b0)
{
    uint2 inputSize;
    uint2 padding;
};

groupshared uint groupMinDepth;
groupshared uint groupMaxDepth;
groupshared uint groupValidCount;

[numthreads(8, 8, 1)]
void main(
    uint3 globalIndex : SV_DispatchThreadID,
    uint3 localIndex3D : SV_GroupThreadID,
    uint3 groupIndex : SV_GroupID)
{
    uint localIndex =
        localIndex3D.y * 8 + localIndex3D.x;

    if (localIndex == 0)
    {
        groupMinDepth = asuint(1.0f);
        groupMaxDepth = asuint(0.0f);
        groupValidCount = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if (globalIndex.x < inputSize.x &&
        globalIndex.y < inputSize.y)
    {
        float depth =
            inputDepth.Load(int3(globalIndex.xy, 0));

        // The camera depth buffer is cleared to 1.0.
        // Clear pixels do not represent visible geometry.
        if (depth >= 0.0f && depth < 1.0f)
        {
            uint depthBits = asuint(depth);

            InterlockedMin(
                groupMinDepth,
                depthBits);

            InterlockedMax(
                groupMaxDepth,
                depthBits);

            InterlockedAdd(
                groupValidCount,
                1);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (localIndex == 0)
    {
        if (groupValidCount > 0)
        {
            outputMinMax[groupIndex.xy] =
                float2(
                    asfloat(groupMinDepth),
                    asfloat(groupMaxDepth));
        }
        else
        {
            // min > max marks a tile without geometry.
            outputMinMax[groupIndex.xy] =
                float2(1.0f, 0.0f);
        }
    }
}