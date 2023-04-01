#include <RHI/StagingDescriptorHeap.h>

namespace PPK::RHI
{
    StagingDescriptorHeap::StagingDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors)
        :DescriptorHeap(device, heapType, numDescriptors, false)
    {
        mCurrentDescriptorIndex = 0;
        mActiveHandleCount = 0;
        mFreeDescriptors.resize(m_maxDescriptors);
    }

    StagingDescriptorHeap::~StagingDescriptorHeap()
    {
        if (mActiveHandleCount != 0)
        {
            throw std::runtime_error("There were active handles when the descriptor heap was destroyed. Look for leaks.");
        }

        mFreeDescriptors.clear();
    }

    DescriptorHeapHandle StagingDescriptorHeap::GetNewHeapHandle()
    {
        uint32_t newHandleID;

        if (mCurrentDescriptorIndex < m_maxDescriptors)
        {
            newHandleID = mCurrentDescriptorIndex;
            mCurrentDescriptorIndex++;
        }
        else if (mFreeDescriptors.size() > 0)
        {
            newHandleID = mFreeDescriptors.back();
            mFreeDescriptors.pop_back();
        }
        else
        {
            throw std::runtime_error("Ran out of dynamic descriptor heap handles, need to increase heap size.");
        }

        DescriptorHeapHandle newHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = m_descriptorHeapCPUStart;
        cpuHandle.ptr += newHandleID * m_descriptorSize;
        newHandle.SetCPUHandle(cpuHandle);
        newHandle.SetHeapIndex(newHandleID);
        mActiveHandleCount++;

        return newHandle;
    }

    void StagingDescriptorHeap::FreeHeapHandle(DescriptorHeapHandle handle)
    {
        mFreeDescriptors.push_back(handle.GetHeapIndex());

        if (mActiveHandleCount == 0)
        {
            throw std::runtime_error("Freeing heap handles when there should be none left");
        }
        mActiveHandleCount--;
    }
}