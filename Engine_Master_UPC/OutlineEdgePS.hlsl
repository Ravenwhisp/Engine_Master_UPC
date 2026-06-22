Texture2D depthTexture : register(t0);
SamplerState pointClampSampler : register(s0);

cbuffer OutlineParams : register(b0)
{
	float4 outlineColor;
	float2 texelSize;
	float threshold;
	float padding;
};

float4 main(float2 texcoord : TEXCOORD) : SV_Target
{
	float d0 = depthTexture.Sample(pointClampSampler, texcoord + float2(-texelSize.x, -texelSize.y)).r;
	float d1 = depthTexture.Sample(pointClampSampler, texcoord + float2(0.0, -texelSize.y)).r;
	float d2 = depthTexture.Sample(pointClampSampler, texcoord + float2(texelSize.x, -texelSize.y)).r;
	float d3 = depthTexture.Sample(pointClampSampler, texcoord + float2(-texelSize.x, 0.0)).r;
	float d5 = depthTexture.Sample(pointClampSampler, texcoord + float2(texelSize.x, 0.0)).r;
	float d6 = depthTexture.Sample(pointClampSampler, texcoord + float2(-texelSize.x, texelSize.y)).r;
	float d7 = depthTexture.Sample(pointClampSampler, texcoord + float2(0.0, texelSize.y)).r;
	float d8 = depthTexture.Sample(pointClampSampler, texcoord + float2(texelSize.x, texelSize.y)).r;

	float gx = -d0 - 2.0 * d3 - d6 + d2 + 2.0 * d5 + d8;
	float gy = -d0 - 2.0 * d1 - d2 + d6 + 2.0 * d7 + d8;

	float magnitude = sqrt(gx * gx + gy * gy);

	if (magnitude > threshold)
	{
		return outlineColor;
	}

	return float4(0.0, 0.0, 0.0, 0.0);
}
