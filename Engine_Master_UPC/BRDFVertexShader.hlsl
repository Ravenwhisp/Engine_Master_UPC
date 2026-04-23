// Full-screen triangle vertex positions in clip space (-1 to 1 range)
static const float2 positions[3] =
{
    float2(-1.0, 1.0), // Top-left
    float2(3.0, 1.0), // Top-right,
    float2(-1.0, -3.0) // Bottom-left,
};

// Texture coordinates for the full-screen triangle
static const float2 uvs[3] =
{
    float2(0.0, 0.0), // Top-left UV
    float2(2.0, 0.0), // Right-extended UV
    float2(0.0, 2.0) // Bottom-extended UV
};

void main(uint vertexID : SV_VertexID, out float2 texcoord : TEXCOORD, out float4 position : SV_Position)
{
    texcoord = uvs[vertexID];
    position = float4(positions[vertexID], 0.0, 1.0);
}