#pragma once

#include <stdafx_renderer.h>

namespace PPK::RHI
{
    class DescriptorHeapHandle
    {
    public:
        DescriptorHeapHandle()
        {
            mCPUHandle.ptr = NULL;
            mGPUHandle.ptr = NULL;
            mHeapIndex = 0;
        }

        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return mCPUHandle; }
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return mGPUHandle; }
        [[nodiscard]] uint32_t GetHeapIndex() const { return mHeapIndex; }

        void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) { mCPUHandle = cpuHandle; }
        void SetGPUHandle(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) { mGPUHandle = gpuHandle; }
        void SetHeapIndex(uint32_t heapIndex) { mHeapIndex = heapIndex; }

        [[nodiscard]] bool IsValid() const { return mCPUHandle.ptr != NULL; }
        [[nodiscard]] bool IsReferencedByShader() const { return mGPUHandle.ptr != NULL; }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
        uint32_t mHeapIndex;
    };
}