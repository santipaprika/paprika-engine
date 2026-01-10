cbuffer CB0 : register(b0)
{
    uint shadowRayTracingCommandBufferIndex : register(b0); // 0
}

[numthreads(1,1,1)]
void MainCS()
{
    RWByteAddressBuffer shadowRayTracingCommandBuffer = ResourceDescriptorHeap[shadowRayTracingCommandBufferIndex];
    // Threadgroups will only grow in X direction. Y and Z constant 1.
    shadowRayTracingCommandBuffer.Store3(0, uint3(0, 1, 1));
}