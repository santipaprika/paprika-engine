struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
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

[earlydepthstencil]
float4 PSMain(PixelShaderInput input) : SV_TARGET
{
    float width;
    float height;
    albedo.GetDimensions(width, height);
    float3 lightPos = float3(10, 10, -10);
    float ndl = saturate(dot(lightPos, input.normal));
    //return float4(input.normal * 0.5 + 0.5, 1.0f);
    return float4(input.uv.x, input.uv.y, 1.0f, 1.0f) * width;
}
