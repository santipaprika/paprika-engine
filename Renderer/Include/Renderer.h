#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h>

#include <RHI/CommandContext.h>

// Passes
#include <RHI/DescriptorHeapManager.h>
#include <RHI/GPUResource.h>
#include <unordered_map>

struct IDxcOperationResult;
struct IDxcCompiler;
struct IDxcLibrary;
struct IDxcBlob;
// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

namespace PPK
{
    class Pass;

	// This sample renderer instantiates a basic rendering pipeline.
    class Renderer
	{
	public:
        Renderer(UINT width, UINT height);
        ~Renderer();
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

        [[nodiscard]] DXGI_FORMAT GetSwapchainFormat() const;

    	void CompileShader(const wchar_t* shaderPath, const wchar_t* entryPoint, const wchar_t* targetProfile, IDxcBlob** outCode) const;

    	// Execute the recorded commands and wait for these to be completed
        void ExecuteCommandListOnce();

        void BeginFrame();
        void EndFrame();

        void WaitForAllGpuFrames();

	private:        // Viewport dimensions.
    	UINT m_width;
        UINT m_height;
        float m_aspectRatio;

        // Output swapchain buffers
        RHI::GPUResource* m_renderTargets[RHI::gFrameCount];

        // Pipeline objects.
        CD3DX12_VIEWPORT m_viewport;
        CD3DX12_RECT m_scissorRect;
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12CommandAllocator> m_commandAllocators[RHI::gFrameCount];
        std::shared_ptr<RHI::CommandContext> m_commandContext;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;

        ComPtr<IDxcLibrary> library;
        ComPtr<IDxcCompiler> compiler;

        // Synchronization objects.
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValues[RHI::gFrameCount];
        UINT64 m_currentFenceValue;

        void LoadPipeline();
        void LoadAssets();
        void WaitForGpu();

        // Window handle
        HWND m_hwnd;
	};

}

// TODO: Should abstract these to another header? Otherwise all the previous bloat is carried over
extern ComPtr<ID3D12Device5> gDevice;
extern ComPtr<IDXGIFactory4> gFactory;
extern PPK::Renderer* gRenderer;
extern std::unordered_map<std::wstring, PPK::RHI::GPUResource*> gResourcesMap; // probably can be vec<enum> instead of map<char*> 
extern PPK::RHI::DescriptorHeapManager* gDescriptorHeapManager;
extern bool gVSync;


