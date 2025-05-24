#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>

namespace PPK
{
    class DenoisePPFXPass : public Pass
    {
    public:
        DenoisePPFXPass(const wchar_t* name = L"UndefinedDenoisePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PrepareDescriptorTables(std::shared_ptr<RHI::CommandContext> context);
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, uint32_t meshIdx) override;
        void PopulateCommandListPPFX(std::shared_ptr<RHI::CommandContext> context);

    private:
        std::shared_ptr<RHI::Texture> m_inputTexture;
        RHI::DescriptorHeapHandle m_cbvBlockStart[gFrameCount];
    };
}