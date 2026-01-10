struct CameraMatrices
{
    matrix worldToView;
    matrix viewToWorld;
    matrix viewToProjection;
    matrix projectionToView;
    float2 viewSize;
    float2 invViewSize;
};

cbuffer CB0 : register(b0)
{
    uint cameraRdhIndex;
    uint skyboxRdhIndex;
};

struct VertexShaderInput
{
    uint vtxID : SV_VertexID;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 cubeDir : TEXCOORD;
};

// 14-vertex cube strip (must be used with TRIANGLESTRIP)
static const float3 g_cubePositions[14] =
{
    float3(-1, +1, -1), //  0
    float3(+1, +1, -1), //  1
    float3(-1, +1, +1), //  2
    float3(+1, +1, +1), //  3
    float3(+1, -1, +1), //  4
    float3(+1, +1, -1), //  5
    float3(+1, -1, -1), //  6
    float3(-1, +1, -1), //  7
    float3(-1, -1, -1), //  8
    float3(-1, +1, +1), //  9
    float3(-1, -1, +1), // 10
    float3(+1, -1, +1), // 11
    float3(-1, -1, -1), // 12
    float3(+1, -1, -1), // 13
};

PSInput MainVS(VertexShaderInput input)
{
    PSInput output;

    ConstantBuffer<CameraMatrices> cameraMatrices = ResourceDescriptorHeap[cameraRdhIndex];

    float3 cubePos = g_cubePositions[input.vtxID];

    // skip translation from the view matrix so the skybox stays centred on the camera.
    float3x3 camRot = (float3x3)cameraMatrices.worldToView;
    float3 cubePosVS = mul(camRot, cubePos);

    float4 clipPos = mul(cameraMatrices.viewToProjection, float4(cubePosVS, 1.0));

    // set vtx depth to cam far plane
    output.pos = clipPos.xyww;
    output.cubeDir = float3(-cubePos.x, cubePos.y, cubePos.z); //< used for cubemap sampling

    return output;
}