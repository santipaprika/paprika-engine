#pragma once

#include <span>

#include "GPUResource.h"

#define MAX_TEXTURE_SUBRESOURCE_COUNT 6

struct ResourceUpdateArgs
{
	UINT64 m_memorySize;
	UINT64 m_rowSizesInBytes[MAX_TEXTURE_SUBRESOURCE_COUNT];
	uint32_t m_numSubresources;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_layouts[MAX_TEXTURE_SUBRESOURCE_COUNT];
	UINT m_numRows[MAX_TEXTURE_SUBRESOURCE_COUNT];

	std::span<D3D12_SUBRESOURCE_DATA> m_srcData;	
	PPK::RHI::GPUResource* m_destResource;
};

/**
 * Upload heap buffer that will be alive for the whole session to move dynamic data from CPU to GPU.
 * We should have as many as the number of frames in flight allowed. Currently 2 with 2MB for each.
 */
class PersistentUploadBuffer : PPK::RHI::GPUResource
{
public:
	PersistentUploadBuffer(uint32_t frameIdx);

	// Set data to the upload buffer and return the index where the data was allocated
	uint32_t SetData(ResourceUpdateArgs& updateArgs);
	uint32_t SetData(const D3D12_SUBRESOURCE_DATA& subresourceData, GPUResource* destResource);
	void ResetIndex();

    uint32_t m_firstFreeIndex;
	std::mutex m_updateResourceMutex;
};
