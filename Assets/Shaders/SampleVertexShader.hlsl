cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix model;
    matrix view;
    matrix projection;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
	float4 color : COLOR0;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
};

PixelShaderInput VSMain(VertexShaderInput input)
{
	PixelShaderInput output;

    matrix viewProj = mul(view, projection);
    float3 pos = input.pos * 0.1;
    float4 posWS = mul(float4(pos, 1.0), model);
    float4 posSS = mul(posWS, viewProj);
    posSS /= posSS.w;
	// pos = mul(pos, model);
	// pos = mul(pos, view);
	// pos = mul(pos, projection);
    output.pos = float4(pos, 1.0);

	output.color = input.color;

	return output;
}
