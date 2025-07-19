#include <stdafx.h>
#include <Logger.h>
#include <Renderer.h>
#include <RHI/StagingDescriptorHeap.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
    StagingDescriptorHeap::StagingDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
        :DescriptorHeap(heapType, numDescriptors, false)
    {
        Logger::Assert(IsMainThread());

        m_currentDescriptorIndex = 0;
        m_activeHandleCount = 0;
        m_freeDescriptors.reserve(m_maxDescriptors);
    }

    StagingDescriptorHeap::~StagingDescriptorHeap()
    {
        Logger::Assert(IsMainThread());

        if (m_activeHandleCount != 0)
        {
            Logger::Error("There were active handles when the descriptor heap was destroyed. Look for leaks.");
        }

        m_freeDescriptors.clear();
    }

    DescriptorHeapHandle StagingDescriptorHeap::GetNewHeapHandle()
    {
        uint32_t newHandleID;
        DescriptorHeapHandle newHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCPUStart;

        {
            std::scoped_lock lock(m_handleUpdateMutex);
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

            cpuHandle.ptr += newHandleID * m_descriptorSize;
            newHandle.SetCPUHandle(cpuHandle);
            newHandle.SetHeapIndex(newHandleID);
            m_activeHandleCount++;
        }

        return newHandle;
    }

    void StagingDescriptorHeap::FreeHeapHandle(DescriptorHeapHandle handle)
    {
        std::scoped_lock lock(m_handleUpdateMutex);

        m_freeDescriptors.push_back(handle.GetHeapIndex());

        if (m_activeHandleCount == 0)
        {
            throw std::runtime_error("Freeing heap handles when there should be none left");
        }
        m_activeHandleCount--;
    }
}