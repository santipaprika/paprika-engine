struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CB0 : register(b0)
{
    float time : register(b0); // 1
    bool denoise : register(b0); // 2
    uint basePassRTIndex : register(b0); // 3
    uint shadowFactorRTIndex : register(b0); // 4
    uint depthTargetIndex : register(b0); // 5
}

SamplerState defaultSampler : register(s0);

uint GetDepthComparisonResult(float4 otherDepthSamples, float pixelDepth)
{
    uint outMask = abs(otherDepthSamples.x - pixelDepth) < 0.001f;
    outMask |= (abs(otherDepthSamples.y - pixelDepth) < 0.001f) << 1;
    outMask |= (abs(otherDepthSamples.z - pixelDepth) < 0.001f) << 2;
    outMask |= (abs(otherDepthSamples.w - pixelDepth) < 0.001f) << 3;
    
    return outMask;
}

static const float b3SplineCoefficients[] = {1.0, 1.0, 1.0}; //{3.0/8.0, 1.0/4.0, 1.0/16.0};

float4 MainPS(PSInput input) : SV_TARGET
{
    // VERY UNOPTIMIZED AND HARDCODED DISTANCE-BASED BLUR
    // TODO: Find better way potentially with wave ops and apply to ray-trace output only!
    // float width;
    // float height;
    // basePassRT.GetDimensions(width, height);
    // float2 pixelSize = rcp(float2(width, height));
    // float2 pixelPerfectUV = input.uv + pixelSize * 0.5;

    const float ambientTerm = 0.15;
    int3 pixelPos = int3(input.pos.xy, 0);

    float totalShadowFactor = 0;
    float totalWeightSum = 0;

    Texture2D<float4> basePassRT = ResourceDescriptorHeap[basePassRTIndex];
    Texture2DMS<float> shadowFactorRT = ResourceDescriptorHeap[shadowFactorRTIndex];
    [branch]
    if (!denoise)
    {
        float4 finalColor = basePassRT.Load(pixelPos);
        finalColor *= ambientTerm + shadowFactorRT.Load(pixelPos, 0);
        return finalColor;
    }

    Texture2DMS<float> depthTarget = ResourceDescriptorHeap[depthTargetIndex];
    float pixelDepth = depthTarget.Load(pixelPos, 0);
    
    const int kernelSize = 11;

    // WIP custom A-trous denoising based on https://jo.dreggn.org/home/2010_atrous.pdf
    // TODO: Maybe worth outputing geometry variance target from base pass (half-res?) and add more or less holes based on that?
    // TODO: Downsample and use 1/4th res to discard if value is the same?
    [unroll]
    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        for (int j = -(kernelSize / 2); j <= kernelSize / 2; j++)
        {
            int3 neighborPos = pixelPos + int3(i, j, 0);
            float neighborDepth = (i == 0 && j == 0) ? pixelDepth : depthTarget.Load(neighborPos, 0);

            float absDepthDiff = abs(neighborDepth - pixelDepth); 
            if (absDepthDiff < 1e-5) // ensure depth proximity (TODO: Probably need normal too for better accuracy)
            {
                float neighborShadowFactor = shadowFactorRT.Load(neighborPos, 0);
                uint coeffIndex = max(abs(i), abs(j));
                float sampleWeight = 1;//exp(-absDepthDiff / 1e-5);// * b3SplineCoefficients[coeffIndex];
                totalWeightSum += sampleWeight;
                totalShadowFactor += sampleWeight * neighborShadowFactor;
            }
        }
    }
    
    totalShadowFactor /= totalWeightSum;

    float4 finalColor = basePassRT.Load(pixelPos);
    finalColor *= ambientTerm + totalShadowFactor;
    return finalColor;
}
