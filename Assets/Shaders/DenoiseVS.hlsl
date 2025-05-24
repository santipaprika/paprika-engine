struct VertexShaderInput
{
    uint vtxID : SV_VertexID;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PixelShaderInput MainVS(VertexShaderInput input)
{
    PixelShaderInput output;

    // Generate UV coordinates in [0, 2] space
    output.uv = float2((input.vtxID << 1) & 2, input.vtxID & 2);
    
    // Map UV [0,2] to clip space [-1, 1] (covers entire screen)
    output.pos = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

    return output;
}