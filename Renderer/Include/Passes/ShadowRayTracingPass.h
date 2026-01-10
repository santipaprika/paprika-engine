#pragma once

#include <Passes/Pass.h>
#include <RHI/ConstantBuffer.h>

namespace PPK
{
    class ShadowRayTracingPass : public Pass
    {
    public:
        ShadowRayTracingPass(const wchar_t* name = L"UndefinedShadowRayTracingPass");

        struct IndirectCommand
        {
            D3D12_DISPATCH_ARGUMENTS dispatchArguments;
        };

        void CreatePSO() override;
        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

    private:
        RHI::GPUResource* m_shadowSampleScatterBuffer; // No ownership - ShadowVariancePass has it
        RHI::GPUResource* m_shadowRayTracingCommandBuffer; // No ownership - ShadowVariancePass has it
        RHI::GPUResource* m_depthTarget; // No ownership - Depth Pass has it
        RHI::GPUResource* m_noiseTexture; // No ownership - ShadowVariancePass has it

        std::shared_ptr<RHI::Texture> m_rayTracedShadowsTarget;
        
        ComPtr<ID3D12CommandSignature> m_commandSignature;
    };
}
