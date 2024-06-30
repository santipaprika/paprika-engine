#include <stdexcept>
#include <RHI/ShaderDescriptorHeap.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
    ShaderDescriptorHeap::ShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
        :DescriptorHeap(heapType, numDescriptors, true)
    {
        m_currentDescriptorIndex = 0;
    }

    DescriptorHeapHandle ShaderDescriptorHeap::GetHeapHandleBlock(uint32_t count)
    {
        uint32_t newHandleID = 0;
        uint32_t blockEnd = m_currentDescriptorIndex + count;

        if (blockEnd < m_maxDescriptors)
        {
            newHandleID = m_currentDescriptorIndex;
            m_currentDescriptorIndex = blockEnd;
        }
        else
        {
            throw std::runtime_error("Ran out of render pass descriptor heap handles, need to increase heap size.");
        }

        DescriptorHeapHandle newHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCPUStart;
        cpuHandle.ptr += newHandleID * m_descriptorSize;
        newHandle.SetCPUHandle(cpuHandle);

        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_descriptorHeapGPUStart;
        gpuHandle.ptr += newHandleID * m_descriptorSize;
        newHandle.SetGPUHandle(gpuHandle);

        newHandle.SetHeapIndex(newHandleID);

        return newHandle;
    }

    void ShaderDescriptorHeap::Reset()
    {
        m_currentDescriptorIndex = 0;
    }
}