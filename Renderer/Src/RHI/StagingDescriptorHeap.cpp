#include <RHI/StagingDescriptorHeap.h>

namespace PPK::RHI
{
    StagingDescriptorHeap::StagingDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
        :DescriptorHeap(heapType, numDescriptors, false)
    {
        m_currentDescriptorIndex = 0;
        m_activeHandleCount = 0;
        m_freeDescriptors.resize(m_maxDescriptors);
    }

    StagingDescriptorHeap::~StagingDescriptorHeap()
    {
        if (m_activeHandleCount != 0)
        {
            Logger::Error("There were active handles when the descriptor heap was destroyed. Look for leaks.");
        }

        m_freeDescriptors.clear();
    }

    DescriptorHeapHandle StagingDescriptorHeap::GetNewHeapHandle()
    {
        uint32_t newHandleID;

        if (m_currentDescriptorIndex < m_maxDescriptors)
        {
            newHandleID = m_currentDescriptorIndex;
            m_currentDescriptorIndex++;
        }
        else if (m_freeDescriptors.size() > 0)
        {
            newHandleID = m_freeDescriptors.back();
            m_freeDescriptors.pop_back();
        }
        else
        {
            Logger::Error("Ran out of dynamic descriptor heap handles, need to increase heap size.");
        }

        DescriptorHeapHandle newHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCPUStart;
        cpuHandle.ptr += newHandleID * m_descriptorSize;
        newHandle.SetCPUHandle(cpuHandle);
        newHandle.SetHeapIndex(newHandleID);
        m_activeHandleCount++;

        return newHandle;
    }

    void StagingDescriptorHeap::FreeHeapHandle(DescriptorHeapHandle handle)
    {
        m_freeDescriptors.push_back(handle.GetHeapIndex());

        if (m_activeHandleCount == 0)
        {
            throw std::runtime_error("Freeing heap handles when there should be none left");
        }
        m_activeHandleCount--;
    }
}