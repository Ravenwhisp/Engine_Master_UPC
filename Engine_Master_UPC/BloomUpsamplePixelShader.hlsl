Texture2D    inputTexture  : register(t0);
SamplerState bilinearClamp : register(s0);

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    float2 texSize;
    inputTexture.GetDimensions(texSize.x, texSize.y);
    float2 halfPixel = 0.5 / texSize;

    float4 sum = inputTexture.Sample(bilinearClamp, uv + float2(-halfPixel.x * 2.0, 0.0));
    sum += inputTexture.Sample(bilinearClamp, uv + float2(-halfPixel.x,  halfPixel.y)) * 2.0;
    sum += inputTexture.Sample(bilinearClamp, uv + float2(0.0,  halfPixel.y * 2.0));
    sum += inputTexture.Sample(bilinearClamp, uv + float2( halfPixel.x,  halfPixel.y)) * 2.0;
    sum += inputTexture.Sample(bilinearClamp, uv + float2( halfPixel.x * 2.0, 0.0));
    sum += inputTexture.Sample(bilinearClamp, uv + float2( halfPixel.x, -halfPixel.y)) * 2.0;
    sum += inputTexture.Sample(bilinearClamp, uv + float2(0.0, -halfPixel.y * 2.0));
    sum += inputTexture.Sample(bilinearClamp, uv + float2(-halfPixel.x, -halfPixel.y)) * 2.0;

    return sum / 12.0;
}
