#pragma once

#include <RHI/Texture.h>
#include <Passes/Pass.h>
#include <RHI/ConstantBuffer.h>
#include <vector>

namespace PPK
{
    class ShadowVariancePass : public Pass
    {
    public:
        ShadowVariancePass(const wchar_t* name = L"UndefinedShadowVariancePass");

        void CreatePSO() override;
        // Initialize root signature, PSO and shaders
        void CreatePassResources() override;
        void DestroyPassResources() override;
        void InitPassParams() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

    private:
        RHI::GPUResource* m_depthTarget; // No ownership - Depth Pass has it
        std::shared_ptr<RHI::Texture> m_shadowVarianceTarget;
        std::shared_ptr<RHI::Texture> m_shadowVarianceTargetResolved;
        std::shared_ptr<RHI::Texture> m_noiseTexture;
        RHI::ConstantBuffer m_shadowSampleScatterBuffer;
        RHI::ConstantBuffer m_shadowRayTracingCommandBuffer;
        uint32_t m_noiseTextureIndex;
    };
}
