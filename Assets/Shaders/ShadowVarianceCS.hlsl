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
	uint shadowSamplesScatterIndex : register(b0); // 6
}

SamplerState linearSampler : register(s0);
SamplerState pointSampler : register(s1);

struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
	matrix projectionToView;
	float2 viewSize;
	float2 invViewSize;
};

struct PointLight
{
    float3 worldPos;
    float radius;
    float3 color;
    float intensity;
};

float ComputeShadowFactor(float3 worldPos, PointLight light, float3 lightPseudoDirection, uint groupId, out float distToLight)
{
	// The goal of this pass is to know whether a tile contains penumbra area or not. To achieve so we use low-discrepancy
	// samples in a disk for each thread in the group (8x8), assuming surface locality. We don't care about banding here
	// since this result is only used to distribute ray samples, not to evaluate the result.
	// From https://blog.demofox.org/2020/05/16/using-blue-noise-for-raytraced-soft-shadows/
	const float2 gBlueNoiseInDisk[64] = {
		float2(0.478712,0.875764),
		float2(-0.337956,-0.793959),
		float2(-0.955259,-0.028164),
		float2(0.864527,0.325689),
		float2(0.209342,-0.395657),
		float2(-0.106779,0.672585),
		float2(0.156213,0.235113),
		float2(-0.413644,-0.082856),
		float2(-0.415667,0.323909),
		float2(0.141896,-0.939980),
		float2(0.954932,-0.182516),
		float2(-0.766184,0.410799),
		float2(-0.434912,-0.458845),
		float2(0.415242,-0.078724),
		float2(0.728335,-0.491777),
		float2(-0.058086,-0.066401),
		float2(0.202990,0.686837),
		float2(-0.808362,-0.556402),
		float2(0.507386,-0.640839),
		float2(-0.723494,-0.229240),
		float2(0.489740,0.317826),
		float2(-0.622663,0.765301),
		float2(-0.010640,0.929347),
		float2(0.663146,0.647618),
		float2(-0.096674,-0.413835),
		float2(0.525945,-0.321063),
		float2(-0.122533,0.366019),
		float2(0.195235,-0.687983),
		float2(-0.563203,0.098748),
		float2(0.418563,0.561335),
		float2(-0.378595,0.800367),
		float2(0.826922,0.001024),
		float2(-0.085372,-0.766651),
		float2(-0.921920,0.183673),
		float2(-0.590008,-0.721799),
		float2(0.167751,-0.164393),
		float2(0.032961,-0.562530),
		float2(0.632900,-0.107059),
		float2(-0.464080,0.569669),
		float2(-0.173676,-0.958758),
		float2(-0.242648,-0.234303),
		float2(-0.275362,0.157163),
		float2(0.382295,-0.795131),
		float2(0.562955,0.115562),
		float2(0.190586,0.470121),
		float2(0.770764,-0.297576),
		float2(0.237281,0.931050),
		float2(-0.666642,-0.455871),
		float2(-0.905649,-0.298379),
		float2(0.339520,0.157829),
		float2(0.701438,-0.704100),
		float2(-0.062758,0.160346),
		float2(-0.220674,0.957141),
		float2(0.642692,0.432706),
		float2(-0.773390,-0.015272),
		float2(-0.671467,0.246880),
		float2(0.158051,0.062859),
		float2(0.806009,0.527232),
		float2(-0.057620,-0.247071),
		float2(0.333436,-0.516710),
		float2(-0.550658,-0.315773),
		float2(-0.652078,0.589846),
		float2(0.008818,0.530556),
		float2(-0.210004,0.519896)
	};

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

    float shadowFactor = 0.0;

	RaytracingAccelerationStructure AS = ResourceDescriptorHeap[0];

	// 1 ray that will be averaged with other rays in wave to determine if we're in penumbra

	// Instantiate ray query object.
    // Template parameter allows driver to generate a specialized
    // implementation.
    RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
         RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

	float2 noise = gBlueNoiseInDisk[groupId]; //noiseTexture.Load(noiseIndex).xy;
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

#define TILE_DIM 8
#define MIN_WAVE_LANE_COUNT 16 //< Intel GPUs have 16 lanes per wave. This means we're wasting memory on NVidia / AMD :( 
#define NUM_WAVES_IN_GROUP TILE_DIM * TILE_DIM / MIN_WAVE_LANE_COUNT
groupshared float shadowFactorSumPerWave[NUM_WAVES_IN_GROUP];

[numthreads(TILE_DIM,TILE_DIM,1)]
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
		float shadowFactor = 1.0 - ComputeShadowFactor(worldPos, light, L, groupId, /* OUT */ distToLight);
		
		// if the same wave has processed distant surfaces, mark tile as 'penumbra' to prevent artifacts
		const float surfaceProximityThreshold = 0.2;
		if (WaveActiveMax(viewPos.z) - WaveActiveMin(viewPos.z) > surfaceProximityThreshold)
		{
			shadowFactor = 0.5;
		}


		float shadowFactorSum = WaveActiveSum(shadowFactor);
		if (WaveIsFirstLane())
		{
			uint waveIndex = groupId / WaveGetLaneCount();
			shadowFactorSumPerWave[waveIndex] = shadowFactorSum;
		}
		
		GroupMemoryBarrierWithGroupSync();
		// Now the first N lanes retrieve the results from LDS

		uint wavesInGroup = TILE_DIM * TILE_DIM / WaveGetLaneCount();
		if (groupId < wavesInGroup)
		{
			float waveSum = shadowFactorSumPerWave[groupId];
			float shadowFactorAverage = WaveActiveSum(waveSum) / (TILE_DIM * TILE_DIM);

			if (groupId == 0)
			{
				uint2 outScreenPos = id.xy / TILE_DIM;
				shadowVarianceTarget[outScreenPos] = shadowFactorAverage;

				if (frac(shadowFactorAverage) > EPS_FLOAT)
				{
					RWByteAddressBuffer shadowSamplesScatterBuffer = ResourceDescriptorHeap[shadowSamplesScatterIndex];
					uint scatterBufferPos;
					// Count is stored in position 0
					shadowSamplesScatterBuffer.InterlockedAdd(0, 1, scatterBufferPos);
					const uint samplesPerTileInPenumbra = 10; //< TODO: Fancier logic here, maybe based on variance or dist
					// TODO: Might be worth packing 4 8-bits per uint? 0.25x mem vs InterlockedOr...
					shadowSamplesScatterBuffer.Store(scatterBufferPos, samplesPerTileInPenumbra);
				}
			}
		}
	}
}
