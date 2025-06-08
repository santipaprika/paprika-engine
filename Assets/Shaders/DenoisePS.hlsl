struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D<float4> basePassRT : register(t0);
Texture2D<float> shadowFactorRT : register(t1);
Texture2D<float> depthTarget : register(t2);

cbuffer CB0 : register(b0)
{
    float time : register(b0);
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

float4 MainPS(PSInput input) : SV_TARGET
{
    // VERY UNOPTIMIZED AND HARDCODED DISTANCE-BASED BLUR
    // TODO: Find better way potentially with wave ops and apply to ray-trace output only!
    // float width;
    // float height;
    // basePassRT.GetDimensions(width, height);
    // float2 pixelSize = rcp(float2(width, height));
    // float2 pixelPerfectUV = input.uv + pixelSize * 0.5;

    int3 pixelPos = int3(input.pos.xy, 0);
    float pixelDepth = depthTarget.Load(pixelPos);
    float finalShadowFactor = shadowFactorRT.Load(pixelPos);
    uint numValidNeighbors = 1;
    
    int kernelSize = 11;
    
    [unroll]
    for (int i = -(kernelSize / 2); i <= kernelSize / 2; i++)
    {
        for (int j = -(kernelSize / 2); j <= kernelSize / 2; j++)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }

            int3 neighborPos = pixelPos - int3(i, j, 0);
            float neighborDepth = depthTarget.Load(neighborPos);
            if (abs(neighborDepth - pixelDepth) < 1e-5)
            {
                float neighborShadowFactor = shadowFactorRT.Load(neighborPos);
                finalShadowFactor += neighborShadowFactor;
                numValidNeighbors += 1;
            }

        }
    }
    
    finalShadowFactor /= float(numValidNeighbors);
        
    //float4 leftResults = depthTarget.Gather(defaultSampler, pixelPerfectUV - pixelSize * float2(-1.0, 0.0));
    //float4 rightResults = depthTarget.Gather(defaultSampler, pixelPerfectUV - pixelSize * float2(1.0, 0.0));
    //float4 topResults = depthTarget.Gather(defaultSampler, pixelPerfectUV - pixelSize * float2(1.0, 0.0));
    //float4 bottomResults = depthTarget.Gather(defaultSampler, pixelPerfectUV - pixelSize * float2(-1.0, 0.0));
    
    //uint bitMask = GetDepthComparisonResult(leftResults, pixelDepth);
    //bitMask = GetDepthComparisonResult(rightResults, pixelDepth) << 4;
    //bitMask = GetDepthComparisonResult(topResults, pixelDepth) << 8;
    //bitMask = GetDepthComparisonResult(bottomResults, pixelDepth) << 12;
    
    //uint currentBit = 0;
    //float4 pixelsLeft = float4(0, 0, 0, 0);
    //float4 pixelsRight = float4(0, 0, 0, 0);
    //float4 pixelsTop = float4(0, 0, 0, 0);
    //float4 pixelsBottom = float4(0, 0, 0, 0);
    
    //if (bitMask & 0xF) pixelsLeft = 
    //uint numBits = countbits(bitMask);
    //while (currentBit = firstbitlow(bitMask) && currentBit != 0xFFFFFFFF)
    //{
    //    if (currentBit < 4)
    //    {
    //        basePassRT.gat

    //    }
    //    else if (currentBit < 8)
    //    {
            
    //    }
    //    else if (currentBit < 12)
    //    {
            
    //    }
    //    else
    //    {
            
    //    }

    //    bitMask &= ~(1 << currentBit); // Set bit to 0
    //}
    // float4 color = pixelPerfectUV;

    float4 finalColor = basePassRT.Load(pixelPos);
    finalColor *= finalShadowFactor;

    return finalColor;  // basePassRT.Sample(defaultSampler, pixelPerfectUV);// * (depthTarget.Sample(defaultSampler, input.uv + invRes * 0.5f) > 0);
}
