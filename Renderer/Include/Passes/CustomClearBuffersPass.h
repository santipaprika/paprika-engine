#pragma once

#include <RHI/Texture.h>
#include <Passes/Pass.h>
#include <RHI/ConstantBuffer.h>
#include <vector>

namespace PPK
{
    class CustomClearBuffersPass : public Pass
    {
    public:
        CustomClearBuffersPass(const wchar_t* name = L"UndefinedCustomClearBufferPass");

        void CreatePSO() override;
        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

    private:
        RHI::GPUResource* m_shadowSampleScatterBuffer; // No ownership - ShadowVariancePass has it
    };
}
