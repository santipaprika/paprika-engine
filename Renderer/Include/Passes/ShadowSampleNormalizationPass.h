#pragma once

#include <Passes/Pass.h>

namespace PPK
{
    class ShadowSampleNormalizationPass : public Pass
    {
    public:
        ShadowSampleNormalizationPass(const wchar_t* name = L"UndefinedShadowSampleNormalizationPass");

        void CreatePSO() override;
        // Initialize root signature, PSO and shaders
        void InitPassParams() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;
        int m_numSamples;

    private:
        RHI::GPUResource* m_shadowSampleScatterBuffer;
        RHI::GPUResource* m_shadowRayTracingCommandBuffer;
    };
}
