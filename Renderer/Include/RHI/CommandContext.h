#pragma once
#include <stdafx_renderer.h>
#include <RHI/DescriptorHeap.h>

#include <RHI/StagingDescriptorHeap.h>

using namespace Microsoft::WRL;
namespace PPK::RHI
{
	struct RenderContext
	{
		ComPtr<ID3D12GraphicsCommandList4> m_commandList;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12Resource> m_framebuffer;
		// other render context state or resources
	};

	class CommandContext
	{
	public:
		CommandContext(ComPtr<ID3D12Device4> device, UINT frameIndex);
		void BeginFrame(ComPtr<ID3D12CommandAllocator> commandAllocator, UINT frameIndex);
		void EndFrame(ComPtr<ID3D12CommandQueue> commandQueue) const;

		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList4> GetCurrentCommandList() const;
		// CAREFUL: if the command list is not closed, all commands recorded so far will be erased!
		void ResetCurrentCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator) const;
		// CAREFUL: if the command list is not closed, all commands recorded so far will be erased!
		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList4> ResetAndGetCurrentCommandList(ComPtr<ID3D12CommandAllocator> commandAllocator) const;

		[[nodiscard]] DescriptorHeapHandle GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetFramebufferDescriptorHandle() const;

	protected:
		std::shared_ptr<StagingDescriptorHeap> m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		ComPtr<ID3D12GraphicsCommandList4> m_commandList;
		UINT m_frameIndex;
		ComPtr<ID3D12Device4> m_device;
	};
}
