Texture2D skybox : register(t0);
SamplerState skyboxSampler1 : register(s0);
SamplerState skyboxSampler2 : register(s1);
SamplerState skyboxSampler3 : register(s2);
SamplerState skyboxSampler4 : register(s3);

static const float PI = 3.14159265f;

float2 CartesianToEquirectangular(in float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    phi = 1.0 - (phi / (2.0 * PI) + 0.5);

    float theta = asin(dir.y);
    theta = theta / PI + 0.5;

    return float2(1.0 - phi, 1.0 - theta);
}

float4 main(float3 coords : TEXCOORD) : SV_Target
{
    float3 dir = normalize(coords);
    float2 uv = CartesianToEquirectangular(dir);
    return float4(skybox.Sample(skyboxSampler3, uv).rgb, 1.0);
}