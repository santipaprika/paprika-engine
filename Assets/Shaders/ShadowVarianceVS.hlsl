struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
};

// cbuffer ModelViewProjectionConstantBuffer : register(b1)
// {
//     matrix worldToView;
//     matrix viewToWorld;
//     matrix viewToProjection;
// };

struct ObjectMatrices
{
	matrix objectToWorld;
	float3x3 objectToWorldNormal;
};

// cbuffer ObjectBuffer : register(b2)
// {
//     matrix objectToWorld;
//     float3x3 objectToWorldNormal;
// };

cbuffer CB0 : register(b0)
{
	float frameIndex : register(b0);
	uint cameraRdhIndex : register(b0);
	uint objectRdhIndex : register(b0);
}

struct VertexShaderInput
{
	float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
    float3 worldPos : POSITION;
	float clipDist : SV_ClipDistance0; // 1 = inside, < 0 = clipped
};

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
    output.worldPos = worldPos.xyz;

	// Clip anything behind near plane (view-space Z <= NEAR)
	output.clipDist = distToNearPlane;

	return output;
}
