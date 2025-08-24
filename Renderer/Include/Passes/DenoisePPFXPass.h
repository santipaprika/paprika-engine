#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>

namespace PPK
{
    struct DenoisePassData
    {
        RHI::GPUResource* m_sceneColorTexture;
        RHI::GPUResource* m_rtShadowsTexture;
        RHI::GPUResource* m_depthTexture;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_denoiseResourcesHandle;
    };

    class DenoisePPFXPass : public Pass
    {
    public:
        DenoisePPFXPass(const wchar_t* name = L"UndefinedDenoisePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandListPPFX(std::shared_ptr<RHI::CommandContext> context);
        void AddDenoisePassRun(const DenoisePassData& denoisePassData);

        std::vector<DenoisePassData> m_denoisePassData;

    private:
        std::shared_ptr<RHI::Texture> m_inputTexture;
        RHI::DescriptorHeapHandle m_cbvBlockStart[gFrameCount];
    };
}