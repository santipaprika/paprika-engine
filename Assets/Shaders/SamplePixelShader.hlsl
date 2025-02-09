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

Texture2D<float4> albedo : register(t0);

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
    // float pos2 = input.pos.z * input.pos.z;
    // float4 color = float4(ddx_fine(pos2) * 20000.0,ddy_fine(pos2) * 20000.0, 0.0, 1.0); 
    return color;
}
