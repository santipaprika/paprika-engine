#pragma once

#include <RHI/DescriptorHeapHandle.h>
#include <memory>

namespace PPK
{
	namespace RHI
	{
		// class DescriptorHeapElement : public DescriptorHeapHandle
		// {
		// public:
		// 	DescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		//
		// 	// Should only be referenced by Null()
		// 	DescriptorHeapElement();
		// 	static std::shared_ptr<DescriptorHeapElement> Null();
		//
		// 	~DescriptorHeapElement() override;
		//
		// 	[[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_heapType; }
		// 	bool IsValid() const override;
		// private:
		// 	
		// 	void SetHeapType(D3D12_DESCRIPTOR_HEAP_TYPE heapType) { m_heapType = heapType; }
		//
		//
		// 	D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
		// };
	}
}
