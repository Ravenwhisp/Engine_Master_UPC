struct VertexData
{
    float3 position;
    float2 texCoord0;
    float3 normal;
    uint2 packedJoints;
    float4 weights;
};

typedef row_major float4x4 RowMajorMatrix;

StructuredBuffer<VertexData> gInputVertices : register(t0);
RWStructuredBuffer<VertexData> gOutputVertices : register(u0);
StructuredBuffer<RowMajorMatrix> gPaletteModel : register(t1);
StructuredBuffer<RowMajorMatrix> gPaletteNormal : register(t2);

cbuffer SkinningParams : register(b0)
{
    uint gVertexCount;
};

uint GetJointIndex(uint2 packedJoints, uint jointSlot)
{
    uint packed = (jointSlot < 2) ? packedJoints.x : packedJoints.y;
    return (jointSlot & 1) == 0 ? (packed & 0xFFFFu) : (packed >> 16);
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint vertexIndex = dispatchThreadID.x;
    if (vertexIndex >= gVertexCount)
        return;

    VertexData inputVertex = gInputVertices[vertexIndex];

    float3 skinnedPosition = float3(0.0f, 0.0f, 0.0f);
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    [unroll]
    for (uint j = 0; j < 4; ++j)
    {
        uint jointIndex = GetJointIndex(inputVertex.packedJoints, j);
        float weight = inputVertex.weights[j];

        if (weight <= 0.0f)
            continue;

        float4 pos = mul(float4(inputVertex.position, 1.0f), gPaletteModel[jointIndex]);
        float3 nrm = mul(float4(inputVertex.normal, 0.0f), gPaletteNormal[jointIndex]).xyz;

        skinnedPosition += pos.xyz * weight;
        skinnedNormal += nrm * weight;
        totalWeight += weight;
    }

    VertexData outputVertex = inputVertex;

    if (totalWeight > 0.0f)
    {
        outputVertex.position = skinnedPosition;

        if (dot(skinnedNormal, skinnedNormal) > 0.0f)
        {
            outputVertex.normal = normalize(skinnedNormal);
        }
    }

    gOutputVertices[vertexIndex] = outputVertex;
}