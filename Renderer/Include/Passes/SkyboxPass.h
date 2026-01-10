#pragma once

#include <Passes/Pass.h>

namespace PPK
{
    namespace RHI
    {
        class GPUResource;
        class Texture;
    }

    class SkyboxPass : public Pass
    {
    public:
        SkyboxPass(const wchar_t* name = L"UndefinedSkyboxPass");

        // Initialize root signature, PSO and shaders
        void CreatePSO() override;
        void CreatePassResources() override;
        void DestroyPassResources() override;
        void InitPassParams() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, const SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

    private:
        RHI::GPUResource* m_renderTarget; // Owned by BasePass
        RHI::GPUResource* m_depthTarget; // Owned by DepthPass

        std::shared_ptr<RHI::Texture> m_skyboxTexture;
        uint32_t m_skyboxTextureIndex;
    };
}
