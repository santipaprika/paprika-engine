#pragma once

#include <Passes/Pass.h>
#include <RHI/Texture.h>
#include <vector>

namespace PPK
{
    struct DepthPassData
    {
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        std::array<D3D12_GPU_DESCRIPTOR_HANDLE, gFrameCount> m_objectHandle;
        uint32_t m_indexCount;
        const char* m_name;
    };

    class DepthPass : public Pass
    {
    public:
        DepthPass(const wchar_t* name = L"UndefinedDepthPass");

        // Initialize root signature, PSO and shaders
        void InitPass() override;
        void BeginPass(std::shared_ptr<RHI::CommandContext> context) override;
        void PopulateCommandList(std::shared_ptr<RHI::CommandContext> context) override;

        void AddDepthPassRun(const DepthPassData& depthPassData);

    private:
        std::shared_ptr<RHI::Texture> m_depthTarget;

        std::vector<DepthPassData> m_depthPassData;
    };
}
