#pragma once

#include <array>
#include <RHI/DescriptorHeap.h>

#include "GPUResource.h"

namespace PPK::RHI
{
	enum class HeapLocation : uint8_t
	{
		TLAS = 0,
		VIEWS,
		OBJECTS,
		TEXTURES,

		NUM_LOCATIONS
	};

	constexpr std::array<uint32_t, static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS)> g_NumDescriptorsPerLocation =
	{
		1, // TLAS
		1, // Views
		5, // Objects
		10  // Shader Textures
	};

	// this can be used to index each location easily
	constexpr std::array<uint32_t, static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS)> g_NumDescriptorsPerLocationCum =
	{
		g_NumDescriptorsPerLocation[0], // TLAS
		g_NumDescriptorsPerLocation[0] + g_NumDescriptorsPerLocation[1], // Views
		g_NumDescriptorsPerLocation[0] + g_NumDescriptorsPerLocation[1] + g_NumDescriptorsPerLocation[2], // Objects
		g_NumDescriptorsPerLocation[0] + g_NumDescriptorsPerLocation[1] + g_NumDescriptorsPerLocation[2] + g_NumDescriptorsPerLocation[3]  // Shader Textures
	};

	class DescriptorHeapHandle;

	// Shader visible descriptor heap
	class ShaderDescriptorHeap : public DescriptorHeap
	{
	public:
		ShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptors);
		//~ShaderDescriptorHeap() final;

		void Reset();
		DescriptorHeapHandle GetHeapLocationHandle(HeapLocation heapLocation, uint32_t offsetInLocation = 0);
		D3D12_GPU_DESCRIPTOR_HANDLE GetHeapLocationGPUHandle(HeapLocation heapLocation, uint32_t offsetInLocation = 0) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetHeapLocationGPUHandle(uint32_t offset = 0) const;
		D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptors(GPUResource* resource, HeapLocation heapLocation);

	private:
		// Contains the start index of inlined buffer for each type of resource (TLAS, View, Object, Textures)
		std::array<uint32_t, static_cast<uint32_t>(HeapLocation::NUM_LOCATIONS)> m_currentDescriptorIndex;
	};
}
