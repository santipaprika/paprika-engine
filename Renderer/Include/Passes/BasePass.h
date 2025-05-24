#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>

namespace PPK
{
    class BasePass : public Pass
    {
    public:
        BasePass(const wchar_t* name = L"UndefinedBasePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PrepareDescriptorTables(std::shared_ptr<RHI::CommandContext> context, CameraComponent& camera, RHI::GPUResource* TLAS);
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, uint32_t meshIdx) override;

    private:
        std::shared_ptr<RHI::Texture> m_depthTarget;
        std::shared_ptr<RHI::Texture> m_renderTarget;
        static constexpr uint32_t FrameCount = 2;
        RHI::DescriptorHeapHandle m_cbvBlockStart[FrameCount];
    };
}
