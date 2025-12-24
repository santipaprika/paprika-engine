#pragma once

#include <RHI/DescriptorHeapElement.h>
#include <array>
#include <mutex>
#include <stdafx_renderer.h>
#include <Logger.h>
#include <d3dx12/d3dx12_barriers.h>

#define INVALID_INDEX -1

using namespace Microsoft::WRL;

template<class E>
	constexpr auto ToInt(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}

namespace PPK::RHI
{
	// Max mips: 12 -> 2k res
	constexpr uint8_t gMaxMipsAllowed = 12;

	class DescriptorHeapHandle;

	enum class EResourceViewType
	{
		CBV = 0,
		SRV,
		UAV,
		RTV,
		DSV,
		NUM_TYPES
	};

	inline EResourceViewType HeapTypeToViewType(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		switch (heapType)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			Logger::Warning("Getting Resource view type from CBV_SRV_UAV heap type. Can't extract exact view type.");
			return EResourceViewType::CBV;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			Logger::Error("Sampler view types not supported yet.");
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
			return EResourceViewType::DSV;
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
			return EResourceViewType::RTV;
		default:
			Logger::Error("Heap type cannot be converted to resource view type");
		}

		return EResourceViewType::NUM_TYPES;
	}

	inline D3D12_DESCRIPTOR_HEAP_TYPE ViewTypeToHeapType(EResourceViewType viewType)
	{
		switch (viewType)
		{
		case EResourceViewType::CBV:
		case EResourceViewType::SRV:
		case EResourceViewType::UAV:
			return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case EResourceViewType::RTV:    return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case EResourceViewType::DSV:    return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		default: Logger::Error("Input resource view type cannot be converted to heap type.");
		}
		return D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	}

	struct DescriptorHeapHandles
	{
		DescriptorHeapHandle& At(uint32_t frameId, EResourceViewType resourceViewType)
		{
			return handles[frameId][ToInt(resourceViewType)];
		}

		const DescriptorHeapHandle& Get(uint32_t frameId, EResourceViewType resourceViewType) const
		{
			return handles[frameId][ToInt(resourceViewType)];
		}

		private:
		// TODO: Check if handle per frame is really needed after transition to bindless
		// TODO: This is ugly and hacky. Probably worth having separate handle list only for CBV_SRV_UAV
		std::array<std::array<DescriptorHeapHandle, ToInt(EResourceViewType::NUM_TYPES)>, gFrameCount> handles;
	};

	class GPUResource
	{
	public:
		GPUResource();
		// Alternate constructor when we only use 1 descriptor element, handle assignment to heap elements array internally instead of relying on the caller.
		GPUResource(ComPtr<ID3D12Resource> resource, const DescriptorHeapHandles& descriptorHeapHandles,
									 D3D12_RESOURCE_STATES usageState, const std::string& name);
		GPUResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES usageState, const std::string& name);
		GPUResource(GPUResource&& other) noexcept;
		GPUResource& operator=(GPUResource&& other) noexcept;
		virtual ~GPUResource();

		void SetupResourceStats();
		void AddDescriptorHandle(const DescriptorHeapHandle& heapHandle, EResourceViewType resourceViewType, uint32_t frameIdx, uint8_t mipIdx = 0);

		[[nodiscard]] ComPtr<ID3D12Resource> GetResource() const { return m_resource; }
		[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_GPUAddress; }
		[[nodiscard]] D3D12_RESOURCE_STATES GetUsageState() const { return m_usageState; }
		[[nodiscard]] const DescriptorHeapHandle& GetDescriptorHeapHandle(EResourceViewType resourceViewType, uint32_t frameIdx = 0, uint8_t mipIdx = 0) const;
		[[nodiscard]] uint32_t GetIndexInRDH(EResourceViewType resourceViewType = EResourceViewType::CBV, uint8_t mipIdx = 0) const;
		void SetUsageState(D3D12_RESOURCE_STATES usageState) { m_usageState = usageState; }

		[[nodiscard]] bool GetIsReady() const { return m_isReady; }
		void SetIsReady(bool isReady) { m_isReady = isReady; }


		// Debug
		[[nodiscard]] size_t GetSizeInBytes() const;

	protected:
		ComPtr<ID3D12Resource> m_resource;
		D3D12_GPU_VIRTUAL_ADDRESS m_GPUAddress;
		D3D12_RESOURCE_STATES m_usageState;
		bool m_isReady;

		DescriptorHeapHandles m_descriptorHeapHandles[gMaxMipsAllowed];

		// Debug
		size_t m_sizeInBytes;
		std::string m_name;
	};


	namespace GPUResourceUtils
	{
		ComPtr<ID3D12Resource> CreateUninitializedGPUBuffer(size_t alignedSize, LPCSTR name);
		
	}

	extern std::mutex g_ResourceCreationMutex;
}
