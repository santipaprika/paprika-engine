#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>

namespace PPK
{
    class BasePass : public Pass
    {
    public:
        BasePass();

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PrepareDescriptorTables(std::shared_ptr<RHI::CommandContext> context, CameraComponent& camera);
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, MeshComponent& mesh, uint32_t meshIdx) override;

    private:
        std::shared_ptr<RHI::Texture> m_depthTarget;
        static constexpr uint32_t FrameCount = 2;
        RHI::DescriptorHeapHandle cbvBlockStart[FrameCount]; // Frame count
    };
}
