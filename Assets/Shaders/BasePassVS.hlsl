
struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
	matrix projectionToView;
};

struct ObjectMatrices
{
	matrix objectToWorld;
	float3x3 objectToWorldNormal;
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

cbuffer CB0 : register(b0)
{
	float frameIndex : register(b0);
	uint numSamples : register(b0);
	uint cameraRdhIndex : register(b0);
	uint objectRdhIndex : register(b0);
}

PSInput MainVS(VertexShaderInput input)
{
	ConstantBuffer<CameraMatrices> cameraMatrices = ResourceDescriptorHeap[cameraRdhIndex];
	ConstantBuffer<ObjectMatrices> objectMatrices = ResourceDescriptorHeap[objectRdhIndex];

	PSInput output;
	float3 worldNormal = mul(objectMatrices.objectToWorldNormal, input.normal);
	worldNormal = normalize(worldNormal);

    float4 worldPos = mul(objectMatrices.objectToWorld, float4(input.pos, 1.0));
	float4 viewPos = mul(cameraMatrices.worldToView, worldPos);
    float4 clipPos = mul(cameraMatrices.viewToProjection, viewPos);

    const float depth = -viewPos.z;
	const float nearPlane = 0.001; //< hardcoded for now :/ but could be either extracted from matrix or passed as cb element
	const float distToNearPlane = depth - nearPlane;

    output.pos = clipPos;
    output.normal = worldNormal;
	output.color = input.color;
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;

	// Clip anything behind near plane (view-space Z <= NEAR)
	output.clipDist = distToNearPlane;

	return output;
}
