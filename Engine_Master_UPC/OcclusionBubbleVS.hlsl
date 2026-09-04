struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    float2 position;

    position.x = vertexID == 2 ? 3.0f : -1.0f;
    position.y = vertexID == 1 ? 3.0f : -1.0f;

    output.position = float4(position, 0.0f, 1.0f);

    return output;
}