#pragma once

#include <RHI/Texture.h>
#include <Passes/Pass.h>
#include <vector>

namespace PPK
{
    struct ShadowVariancePassData
    {
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        uint32_t m_indexCount;
        uint32_t m_objectRdhIndex;
        const char* m_name;
    };

    class ShadowVariancePass : public Pass
    {
    public:
        ShadowVariancePass(const wchar_t* name = L"UndefinedShadowVariancePass");

        void CreatePSO() override;
        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context, SceneRenderContext sceneRenderContext) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

        void AddShadowVariancePassRun(const ShadowVariancePassData& shadowVariancePassData);

    private:
        RHI::GPUResource* m_depthTarget; // No ownership - Depth Pass has it
        std::shared_ptr<RHI::Texture> m_shadowVarianceTarget;
        std::shared_ptr<RHI::Texture> m_shadowVarianceTargetResolved;
        std::shared_ptr<RHI::Texture> m_noiseTexture;

        std::vector<ShadowVariancePassData> m_shadowVariancePassData;
        uint32_t m_noiseTextureIndex;
    };
}
