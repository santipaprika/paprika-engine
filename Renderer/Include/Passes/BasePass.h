#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>
#include <vector>

namespace PPK
{
    struct BasePassData
    {
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_objectHandle;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_materialHandle;
        uint32_t m_indexCount;
        const char* m_name;
    };

    class BasePass : public Pass
    {
    public:
        BasePass(const wchar_t* name = L"UndefinedBasePass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

        void AddBasePassRun(const BasePassData& basePassData);

        D3D12_GPU_DESCRIPTOR_HANDLE m_shadowVarianceTargetHandle;

    private:
        RHI::GPUResource* m_depthTarget; // Owned by DepthPass
        std::shared_ptr<RHI::Texture> m_renderTarget;
        std::shared_ptr<RHI::Texture> m_resolvedRenderTarget;
        std::shared_ptr<RHI::Texture> m_rayTracedShadowsTarget;
        RHI::GPUResource* m_noiseTexture; // Owned by ShadowVariancePass

        std::vector<BasePassData> m_basePassData;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_noiseTextureHandle;

        // Num raytrace samples should only be modified by imgui result in Application
        friend class Application;
        int m_numSamples;
    };
}
