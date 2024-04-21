#pragma once

#include <stdafx_renderer.h>

namespace PPK::RHI
{
    class DescriptorHeapHandle
    {
    public:
        DescriptorHeapHandle();
        virtual ~DescriptorHeapHandle() = default;

        static DescriptorHeapHandle Null() { return DescriptorHeapHandle(); }

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_CPUHandle; }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_GPUHandle; }
        [[nodiscard]] uint32_t GetHeapIndex() const { return m_heapIndex; }

        void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) { m_CPUHandle = cpuHandle; }
        void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) { m_GPUHandle = gpuHandle; }
        void SetHeapIndex(uint32_t heapIndex) { m_heapIndex = heapIndex; }

        [[nodiscard]] virtual bool IsValid() const;
        [[nodiscard]] bool IsReferencedByShader() const;

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
        uint32_t m_heapIndex;
    };
}
