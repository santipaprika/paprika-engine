struct PSInput
{
    float4 pos       : SV_POSITION;
    float3 cubeDir   : TEXCOORD;   // object-space cube position -> cubemap sample direction
};

cbuffer CB0 : register(b0)
{
    uint cameraRdhIndex;
    uint skyboxRdhIndex;
};

SamplerState linearSampler : register(s0);

float4 MainPS(PSInput input) : SV_Target0
{
	TextureCube<float4> skyboxTexture = ResourceDescriptorHeap[skyboxRdhIndex];
    float3 color = skyboxTexture.SampleLevel(linearSampler, input.cubeDir, 0).rgb;
    return float4(color, 1.0);
}