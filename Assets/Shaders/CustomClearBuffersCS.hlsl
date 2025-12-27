cbuffer CB0 : register(b0)
{
    uint shadowSamplesScatterIndex : register(b0); // 0
}

[numthreads(1,1,1)]
void MainCS()
{
    RWByteAddressBuffer shadowSamplesScatterBuffer = ResourceDescriptorHeap[shadowSamplesScatterIndex];

    // We only need to clear the 'count' element. Elements at position i < 'count' will be overwritten, and i > 'count' won't be used
    shadowSamplesScatterBuffer.Store(0, 0);
}