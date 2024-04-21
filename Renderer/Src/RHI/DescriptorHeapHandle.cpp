#include <RHI/DescriptorHeapHandle.h>
#include <RHI/DescriptorHeapManager.h>

namespace PPK::RHI
{
    DescriptorHeapHandle::DescriptorHeapHandle()
    {
        m_CPUHandle.ptr = NULL;
        m_GPUHandle.ptr = NULL;
        m_heapIndex = 0;
    }

    bool DescriptorHeapHandle::IsValid() const
    {
        return m_CPUHandle.ptr != NULL;
    }

    bool DescriptorHeapHandle::IsReferencedByShader() const
    {
        return m_GPUHandle.ptr != NULL;
    }
}
