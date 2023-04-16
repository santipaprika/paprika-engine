struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

cbuffer CB0 : register(b0)
{
	float time;
}

float4 PSMain(PixelShaderInput input) : SV_TARGET
{
	return float4(input.color * time / 50.0, 1.0f);
}
