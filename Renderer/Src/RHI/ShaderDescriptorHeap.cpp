#include <stdexcept>
#include <RHI/ShaderDescriptorHeap.h>
#include <RHI/DescriptorHeapHandle.h>
#include <Logger.h>

namespace PPK::RHI
{
    ShaderDescriptorHeap::ShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
        :DescriptorHeap(heapType, numDescriptors, true)
    {
        static_assert(g_NumDescriptorsPerLocationCum.size() == static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS));
        static_assert(g_NumDescriptorsPerLocation.size() == static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS));

        Reset();
    }

    DescriptorHeapHandle ShaderDescriptorHeap::GetHeapLocationNewHandle(HeapLocation heapLocation)
    {
        Logger::Assert(m_heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            L"Requesting new handle with a specific location can only be done for resource desc heaps (== CBV_SRV_UAV).");
        
        const uint32_t heapLocationIdx = static_cast<uint32_t>(heapLocation);

        // Should be atomic if multithreading is added
        const uint32_t FirstIndexAvailable = m_currentDescriptorIndex[heapLocationIdx];
        m_currentDescriptorIndex[heapLocationIdx]++;

        Logger::Assert(FirstIndexAvailable < g_NumDescriptorsPerLocationCum[heapLocationIdx],
            L"Ran out of render pass descriptor heap handles, need to increase heap size.");
        // const uint32_t locationIndex = heapLocationIndex == 0 ? 0 : g_NumDescriptorsPerLocationCum[heapLocationIndex - 1];
        // const uint32_t indexInHeap = locationIndex + offsetInLocation;

        DescriptorHeapHandle newHandle;
        newHandle.SetCPUHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCPUStart, FirstIndexAvailable, m_descriptorSize));
        newHandle.SetGPUHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapGPUStart, FirstIndexAvailable, m_descriptorSize));
        newHandle.SetHeapIndex(FirstIndexAvailable);

        return newHandle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ShaderDescriptorHeap::GetHeapLocationGPUHandle(HeapLocation heapLocation, uint32_t offsetInLocation) const
    {
        const uint32_t heapLocationIndex = static_cast<uint32_t>(heapLocation);
        const uint32_t locationIndex = heapLocationIndex == 0 ? 0 : g_NumDescriptorsPerLocationCum[heapLocationIndex - 1];
        const uint32_t indexInHeap = locationIndex + offsetInLocation;
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapGPUStart, indexInHeap, m_descriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE ShaderDescriptorHeap::GetHeapLocationGPUHandle(uint32_t offset) const
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeapGPUStart, offset, m_descriptorSize);
    }

    void ShaderDescriptorHeap::Reset()
    {
        m_currentDescriptorIndex[0] = 0;
        for (uint32_t heapLocation = 1; heapLocation < static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS); ++heapLocation)
        {
            m_currentDescriptorIndex[heapLocation] = g_NumDescriptorsPerLocationCum[heapLocation - 1];
        }
    }
}