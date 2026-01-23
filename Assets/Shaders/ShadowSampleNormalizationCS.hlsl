#define EPS_FLOAT 1e-8
#define PI 3.14159265359

cbuffer CB0 : register(b0)
{
	// TOTAL: 3
	uint shadowSamplesScatterIndex : register(b0); // 0
	uint shadowRayTracingCommandBufferIndex : register(b0); // 1
	uint sampleMultiplier : register(b0); // 2
}

[numthreads(64,1,1)]
void MainCS(uint3 id : SV_DispatchThreadID, uint groupId : SV_GroupIndex)
{
	// Each lane processes 1 tile

	ByteAddressBuffer shadowRayTracingCommandBuffer = ResourceDescriptorHeap[shadowRayTracingCommandBufferIndex];
	uint numPenumbraTiles = shadowRayTracingCommandBuffer.Load(0);

	if (id.x >= numPenumbraTiles)
	{
		return;
	}

	RWByteAddressBuffer shadowSamplesScatterBuffer = ResourceDescriptorHeap[shadowSamplesScatterIndex];

	
	uint packedTile = shadowSamplesScatterBuffer.Load(id.x * 4);
	uint numSamples = packedTile & 0xFF;
	// For now only multiply, but ideally should accumulate samples from all tiles and normalize based on that
	uint normalizedNumSamples = min(0xFF, numSamples * sampleMultiplier);
	packedTile = (packedTile & ~0xFF) | normalizedNumSamples;
	shadowSamplesScatterBuffer.Store(id.x * 4, packedTile);
}
