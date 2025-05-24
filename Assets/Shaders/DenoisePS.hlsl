struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float4> basePassRT : register(t0);

cbuffer CB0 : register(b0)
{
    float time : register(b0);
}

SamplerState defaultSampler : register(s0);

float4 MainPS(PixelShaderInput input) : SV_TARGET
{
    // TODO: Denoise :)
    float2 invRes = rcp(float2(1280.0, 720.0));
    return basePassRT.Sample(defaultSampler, input.uv + invRes * 0.5);
}
