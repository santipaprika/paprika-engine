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
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context, Mesh& mesh, Camera& camera) override;

    private:
        std::shared_ptr<RHI::Texture> m_depthTarget;
    };

    // inline std::shared_ptr<PPK::RHI::Sampler> defaultSampler;
}
