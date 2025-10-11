#include <GLTFReader.h>
#include <RHI/DescriptorHeapHandle.h>

namespace PPK::RHI
{
    DescriptorHeapHandle::DescriptorHeapHandle()
    {
        m_CPUHandle.ptr = NULL;
        m_GPUHandle.ptr = NULL;
        m_heapIndex = -1;
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
