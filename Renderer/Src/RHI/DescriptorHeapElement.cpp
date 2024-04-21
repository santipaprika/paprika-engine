#include <RHI/DescriptorHeapElement.h>
#include <RHI/DescriptorHeapManager.h>

namespace PPK::RHI
{
	//DescriptorHeapElement::DescriptorHeapElement(const DescriptorHeapHandle& handle)
	//	: m_descriptorHeapHandle(handle)
	//{
	//}

	DescriptorHeapElement::DescriptorHeapElement(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
		: m_heapType(heapType),
		  DescriptorHeapHandle(DescriptorHeapManager::Get()->GetNewStagingHeapHandle(heapType))

	{
		assert(heapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);

		//m_descriptorHeapHandle = DescriptorHeapManager::Get()->GetNewStagingHeapHandle(heapType);
	}

	std::shared_ptr<DescriptorHeapElement> DescriptorHeapElement::Null()
	{
		// Call null constructor
		return std::make_shared<DescriptorHeapElement>();
	}

	DescriptorHeapElement::~DescriptorHeapElement()
	{
		if (IsValid())
		{
			DescriptorHeapManager::Get()->FreeDescriptor(m_heapType, static_cast<DescriptorHeapHandle>(*this));
		}
	}

	bool DescriptorHeapElement::IsValid() const
	{
		return DescriptorHeapHandle::IsValid() && m_heapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}

	DescriptorHeapElement::DescriptorHeapElement()
		: m_heapType(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES),
		  DescriptorHeapHandle(DescriptorHeapHandle::Null())
	{
	}
}
