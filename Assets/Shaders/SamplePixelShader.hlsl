struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
    float3 normal : NORMAL;
};

cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix model;
    matrix view;
    matrix projection;
};

cbuffer CB0 : register(b0)
{
	float time : register(b0);
}

float4 PSMain(PixelShaderInput input) : SV_TARGET
{
    float3 lightPos = float3(10, 10, -10);
    float ndl = saturate(dot(lightPos, input.normal));
    return float4(input.normal * 0.5 + 0.5, 1.0f);
}
