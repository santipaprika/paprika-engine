cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix worldToView;
    matrix viewToWorld;
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
	float3 normal : NORMAL;
};

PixelShaderInput VSMain(VertexShaderInput input)
{
	PixelShaderInput output;

    matrix viewProj = mul(projection, worldToView);
    float3 pos = input.pos * 0.03;
    // float4 posWS = mul(float4(pos, 1.0), model);
    float4 posSS = mul(worldToView, float4(pos, 1.0));
    posSS = mul(projection, posSS);
    posSS /= posSS.w;
	// pos = mul(pos, model);
	// pos = mul(pos, view);
	// pos = mul(pos, projection);
    output.pos = posSS;
    output.normal = input.normal;
	output.color = input.color;

	return output;
}
