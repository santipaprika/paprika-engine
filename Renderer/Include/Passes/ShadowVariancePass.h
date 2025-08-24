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
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_objectHandle;
        uint32_t m_indexCount;
        const char* m_name;
    };

    class ShadowVariancePass : public Pass
    {
    public:
        ShadowVariancePass(const wchar_t* name = L"UndefinedShadowVariancePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

        void AddShadowVariancePassRun(const ShadowVariancePassData& shadowVariancePassData);

    private:
        RHI::GPUResource* m_depthTarget; // No ownership - Depth Pass has it
        std::shared_ptr<RHI::Texture> m_shadowVarianceTarget;
        std::shared_ptr<RHI::Texture> m_shadowVarianceTargetResolved;
        std::shared_ptr<RHI::Texture> m_noiseTexture;

        std::vector<ShadowVariancePassData> m_shadowVariancePassData;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_noiseTextureHandle;
    };
}
