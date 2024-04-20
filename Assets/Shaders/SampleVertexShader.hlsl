cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix worldToView;
    matrix viewToWorld;
    matrix viewToProjection;
};

cbuffer ObjectBuffer : register(b2)
{
    matrix objectToWorld;
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
    float2 uv : TEXCOORD;
    float3 worldPos : POSITION;
};

PixelShaderInput VSMain(VertexShaderInput input)
{
	PixelShaderInput output;

    float4 worldPos = mul(objectToWorld, float4(input.pos, 1.0));

    matrix viewProj = mul(viewToProjection, worldToView);
    //float3 pos = input.pos;// * 0.03;
    // float4 posWS = mul(model, float4(pos, 1.0));
    float4 posClip = mul(viewProj, worldPos);

    const float depth = posClip.w;
	posClip /= posClip.w;

     // If vertex is behind the camera move its projection out of clip space
	if (depth < 0.0)
    {
        posClip.xyz += float3(5.0, 5.0, 5.0);
    }

    output.pos = posClip;
    output.normal = input.normal;
	output.color = input.color;
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;

	return output;
}
