#define EPS_FLOAT 1e-8
#define PI 3.14159265359

cbuffer CB0 : register(b0)
{
	// TOTAL: 6
	float frameIndex : register(b0); // 0
	uint cameraRdhIndex : register(b0); // 1
	uint noiseTextureIndex : register(b0); // 2
	uint lightsRdhIndex : register(b0); // 3
	uint depthTargetIndex : register(b0); // 4
	uint shadowVarianceTargetIndex : register(b0); // 5
}

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
	matrix projectionToView;
};

struct PointLight
{
    float3 worldPos;
    float radius;
    float3 color;
    float intensity;
};

// R2 is from http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
// R2 Low discrepancy sequence
float2 R2(int index)
{
    static const float g  = 1.32471795724474602596f;
    static const float a1 = 1 / g;
    static const float a2 = 1 / (g * g);
    return float2(frac(float(index) * a1), frac(float(index) * a2));
}

float ComputeShadowFactor(float3 worldPos, PointLight light, float3 lightPseudoDirection, float2 screenPos, out float distToLight)
{
    // Avoid lowpoly shadows:
    // From Ray Tracing Gems II - HACKING THE SHADOW TERMINATOR - Johannes Hanika - KIT/Weta Digital
    // To use that implementation we need tangent info first via vertex non-interpolated normals, TODO!
    
    // So ugly shadows for now:
    RayDesc ray;
    ray.Origin = worldPos;
    ray.TMin = 0.03;

    // Sample towards a disk around the point-dependent light direction since a disk is the projection of a sphere in any
    // direction, so in essence we cover the same area. Assuming uniform spherical point lights, this should be correct.
    // If x > 0, switch x and (-)y and set Z to 0. Otherwise there's risk that x and y are both 0 and we end up with
    // {0,0,0} vector, so switch y and (-)z and set x to 0 instead.
    bool bIsXNonZero = abs(lightPseudoDirection.x) > EPS_FLOAT;
    float3 lightSpaceLeft = normalize(float3(bIsXNonZero ? -lightPseudoDirection.y : 0.0,
                                             bIsXNonZero ? lightPseudoDirection.x : -lightPseudoDirection.z,
                                             bIsXNonZero ? 0.0 : lightPseudoDirection.y));
    float3 lightSpaceUp = cross(lightPseudoDirection, lightSpaceLeft);
    float3x3 lightToWorld = float3x3(lightSpaceLeft, lightSpaceUp, lightPseudoDirection);

	Texture2D<float2> noiseTexture = ResourceDescriptorHeap[noiseTextureIndex];
    uint noiseWidth, noiseHeight;
	// Only power of 2 supported!
    noiseTexture.GetDimensions(noiseWidth, noiseHeight);
    uint2 noiseTextureDims = uint2(noiseWidth, noiseHeight);
    uint3 noiseIndex = uint3(floor(screenPos.x) % noiseWidth, floor(screenPos.y) % noiseHeight, 0); //< TODO: frame idx here!
    float shadowFactor = 0.0;

	RaytracingAccelerationStructure AS = ResourceDescriptorHeap[0];

	// 1 ray that will be averaged with other rays in wave to determine if we're in penumbra

	// Instantiate ray query object.
    // Template parameter allows driver to generate a specialized
    // implementation.
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
         RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    // Using cosine-weighted 2D noise generated with https://github.com/electronicarts/fastnoise
    uint2 indexOffset = R2(0) * noiseTextureDims;
    // use & (DIMS - 1) instead of modulo for better perf - only works if dims are power of 2
    uint3 sampleNoiseIndex = uint3((noiseIndex.xy + indexOffset) & (noiseTextureDims - 1), 0);
    float2 noise = noiseTexture.Load(noiseIndex).xy;
    noise = mad(noise, 2.0, -1.0);
    float3 Offset3 = mul(lightToWorld, float3(noise, 0));
	float3 rayPath = light.worldPos + Offset3 * light.radius - worldPos;
	distToLight = length(rayPath);
    // Set up a trace.  No work is done yet.
    ray.TMax = distToLight;
    ray.Direction = rayPath / distToLight;
    q.TraceRayInline(
    AS,
    0, // OR'd with flags above
    1,
    ray);

	// Proceed() below is where behind-the-scenes traversal happens,
    // including the heaviest of any driver inlined code.
    // In this simplest of scenarios, Proceed() only needs
    // to be called once rather than a loop:
    // Based on the template specialization above,
    // traversal completion is guaranteed.
    q.Proceed();

	// Examine and act on the result of the traversal.
	// Was a hit committed?
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        shadowFactor += 1.0;
    }

    return shadowFactor;
}

[numthreads(8,8,1)]
void MainCS(uint3 id : SV_DispatchThreadID, uint groupId : SV_GroupIndex)
{
	uint2 screenPos = id.xy;
	Texture2DMS<float> depthTarget = ResourceDescriptorHeap[depthTargetIndex];
	RWTexture2D<float> shadowVarianceTarget = ResourceDescriptorHeap[shadowVarianceTargetIndex];
	uint width;
  	uint height;
  	uint numberOfSamples;
	depthTarget.GetDimensions(width, height, numberOfSamples);
	float2 clipPos = id.xy * rcp(uint2(width, height));

	float pixelDepth = depthTarget.Load(screenPos, 0); // Should be average of 4 samples?

	ConstantBuffer<CameraMatrices> cameraMatrices = ResourceDescriptorHeap[cameraRdhIndex];

	clipPos.y = 1.0f - clipPos.y;
	float4 ndcClipPos = float4(clipPos * 2.0 - 1.0, pixelDepth, 1.0);
	float4 viewPos = mul(cameraMatrices.projectionToView, ndcClipPos);
	viewPos /= viewPos.w;
	float3 worldPos = mul(cameraMatrices.viewToWorld, viewPos).xyz;
	
	StructuredBuffer<PointLight> lightsBuffer = ResourceDescriptorHeap[lightsRdhIndex];
	PointLight light = lightsBuffer[0];

    float3 L = normalize(light.worldPos - worldPos);
	{
		float distToLight = 1.0;
		float shadowFactor = 1.0 - ComputeShadowFactor(worldPos, light, L, screenPos, /* OUT */ distToLight);
		

		// if (WaveActiveMax(viewPos.z) - WaveActiveMin(viewPos.z) < 0.1) // 10cm
		{
			float averageShadowFactor = WaveActiveSum(shadowFactor) / WaveActiveCountBits(true); 
			shadowVarianceTarget[screenPos] = averageShadowFactor;
			return;
		}

		// float quadAvgViewPosZ = (QuadReadLaneAt(viewPos.z, 0) + QuadReadLaneAt(viewPos.z, 1) +
		// 	QuadReadLaneAt(viewPos.z, 2) + QuadReadLaneAt(viewPos.z, 3)) / 4;
		// if (abs(quadAvgViewPosZ - viewPos.z) < 0.1)
		// {
		// 	shadowVarianceTarget[screenPos] = quadAvgViewPosZ;
		// }
		// else
		// {
		// 	shadowVarianceTarget[screenPos] = 0.5;
		// }
	}

}
