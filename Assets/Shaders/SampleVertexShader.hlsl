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
    float3 pos = input.pos;// * 0.03;
    // float4 posWS = mul(model, float4(pos, 1.0));
    float4 posSS = mul(viewProj, float4(pos, 1.0));

    const float depth = posSS.w;
	posSS /= posSS.w;

     // If vertex is behind the camera move its projection out of clip space
	if (depth < 0.0)
    {
        posSS.xyz += float3(5.0, 5.0, 5.0);
    }

    output.pos = posSS;
    output.normal = input.normal;
	output.color = input.color;

	return output;
}
