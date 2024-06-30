#pragma once

#include <d3d12.h>
#include <d3dx12/d3dx12.h>

namespace PPK::RHI
{
	class DescriptorHeap
	{
	public:
		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors, bool isReferencedByShader);
		virtual ~DescriptorHeap();

		[[nodiscard]] ID3D12DescriptorHeap* GetHeap() const { return m_descriptorHeap; }
		[[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_heapType; }
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUStart() const { return m_descriptorHeapCPUStart; }
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetHeapCPUAtIndex(UINT index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCPUStart, index, m_descriptorSize); }
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetHeapGPUStart() const { return m_descriptorHeapGPUStart; }
		[[nodiscard]] uint32_t GetMaxDescriptors() const { return m_maxDescriptors; }
		[[nodiscard]] uint32_t GetDescriptorSize() const { return m_descriptorSize; }

	protected:
		ID3D12DescriptorHeap* m_descriptorHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
		D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHeapCPUStart;
		D3D12_GPU_DESCRIPTOR_HANDLE m_descriptorHeapGPUStart;
		uint32_t m_maxDescriptors;
		uint32_t m_descriptorSize;
		bool m_isReferencedByShader;
	};
}