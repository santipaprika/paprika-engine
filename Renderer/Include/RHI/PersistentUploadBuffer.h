#pragma once

#include "GPUResource.h"


/**
 * Upload heap buffer that will be alive for the whole session to move dynamic data from CPU to GPU.
 * We should have as many as the number of frames in flight allowed. Currently 2 with 2MB for each.
 */
class PersistentUploadBuffer : PPK::RHI::GPUResource
{
public:
	PersistentUploadBuffer();

	// Set data to the upload buffer and return the index where the data was allocated
	uint32_t SetData(const D3D12_SUBRESOURCE_DATA& subresourceData, GPUResource* destResource);
	void ResetIndex();

	void* m_mappedBuffer;
    uint32_t m_firstFreeIndex;
	std::mutex m_updateResourceMutex;
};
