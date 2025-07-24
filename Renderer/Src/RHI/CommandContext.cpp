#include <ApplicationHelper.h>
#include <Renderer.h>
#include <Timer.h>
#include <RHI/CommandContext.h>

namespace PPK::RHI
{
	CommandContext::CommandContext(UINT m_frameIndex)
		: m_frameIndex(m_frameIndex)
	{
		// Create the command list.
		ThrowIfFailed(gDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
			IID_PPV_ARGS(&m_commandList)));
	}

	void CommandContext::BeginFrame(ComPtr<ID3D12CommandAllocator> commandAllocator, UINT frameIndex)
	{
		m_frameIndex = frameIndex;

		// Reset to ensure clean recording state
		ResetCurrentCommandList(commandAllocator);
	}

	void CommandContext::EndFrame(ComPtr<ID3D12CommandQueue> commandQueue) const
	{
		{
			SCOPED_TIMER("CommandContext::EndFrame::2_CloseCommandList")
			m_commandList->Close();
		}

		{
			// This is slow if the GPU cost is high; triple buffer could help
			SCOPED_TIMER("CommandContext::EndFrame::3_ExecuteCommandLists")
			// Execute the command list.
			ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
			commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		}
	}

	ComPtr<ID3D12GraphicsCommandList4> CommandContext::GetCurrentCommandList() const
	{
		return m_commandList;
	}

	UINT CommandContext::GetFrameIndex() const
	{
		return m_frameIndex;
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
}
