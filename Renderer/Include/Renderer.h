#pragma once

#include <stdafx.h>
#include <ShaderStructures.h>
#include <StepTimer.h>

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

namespace PPK
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Renderer
	{
	public:
        Renderer(UINT width, UINT height);
        void OnInit();
        void OnRender();
        void OnDestroy();

        // Accessors
        UINT GetWidth() const { return m_width; }
        UINT GetHeight() const { return m_height; }

	private:
        void GetHardwareAdapter(
            _In_ IDXGIFactory1* pFactory,
            _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
            bool requestHighPerformanceAdapter = false);

        struct Vertex
        {
	        DirectX::XMFLOAT3 position;
	        DirectX::XMFLOAT4 color;
        };

        // Viewport dimensions.
        UINT m_width;
        UINT m_height;
        float m_aspectRatio;

        static constexpr UINT FrameCount = 2;

        bool m_useWarpDevice;

        // Pipeline objects.
        CD3DX12_VIEWPORT m_viewport;
        CD3DX12_RECT m_scissorRect;
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize;

        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

        // Synchronization objects.
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        void LoadPipeline();
        void LoadAssets();
        void PopulateCommandList();
        void WaitForPreviousFrame();
	};
}

