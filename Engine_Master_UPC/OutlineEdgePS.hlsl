Texture2D depthTexture : register(t0);
SamplerState pointClampSampler : register(s0);

cbuffer OutlineParams : register(b0)
{
	float4 outlineColor;
	float2 texelSize;
	float  minSeparation;
	float  maxSeparation;
	float  minDistance;
	float  maxDistance;
	int    searchSize;
	float  noiseScale;
};

float random(float2 uv)
{
	return frac(sin(dot(uv, float2(12.9898f, 78.233f))) * 43758.5453f);
}

struct PSInput
{
	float2 texcoord : TEXCOORD;
	float4 svPos : SV_Position;
};

float4 main(PSInput input) : SV_Target
{
	float2 fragCoord = input.svPos.xy;
	float2 uv = input.texcoord;

	float2 noise = float2(0.0f, 0.0f);
	if (noiseScale > 0.0f)
	{
		noise.x = random(floor(fragCoord * 0.1f));
		noise.y = random(floor(fragCoord * 0.1f + float2(1.0f, 1.0f)));
		noise = (noise * 2.0f - 1.0f) * noiseScale;
	}
	float2 noiseOffset = float2(noise.x * texelSize.x, noise.y * texelSize.y * 0.5f);

	float2 centerUV = uv + noiseOffset;
	float depth = depthTexture.Sample(pointClampSampler, centerUV).r;
	float separation = lerp(maxSeparation, minSeparation, depth);

	float mx = 0.0f;
	for (int i = -searchSize; i <= searchSize; ++i)
	{
		for (int j = -searchSize; j <= searchSize; ++j)
		{
			float2 sampleCoord = fragCoord + noiseOffset + float2((float)i, (float)j) * separation;
			float2 sampleUV = sampleCoord * texelSize;
			float sampleDepth = depthTexture.Sample(pointClampSampler, sampleUV).r;
			float diff = abs(depth - sampleDepth);
			mx = max(mx, diff);
		}
	}

	float edge = smoothstep(minDistance, maxDistance, mx);
	return float4(outlineColor.rgb, outlineColor.a * edge);
}
