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

cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix fmodel;
    matrix view;
    matrix projection;
};

cbuffer ObjectBuffer : register(b2)
{
    matrix objectToWorld;
	float3x3 objectToWorldNormal;
};

RaytracingAccelerationStructure myScene : register(t0);
Texture2D<float2> noiseTexture : register(t1);
Texture2D<float4> albedo : register(t2);

cbuffer CB0 : register(b0)
{
	float frameIndex : register(b0);
	uint numSamples : register(b0);
}

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
    static const float a1 = 1 / g;
    static const float a2 = 1 / (g * g);
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
    ray.TMin = 0.01;
    ray.TMax = 50;

    // Sample towards a disk around the point-dependent light direction since a disk is the projection of a sphere in any
    // direction, so in essence we cover the same area. Assuming uniform spherical point lights, this should be correct.
    // If x > 0, switch x and (-)y and set Z to 0. Otherwise there's risk that x and y are both 0 and we end up with
    // {0,0,0} vector, so switch y and (-)z and set x to 0 instead.
    bool bIsXNonZero = abs(lightPseudoDirection) > EPS_FLOAT;
    float3 lightSpaceLeft = normalize(float3(bIsXNonZero ? -lightPseudoDirection.y : 0.0,
                                             bIsXNonZero ? lightPseudoDirection.x : -lightPseudoDirection.z,
                                             bIsXNonZero ? 0.0 : lightPseudoDirection.y));
    float3 lightSpaceUp = cross(lightPseudoDirection, lightSpaceLeft);
    float3x3 lightToWorld = float3x3(lightSpaceLeft, lightSpaceUp, lightPseudoDirection);

    uint noiseWidth, noiseHeight;
	// Only power of 2 supported!
    noiseTexture.GetDimensions(noiseWidth, noiseHeight);
    uint2 noiseTextureDims = uint2(noiseWidth, noiseHeight);
    uint3 noiseIndex = uint3(floor(screenPos.x) % noiseWidth, floor(screenPos.y) % noiseHeight, 0); //< TODO: frame idx here!
    float shadowFactor = 0.0;
    for (int i = 0; i < numSamples; i++)
    {
		// Naive approach: 1 sequential query per sample

		// Instantiate ray query object.
	    // Template parameter allows driver to generate a specialized
	    // implementation.
	        RayQuery < RAY_FLAG_CULL_NON_OPAQUE |
	             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
	             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    	// Using cosine-weighted 2D noise generated with https://github.com/electronicarts/fastnoise
        uint2 indexOffset = R2(i) * noiseTextureDims;
    	// use & (DIMS - 1) instead of modulo for better perf - only works if dims are power of 2
        uint3 sampleNoiseIndex = uint3((noiseIndex.xy + indexOffset) & (noiseTextureDims - 1), 0);
        float2 noise = noiseTexture.Load(sampleNoiseIndex).xy;
        float3 Offset3 = mul(lightToWorld, float3(noise, 0));

        // Set up a trace.  No work is done yet.
        ray.Direction = normalize(light.worldPos + Offset3 * light.radius - worldPos);
        q.TraceRayInline(
        myScene,
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
            shadowFactor += 1.0 / numSamples;
        }
    }

    return shadowFactor;
}

[earlydepthstencil]
PSOutput MainPS(PSInput input)
{
    PointLight light;
	light.worldPos = float3(0, 20, 0);
    light.radius = 2;

    float3 L = normalize(light.worldPos - input.worldPos);
    float ndl = saturate(dot(L, input.normal));
    float4 baseColor = albedo.Sample(linearSampler, input.uv);
    float4 radiance = baseColor * ndl;

    PSOutput psOutput;
    psOutput.color = radiance;

    psOutput.shadowFactor = 1.0 - ComputeShadowFactor(input.worldPos, light, L, input.pos.xy);

    return psOutput;
}
