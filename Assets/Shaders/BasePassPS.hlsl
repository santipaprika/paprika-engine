#define EPS_FLOAT 1e-8
#define PI 3.14159265359
struct PSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPos : POSITION;
	float clipDist : SV_ClipDistance0; // 1 = inside, < 0 = clipped
};

struct PSOutput {
    float4 color : SV_Target0;
};

cbuffer CB0 : register(b0)
{
	uint frameIndex : register(b0); // 0
	uint numSamples : register(b0); // 1
	uint cameraRdhIndex : register(b0); // 2
	uint objectRdhIndex : register(b0); // 3
	bool bSmartSampleAllocation : register(b0); // 4
	uint noiseTextureIndex : register(b0); // 5
	uint shadowVarianceTextureIndex : register(b0); // 6
	uint materialIndex : register(b0); // 7
	uint lightsRdhIndex : register(b0); // 8
	uint rayTracedShadowsTargetIndex : register(b0); // 9
}

struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
	matrix projectionToView;
	float2 viewSize;
	float2 invViewSize;
};

struct MaterialRenderResources
{
	uint baseColorIndex;
	uint metallicRoughnessIndex;
	uint normalIndex;
	uint occlusionIndex;
	uint emissiveIndex;
};

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

struct PointLight
{
    float3 worldPos;
    float radius;
    float3 color;
    float intensity;
};

[earlydepthstencil]
PSOutput MainPS(PSInput input)
{
	StructuredBuffer<PointLight> lightsBuffer = ResourceDescriptorHeap[lightsRdhIndex];
    PointLight light = lightsBuffer[0];
    float3 lightDirWS = normalize(light.worldPos - input.worldPos);
    float NdL = dot(lightDirWS, input.normal);

	ConstantBuffer<MaterialRenderResources> materialRenderResources = ResourceDescriptorHeap[materialIndex];
	Texture2D<float4> baseColorTex = ResourceDescriptorHeap[materialRenderResources.baseColorIndex];
    float4 baseColor = baseColorTex.Sample(linearSampler, input.uv);

	// Phong
	// TODO: Should be material parameters. Will do with pbr implementation
	const float diffuseConstant = 0.7;
	const float specularConstant = 1.0 - diffuseConstant;
	const float shininess = 25.0;

	// Diffuse
    float3 diffuse = (baseColor * saturate(NdL)).rgb;

	// Specular
	ConstantBuffer<CameraMatrices> cameraMatrices = ResourceDescriptorHeap[cameraRdhIndex];
	float3 camPosWS = cameraMatrices.viewToWorld._m03_m13_m23;
	// float3 camPosWS = float3(10.3, 6.4, 0.8);
	float3 viewDirWS = normalize(camPosWS - input.worldPos);
	float RdV = max(0.0, dot(reflect(-lightDirWS, input.normal), viewDirWS));
	float3 specular = light.color * pow(RdV, shininess);

	// Final color - ambient is added after shadow contribution
	float3 radiance = diffuseConstant * diffuse + specularConstant * specular;

	Texture2D<float> shadowVarianceTexture = ResourceDescriptorHeap[shadowVarianceTextureIndex];
	float2 screenUv = input.pos.xy * cameraMatrices.invViewSize;
	float shadowFactor = shadowVarianceTexture.Sample(pointSampler, screenUv);
	if (frac(shadowFactor) > EPS_FLOAT)
	{
		Texture2D<float> rayTracedShadowsTarget = ResourceDescriptorHeap[rayTracedShadowsTargetIndex];
		shadowFactor = rayTracedShadowsTarget.Sample(pointSampler, screenUv);
		// We're in penumbra. Sample high-res mip.
		
	}

	const float ambientConstant = 0.1;
	PSOutput psOutput;
	psOutput.color.rgb = radiance.rgb * shadowFactor + ambientConstant * baseColor.rgb;
	psOutput.color.a = 1.0;

    return psOutput;
}
