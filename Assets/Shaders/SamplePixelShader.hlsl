struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPos : POSITION;
};

cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
    matrix model;
    matrix view;
    matrix projection;
};

cbuffer ObjectBuffer : register(b2)
{
    matrix objectToWorld;
};

RaytracingAccelerationStructure myScene : register(t0);
Texture2D<float4> albedo : register(t1);

cbuffer CB0 : register(b0)
{
	float time : register(b0);
}

SamplerState defaultSampler : register(s0);

[earlydepthstencil]
float4 PSMain(PixelShaderInput input) : SV_TARGET
{
    float width;
    float height;
    albedo.GetDimensions(width, height);
    float3 lightPos = float3(10, 10, -10);
    float3 L = normalize(lightPos - input.worldPos);
    float ndl = saturate(dot(L, input.normal));
    //return float4(input.normal * 0.5 + 0.5, 1.0f);
    float4 color = albedo.Sample(defaultSampler, input.uv) * ndl;

    // Instantiate ray query object.
    // Template parameter allows driver to generate a specialized
    // implementation.
    RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;

    // Set up a trace.  No work is done yet.

    RayDesc ray;
    ray.Origin = float3(0,0,0);
    ray.TMin = 0;
    ray.TMax = 1000;
    ray.Direction = float3(0,1,0);
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
    if(q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
    }
    else // COMMITTED_NOTHING
        // From template specialization,
            // COMMITTED_PROCEDURAL_PRIMITIVE can't happen.
    {
        color = float4(1,0,0,1);
    }
    // float pos2 = input.pos.z * input.pos.z;
    // float4 color = float4(ddx_fine(pos2) * 20000.0,ddy_fine(pos2) * 20000.0, 0.0, 1.0); 
    return color;
}
