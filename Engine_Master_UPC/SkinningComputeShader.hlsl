struct VertexInput
{
    float3 position;
    float2 texCoord0;
    float3 normal;
    uint4 joints;
    float4 weights;
};

struct VertexOutput
{
    float3 position;
    float2 texCoord0;
    float3 normal;
    uint4 joints;
    float4 weights;
};

StructuredBuffer<VertexInput> gInputVertices : register(t0);
RWStructuredBuffer<VertexOutput> gOutputVertices : register(u0);
StructuredBuffer<float4x4> gPaletteModel : register(t1);
StructuredBuffer<float4x4> gPaletteNormal : register(t2);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint vertexIndex = dispatchThreadID.x;

    VertexInput inputVertex = gInputVertices[vertexIndex];

    VertexOutput outputVertex;
    outputVertex.position = inputVertex.position;
    outputVertex.texCoord0 = inputVertex.texCoord0;
    outputVertex.normal = inputVertex.normal;
    outputVertex.joints = inputVertex.joints;
    outputVertex.weights = inputVertex.weights;

    gOutputVertices[vertexIndex] = outputVertex;
}