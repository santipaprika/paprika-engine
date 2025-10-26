#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>
#include <vector>

namespace PPK
{
    class ShadowVariancePass;
}

namespace PPK
{
    struct BasePassData
    {
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        uint32_t m_materialRdhIndex;
        uint32_t m_objectRdhIndex;
        uint32_t m_indexCount;
        const char* m_name;
    };

    class BasePass : public Pass
    {
    public:
        BasePass(const wchar_t* name = L"UndefinedBasePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

        void AddBasePassRun(const BasePassData& basePassData);

    private:
        RHI::GPUResource* m_depthTarget; // Owned by DepthPass
        std::shared_ptr<RHI::Texture> m_renderTarget;
        std::shared_ptr<RHI::Texture> m_resolvedRenderTarget;
        std::shared_ptr<RHI::Texture> m_rayTracedShadowsTarget;
        RHI::GPUResource* m_noiseTexture; // Owned by ShadowVariancePass
        RHI::GPUResource* m_shadowVarianceTarget; // Owned by ShadowVariancePass

        std::vector<BasePassData> m_basePassData;

        uint32_t m_noiseTextureIndex;
        uint32_t m_shadowVarianceTargetIndex;

        // Num raytrace samples should only be modified by imgui result in Application
        friend class Application;
        friend class ShadowVariancePass;
        int m_numSamples;
    };
}
