#pragma once

#include <stdafx_renderer.h>
#include <ShaderStructures.h>
#include <StepTimer.h>
#include <RHI/CommandContext.h>

// Passes
#include <Passes/Pass.h>
#include <RHI/GPUResourceManager.h>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

namespace PPK
{
    class Pass;

    class DX12Interface
    {
    public:
        DX12Interface();
        ~DX12Interface();
        void OnDestroy();
        [[nodiscard]] static std::shared_ptr<DX12Interface> Get() { return m_instance; };
        [[nodiscard]] ComPtr<ID3D12Device4> GetDevice() { return m_device; }

        ComPtr<ID3D12Device4> m_device;
        ComPtr<IDXGIFactory4> m_factory;
        bool m_useWarpDevice;

    private:
        static std::shared_ptr<DX12Interface> m_instance;
    };

	// This sample renderer instantiates a basic rendering pipeline.
    class Renderer : DX12Interface, RHI::GPUResourceManager
	{
	public:
        Renderer(UINT width, UINT height);
        void OnInit(HWND hwnd);
        //void OnRender();
        void OnDestroy();

        // Accessors
        [[nodiscard]] UINT GetWidth() const { return m_width; }
        [[nodiscard]] UINT GetHeight() const { return m_height; }
        [[nodiscard]] UINT GetAspectRatio() const { return m_aspectRatio; }
        [[nodiscard]] CD3DX12_VIEWPORT GetViewport() const { return m_viewport; }
        [[nodiscard]] CD3DX12_RECT GetScissorRect() const { return m_scissorRect; }

        [[nodiscard]] CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvDescriptorHandle() const;
        [[nodiscard]] CD3DX12_RESOURCE_BARRIER GetTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) const;
        [[nodiscard]] CD3DX12_RESOURCE_BARRIER GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) const;

        // Reset current frame's command list leaving it in recording state
		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList4> GetCurrentCommandListReset();
        [[nodiscard]] std::shared_ptr<RHI::CommandContext> GetCommandContext() const;

        // Execute the recorded commands and wait for these to be completed
        void ExecuteCommandListOnce();

        void BeginFrame();
        void EndFrame();

	private:
        // Viewport dimensions.
        UINT m_width;
        UINT m_height;
        float m_aspectRatio;

        // Output swapchain buffers
        RHI::GPUResource* m_renderTargets[FrameCount];

        // Pipeline objects.
        CD3DX12_VIEWPORT m_viewport;
        CD3DX12_RECT m_scissorRect;
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
        std::shared_ptr<RHI::CommandContext> m_commandContext;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;

        // Synchronization objects.
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValues[FrameCount];
        UINT64 m_currentFenceValue;

        void LoadPipeline();
        void LoadAssets();
        void WaitForGpu();
        void WaitForAllGpuFrames();

        // Window handle
        HWND m_hwnd;
	};

}

extern PPK::Renderer* gRenderer;


