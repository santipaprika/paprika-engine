#include <ApplicationHelper.h>
#include <Renderer.h>
#include <RHI/DescriptorHeap.h>

namespace PPK::RHI
{
    DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors, bool isReferencedByShader)
    {
        m_heapType = heapType;
        m_maxDescriptors = numDescriptors;
        m_isReferencedByShader = isReferencedByShader;

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        heapDesc.NumDescriptors = m_maxDescriptors;
        heapDesc.Type = m_heapType;
        heapDesc.Flags = m_isReferencedByShader ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;

        ThrowIfFailed(gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

        m_descriptorHeapCPUStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        if (m_isReferencedByShader)
        {
            m_descriptorHeapGPUStart = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        }

        m_descriptorSize = gDevice->GetDescriptorHandleIncrementSize(m_heapType);
    }

    DescriptorHeap::~DescriptorHeap()
    {
        m_descriptorHeap->Release();
        m_descriptorHeap = nullptr;
    }
}