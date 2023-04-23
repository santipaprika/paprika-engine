#include <RHI/CommandContext.h>

namespace PPK::RHI
{
	CommandContext::CommandContext(ComPtr<ID3D12Device4> device, UINT m_frameIndex)
		: m_device(device), m_frameIndex(m_frameIndex)
	{
		// Create the command list.
		ThrowIfFailed(m_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAGS::D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&m_commandList)));

		for (UINT descriptorHeapType = 0; descriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; descriptorHeapType++)
		{
			m_currentDescriptorHeaps[descriptorHeapType] = std::make_shared<StagingDescriptorHeap>(device.Get(), static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(descriptorHeapType), 2);
		}
	}

	void CommandContext::BeginFrame(ComPtr<ID3D12CommandAllocator> commandAllocator, UINT frameIndex)
	{
		m_frameIndex = frameIndex;

		// Reset to ensure clean recording state
		ResetCurrentCommandList(commandAllocator);
	}

	void CommandContext::EndFrame(ComPtr<ID3D12CommandQueue> commandQueue) const
	{
		m_commandList->Close();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	DescriptorHeapHandle CommandContext::GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		return m_currentDescriptorHeaps[heapType]->GetNewHeapHandle();
	}

	ComPtr<ID3D12GraphicsCommandList4> CommandContext::GetCurrentCommandList() const
	{
		return m_commandList;
	}

	void CommandContext::ResetCurrentCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator) const
	{
		ThrowIfFailed(m_commandList->Reset(commandAllocator.Get(), nullptr));
	}

	ComPtr<ID3D12GraphicsCommandList4> CommandContext::ResetAndGetCurrentCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator) const
	{
		ResetCurrentCommandList(commandAllocator);
		return m_commandList;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE CommandContext::GetFramebufferDescriptorHandle() const
	{
		return m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetHeapCPUAtIndex(m_frameIndex);
	}
}
