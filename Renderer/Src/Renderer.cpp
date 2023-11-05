//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include <ApplicationHelper.h>
#include <Renderer.h>
#include <BackendUtils.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/VertexBuffer.h>

using namespace PPK;


DX12Interface::DX12Interface() :
    m_useWarpDevice(false)
{
    // Create DX12 device and swapchain
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug1> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(true);

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_12_1,
            IID_PPV_ARGS(&m_device)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(m_factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_12_1,
            IID_PPV_ARGS(&m_device)
        ));
    }

    m_instance = std::make_shared<DX12Interface>(*this);
}

std::shared_ptr<DX12Interface> DX12Interface::m_instance;

Renderer::Renderer(UINT width, UINT height) :
	m_width(width),
	m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_frameIndex(0),
	m_currentFenceValue(0),
	m_fenceValues{}
{
    // DX12Interface() and GPUResourceManager() implicitly constructed

    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

CD3DX12_RESOURCE_BARRIER Renderer::GetTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore,
	D3D12_RESOURCE_STATES stateAfter) const
{
    return CD3DX12_RESOURCE_BARRIER::Transition(resource, stateBefore, stateAfter);
}

CD3DX12_RESOURCE_BARRIER Renderer::GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) const
{
    return GetTransitionBarrier(m_renderTargets[m_frameIndex]->GetResource().Get(), stateBefore, stateAfter);
}

ComPtr<ID3D12GraphicsCommandList4> Renderer::GetCurrentCommandListReset()
{
    return m_commandContext->ResetAndGetCurrentCommandList(m_commandAllocators[m_frameIndex]);
}

std::shared_ptr<RHI::CommandContext> Renderer::GetCommandContext() const
{
    return m_commandContext;
}

void Renderer::OnInit(HWND hwnd)
{
    Logger::Info("Initializing Renderer...");

    m_hwnd = hwnd;
    LoadPipeline();
    LoadAssets();

    Logger::Info("Renderer initialized successfully!");
}

void Renderer::OnDestroy()
{
    // Release all resources
    WaitForGpu();
    for (uint32_t i = 0; i < FrameCount; i++)
    {
	    RHI::GPUResourceManager::Get()->FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	                                                   m_renderTargets[i]->GetDescriptorHeapHandle());
	    delete m_renderTargets[i];
    }
    GPUResourceManager::OnDestroy();
    Logger::Info("Released resources!");
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForGpu();

    CloseHandle(m_fenceEvent);
}

// Load the rendering pipeline dependencies.
void Renderer::LoadPipeline()
{
    Logger::Info("Loading rendering pipeline...");

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(m_factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Initialize descriptor heaps
    m_commandContext = std::make_shared<RHI::CommandContext>(m_device, m_frameIndex);


    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ComPtr<ID3D12Resource> renderTarget;
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTarget)));

            // Get new descriptor heap index
            const RHI::DescriptorHeapHandle rtvHandle = RHI::GPUResourceManager::Get()->GetNewHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            m_device->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle.GetCPUHandle());
            NAME_D3D12_OBJECT_NUMBERED_CUSTOM(renderTarget, Final_color, n);
            m_renderTargets[n] = new RHI::GPUResource(renderTarget, rtvHandle, D3D12_RESOURCE_STATE_RENDER_TARGET);

            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
        }
    }

	Logger::Info("Pipeline loaded successfully!");
}

// Load the sample assets.
void Renderer::LoadAssets()
{
    Logger::Info("Loading assets...");

    // RHI::ConstantBuffer* mBuffer1;
    // RHI::ConstantBuffer* mBuffer2;
    // RHI::DescriptorHeapHandle cbvBlockStart = cbvHeap->GetHeapHandleBlock(2);
    // D3D12_CPU_DESCRIPTOR_HANDLE currentCBVHandle = cbvBlockStart.GetCPUHandle();
    // uint32 cbvDescriptorSize = cbvHeap->GetDescriptorSize();
    //
    // device->CopyDescriptorsSimple(1, currentCBVHandle, mBuffer1->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    //
    // currentCBVHandle.ptr += cbvDescriptorSize;
    //
    // device->CopyDescriptorsSimple(1, currentCBVHandle, mBuffer2->GetConstantBufferViewHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    // commandList->SetGraphicsRootDescriptorTable([index], cbvBlockStart->GetGPUHandle());

    Logger::Info("Assets loaded successfully!");
}

// Wait for pending GPU work to complete.
void Renderer::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_frameIndex] = ++m_currentFenceValue;
}

void Renderer::ExecuteCommandListOnce()
{
    // Close the command list and execute it to begin the vertex buffer copy into
    // the default heap.
    ID3D12CommandList* ppCommandLists[] = { m_commandContext->GetCurrentCommandList().Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(m_fence.GetAddressOf())));
        m_fenceValues[m_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }
}

void Renderer::BeginFrame()
{
    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the allocator for the current frame not ready to be filled yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for this frame.
    m_fenceValues[m_frameIndex] = ++m_currentFenceValue;

    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    m_commandContext->BeginFrame(m_commandAllocators[m_frameIndex], m_frameIndex);
}

void Renderer::EndFrame()
{
    // Close the command list
    m_commandContext->EndFrame(m_commandQueue);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue));
}