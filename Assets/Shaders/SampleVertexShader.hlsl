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

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPos : POSITION;
	float clipDist : SV_ClipDistance0; // 1 = inside, < 0 = clipped
};

PSInput MainVS(VertexShaderInput input)
{
	PSInput output;

    float4 worldPos = mul(objectToWorld, float4(input.pos, 1.0));

	float4 viewPos = mul(worldToView, worldPos);
    float4 clipPos = mul(viewToProjection, viewPos);

    const float depth = -viewPos.z;

	// TODO: Profile this vs with dynamic branching for early out
	const float nearPlane = 0.001; //< hardcoded for now :/ but could be either extracted from matrix or passed as cb element
	const float distToNearPlane = depth - nearPlane;

    output.pos = clipPos;
    output.normal = input.normal;
	output.color = input.color;
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;

	// Clip anything behind near plane (view-space Z <= NEAR)
	output.clipDist = distToNearPlane;

	return output;
}
