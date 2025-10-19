#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>

namespace PPK
{
    struct DenoisePassData
    {
        RHI::GPUResource* m_sceneColorTexture;
        uint32_t m_sceneColorTextureIndex = INVALID_INDEX;

        RHI::GPUResource* m_rtShadowsTexture;
        uint32_t m_rtShadowsTextureIndex = INVALID_INDEX;

        RHI::GPUResource* m_depthTexture;
        uint32_t m_depthTextureIndex = INVALID_INDEX;
    };

    class DenoisePPFXPass : public Pass
    {
    public:
        DenoisePPFXPass(const wchar_t* name = L"UndefinedDenoisePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandListPPFX(std::shared_ptr<RHI::CommandContext> context);
        void AddDenoisePassRun(const DenoisePassData& denoisePassData);

        std::vector<DenoisePassData> m_denoisePassData;

    private:
        std::shared_ptr<RHI::Texture> m_inputTexture;
    };
}