struct GradientColor
{
    float4 color;
    
    float percentage;
    float3 padding;
};

cbuffer GradientConstantBuffer : register(b1)
{
    GradientColor colors[10];
};

struct PSInput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float percentage : PERCENTAGE;
};

float4 main(PSInput input) : SV_TARGET
{
    
    GradientColor previousMark;
    GradientColor nextMark;
    
    if (input.percentage == 0)
    {
        nextMark = colors[1];
        previousMark = colors[0];
    }
    else
    {
        for (int i = 0; i < 10; ++i)
        {
            if (colors[i].percentage >= input.percentage && colors[i].percentage != 0)
            {
                nextMark = colors[i];
                previousMark = colors[i - 1];
                break;
            }
       
        }
    }
    
    float distance = nextMark.percentage - previousMark.percentage;
    float distancePixel = input.percentage - previousMark.percentage;
    
    float percentage = distancePixel / distance;
    
    float4 result = lerp(previousMark.color, nextMark.color, percentage);
    return result;
}