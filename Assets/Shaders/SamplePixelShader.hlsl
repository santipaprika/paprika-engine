struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
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
	return float4(input.color * time * model._m03_m13_m23 / 50.0, 1.0f);
}
