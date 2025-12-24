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
    float shadowFactor : SV_Target1;
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
}

struct CameraMatrices
{
	matrix worldToView;
	matrix viewToWorld;
	matrix viewToProjection;
	matrix projectionToView;
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

// R2 is from http://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/
// R2 Low discrepancy sequence
float2 R2(int index)
{
    static const float g  = 1.32471795724474602596f;
    static const float a1 = 1.0 / g;
    static const float a2 = 1.0 / (g * g);
    return float2(frac(float(index) * a1), frac(float(index) * a2));
}

float ComputeShadowFactor(float3 worldPos, PointLight light, float3 lightPseudoDirection, float2 screenPos)
{
    // Avoid lowpoly shadows:
    // From Ray Tracing Gems II - HACKING THE SHADOW TERMINATOR - Johannes Hanika - KIT/Weta Digital
    // To use that implementation we need tangent info first via vertex non-interpolated normals, TODO!
    
    // So ugly shadows for now:
    RayDesc ray;
    ray.Origin = worldPos;
    ray.TMin = 0.02;

	float sampleMultiplier = 1.0;
	[branch]
	if (bSmartSampleAllocation)
	{
		// Fetch variance to estimate how many samples will we need
		Texture2D<float> shadowVarianceTexture = ResourceDescriptorHeap[shadowVarianceTextureIndex];
		float averageFactor = shadowVarianceTexture.Load(int3(screenPos, 0), 0);

		// Early exit: If averaged shadow factor from variance pass is 0 or 1, use that and avoid more ray traces
		// TODO: This has artifacts for now, but probably with a mip-like factor we should get rid of them.
		[branch]
		if (frac(averageFactor) < EPS_FLOAT)
		{
			return 1.0 - averageFactor;
		}

		sampleMultiplier = 5.0;
	}

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
	uint finalNumSamples = numSamples * sampleMultiplier; // TODO: Add debug counter for total number of rays!

	RaytracingAccelerationStructure AS = ResourceDescriptorHeap[0];
    for (uint i = 0; i < finalNumSamples; i++)
    {
		// Naive approach: 1 sequential query per sample

		// Instantiate ray query object.
	    // Template parameter allows driver to generate a specialized
	    // implementation.
	        RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
	             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
	             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;
		// TODO: If we assume closed opaque meshes, then add RAY_FLAG_CULL_BACK_FACING_TRIANGLES. Sponza does not guarantee that :(

    	// Using cosine-weighted 2D noise generated with https://github.com/electronicarts/fastnoise
        uint2 indexOffset = R2(i) * noiseTextureDims;
    	// use & (DIMS - 1) instead of modulo for better perf - only works if dims are power of 2
        uint3 sampleNoiseIndex = uint3((noiseIndex.xy + indexOffset) & (noiseTextureDims - 1), 0);
        float2 noise = noiseTexture.Load(sampleNoiseIndex).xy;
    	noise = mad(noise, 2.0, -1.0);
        float3 Offset3 = mul(lightToWorld, float3(noise, 0));
		float3 rayPath = light.worldPos + Offset3 * light.radius - worldPos;
		
        // Set up a trace.  No work is done yet.
    	ray.TMax = length(rayPath);
        ray.Direction = rayPath / ray.TMax;
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
            shadowFactor += 1.0 / finalNumSamples;
        }
    }

    return shadowFactor;
}

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
	PSOutput psOutput;
    psOutput.color = float4(radiance.rgb, 1.0);

	[branch]
	if (NdL > sin(-PI / 12.0)) //< Don't trace rays if NdL is smaller than -15 degrees
	{
		psOutput.shadowFactor = 1.0 - ComputeShadowFactor(input.worldPos, light, lightDirWS, input.pos.xy);
	}
	else
	{
		psOutput.shadowFactor = 0.0;
	}

    return psOutput;
}
