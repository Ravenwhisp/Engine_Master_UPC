static const float2 positions[3] =
{
	float2(-1.0, 1.0),
	float2(3.0, 1.0),
	float2(-1.0, -3.0)
};

static const float2 uvs[3] =
{
	float2(0.0, 0.0),
	float2(2.0, 0.0),
	float2(0.0, 2.0)
};

struct VSOut
{
	float2 texcoord : TEXCOORD;
	float4 position : SV_Position;
};

VSOut main(uint vertexID : SV_VertexID)
{
	VSOut output;
	output.texcoord = uvs[vertexID];
	output.position = float4(positions[vertexID], 0.0, 1.0);
	return output;
}
