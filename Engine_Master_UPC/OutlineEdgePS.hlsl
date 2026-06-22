Texture2D depthTexture : register(t0);
Texture2D colorTexture : register(t1);
Texture2D noiseTexture : register(t2);
SamplerState pointClampSampler : register(s0);
SamplerState linearClampSampler : register(s1);

cbuffer OutlineParams : register(b0)
{
	float4   colorModifier;
	float2   texelSize;
	float    minSeparation;
	float    maxSeparation;
	float    minDistance;
	float    maxDistance;
	int      searchSize;
	float    noiseScale;
	float4x4 invProjection;
};

float4 ViewPos(float2 uv, float d, float4x4 invProj)
{
	float4 ndc = float4(uv * 2.0f - 1.0f, d, 1.0f);
	float4 vp  = mul(ndc, invProj);
	return vp / vp.w;
}

struct PSInput
{
	float2 texcoord : TEXCOORD;
	float4 svPos    : SV_Position;
};

float4 main(PSInput input) : SV_Target
{
	float2 fragCoord = input.svPos.xy;
	float2 uv        = input.texcoord;

	float2 noise = float2(0.0f, 0.0f);
	if (noiseScale > 0.0f)
	{
		float2 noiseUV = fragCoord / 128.0f;
		float3 noiseSample = noiseTexture.Sample(linearClampSampler, noiseUV).rgb;
		noise = noiseSample.rb * 2.0f - 1.0f;
		noise *= noiseScale;
	}
	float2 noiseOffset = float2(noise.x * texelSize.x, noise.y * texelSize.y);

	float2 centerUV = uv + noiseOffset;
	float  depth    = depthTexture.Sample(pointClampSampler, centerUV).r;

	float3 centerView = ViewPos(centerUV, depth, invProjection).xyz;
	float  separation = lerp(maxSeparation, minSeparation, depth);

	float mx = 0.0f;
	for (int i = -searchSize; i <= searchSize; ++i)
	{
		for (int j = -searchSize; j <= searchSize; ++j)
		{
			float2 sampleCoord = fragCoord + noiseOffset + float2((float)i, (float)j) * separation;
			float2 sampleUV    = sampleCoord * texelSize;
			float  sampleDepth = depthTexture.Sample(pointClampSampler, sampleUV).r;
			float3 sampleView  = ViewPos(sampleUV, sampleDepth, invProjection).xyz;

			float dist = length(centerView - sampleView);
			mx = max(mx, dist);
		}
	}

	float edge = smoothstep(minDistance, maxDistance, mx);
	float3 sceneColor = colorTexture.Sample(linearClampSampler, centerUV).rgb;
	return float4(sceneColor * colorModifier.rgb, colorModifier.a * edge);
}
