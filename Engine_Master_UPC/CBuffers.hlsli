cbuffer SceneData : register(b1)
{
	float3 lightDirection;
	float3 lightColor;
	float3 ambientColor;
	float3 view;
};

cbuffer ModelData : register(b2)
{
	float4x4 model;
	float4x4 normalMat;
    
	float3 diffuseColour;
    bool hasDiffuseTex;
    float3 specularColour;
	float shininess;

};