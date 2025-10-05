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
#include <stdafx_renderer.h>
#include <RHI/ConstantBuffer.h>
#include <RHI/DescriptorHeapElement.h>
#include <dxcapi.h>
#include <Timer.h>

using namespace PPK;
Renderer* gRenderer;
ComPtr<ID3D12Device5> gDevice;
ComPtr<IDXGIFactory4> gFactory;
RHI::DescriptorHeapManager* gDescriptorHeapManager;

// Global variables for runtime manipulation
uint32_t gTotalFrameIndex = 0;
bool gVSync = false;
extern bool gMSAA = true;
extern bool gDenoise = true;
extern uint32_t gMSAACount = 4.f;
bool gSmartSampleAllocation = true;

// This doesn't have ownership over anything! Careful when accessing
// TODO: Maybe should be weak ptrs?
std::unordered_map<std::string, PPK::RHI::GPUResource*> gResourcesMap;

void InitializeDeviceFactory(bool useWarpDevice = false)
{
    // Create DX12 device and swapchain
    UINT dxgiFactoryFlags = 0;

#ifdef PPK_D3D_DEBUG_LAYER
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

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&gFactory)));

    if (useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(gFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_12_1,
            IID_PPV_ARGS(&gDevice)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(gFactory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_12_1,
            IID_PPV_ARGS(&gDevice)
        ));

#ifdef PPK_D3D_DEBUG_LAYER
        // Filter out verbose warnings from debug layer
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(gDevice->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            D3D12_MESSAGE_ID disabledMessages[] = {
                D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE // Skip clear mismatch warnings when depth resource is typeless (can't provide default clear value then)
            };
            D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(disabledMessages);
            filter.DenyList.pIDList = disabledMessages;
            filter.DenyList.NumSeverities = _countof(severities);
            filter.DenyList.pSeverityList = severities;
            infoQueue->PushStorageFilter(&filter);
        }
#endif
    }

    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
}

Renderer::Renderer(UINT width, UINT height) :
	m_width(width),
	m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0l, 0l, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_frameIndex(0),
	m_currentFenceValue(0),
	m_fenceValues{}
{
    // Maybe this should go to another core engine file
    gMainThreadId = std::this_thread::get_id();
    
    // DX12Interface() and DescriptorHeapManager() implicitly constructed
    InitializeDeviceFactory();
    gDescriptorHeapManager = new RHI::DescriptorHeapManager();
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featuresSupported;
    ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featuresSupported, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5)));
    if (featuresSupported.RaytracingTier == 0)
    {
        Logger::Error("Current GPU doesn't support hardware raytracing");
    }

    D3D12_FEATURE_DATA_SHADER_MODEL shaderModelSupported = {};
    shaderModelSupported.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;
    ThrowIfFailed(gDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModelSupported, sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)));
    if (shaderModelSupported.HighestShaderModel < D3D_SHADER_MODEL_6_0)
    {
        Logger::Error("Current GPU doesn't support shader model 6_0");
    }

    ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
}

Renderer::~Renderer()
{
#ifdef PPK_D3D_DEBUG_LAYER
    ComPtr<IDXGIDebug1> dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiDebug.GetAddressOf()))))
    {
        dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
#endif
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

DXGI_FORMAT Renderer::GetSwapchainFormat() const
{
    DXGI_SWAP_CHAIN_DESC1 desc;
    m_swapChain->GetDesc1(&desc);
    return desc.Format;
}

void Renderer::CompileShader(const wchar_t* shaderPath, const wchar_t* entryPoint, const wchar_t* targetProfile, IDxcBlob** outCode) const
{
#ifdef PPK_DEBUG_SHADERS
    // Enable better shader debugging with the graphics debugging tools.
    // TODO: Handle stripping debug and reflection blobs
    const wchar_t* arguments[] = {
#ifndef PPK_PROFILE_SHADERS
        L"-Od",
#endif
        L"-Zi"
    }; // Debug + skip optimization
#endif

    uint32_t codePage = CP_UTF8;
    IDxcBlobEncoding* shaderSourceBlob;
    ThrowIfFailed(library->CreateBlobFromFile(GetAssetFullPath(shaderPath).c_str(), &codePage, &shaderSourceBlob));

    IDxcOperationResult* result;
    HRESULT hr = compiler->Compile(
        shaderSourceBlob, // pSource
        shaderPath, // pSourceName
        entryPoint, // pEntryPoint
        targetProfile, // pTargetProfile
#ifdef PPK_DEBUG_SHADERS
        arguments, _countof(arguments), // pArguments, argCount
#else
        nullptr, 0,
#endif
        NULL, 0, // pDefines, defineCount
        NULL, // pIncludeHandler
        &result); // ppResult
    if(SUCCEEDED(hr))
        result->GetStatus(&hr);
    if(FAILED(hr))
    {
        if(result)
        {
            IDxcBlobEncoding* errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if(SUCCEEDED(hr) && errorsBlob)
            {
                Logger::Error((const char*)errorsBlob->GetBufferPointer());
            }
        }
    }

    result->GetResult(outCode);
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
	// Make sure resource references for in-fly frames are freed
    WaitForAllGpuFrames();

    // Release all resources
    for (uint32_t i = 0; i < RHI::gFrameCount; i++)
    {
	    //gDescriptorHeapManager->FreeDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	    //                                               m_renderTargets[i]->GetDescriptorHeapHandle());
	    delete m_renderTargets[i];
    }

    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForGpu();

    gDescriptorHeapManager->OnDestroy();
    Logger::Info("Released resources!");

    // Make sure fence references for in-fly frames are freed
    WaitForAllGpuFrames();

    CloseHandle(m_fenceEvent);

    delete gDescriptorHeapManager;

	// Pipeline objects
    m_rootSignature.Reset();
    m_pipelineState.Reset();
    m_commandContext = nullptr; // command lists are removed here
    for (auto& commandAllocator : m_commandAllocators)
    {
        commandAllocator.Reset();
    }
    m_commandQueue.Reset();
    m_swapChain.Reset();
    m_fence.Reset();

    gFactory.Reset();
    gDevice.Reset();
}

// Load the rendering pipeline dependencies.
void Renderer::LoadPipeline()
{
    Logger::Info("Loading rendering pipeline...");

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(gDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = RHI::gFrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(gFactory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(gFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Initialize descriptor heaps
    m_commandContext = std::make_shared<RHI::CommandContext>(m_frameIndex);

    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (UINT n = 0; n < RHI::gFrameCount; n++)
        {
            ComPtr<ID3D12Resource> renderTarget;
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTarget)));

            // Get new descriptor heap index
            const std::shared_ptr<RHI::DescriptorHeapElement> rtvHeapElement = std::make_shared<RHI::DescriptorHeapElement>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            gDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHeapElement->GetCPUHandle());
            LPCSTR name = "SwapchainOutput";
            NAME_D3D12_OBJECT_NUMBERED_CUSTOM(renderTarget, name, n);
            m_renderTargets[n] = new RHI::GPUResource(renderTarget, rtvHeapElement, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      (std::string(name) + "[" + std::to_string(n) + "]").c_str());

            ThrowIfFailed(gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
        }

        // m_commandContext->ResetCurrentCommandList(m_commandAllocators[m_frameIndex]);
        // Create DSV
        {
            //ComPtr<ID3D12Resource> renderTarget;
            //ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTarget)));


            //// Get new descriptor heap index
            //const RHI::DescriptorHeapHandle rtvHandle = gDescriptorHeapManager->GetNewStagingHeapHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            //gDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle.GetCPUHandle());
            //NAME_D3D12_OBJECT_NUMBERED_CUSTOM(renderTarget, Final_color, n);
            //m_renderTargets[n] = new RHI::GPUResource(renderTarget, rtvHandle, D3D12_RESOURCE_STATE_RENDER_TARGET);

            //ThrowIfFailed(gDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
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

void Renderer::TransitionResources(ComPtr<ID3D12GraphicsCommandList4> commandList,
    TransitionsList transitionsList)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(transitionsList.size());

    for (auto& [resource, destState] : transitionsList)
    {
        if (destState != resource->GetUsageState())
        {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource->GetResource().Get(), resource->GetUsageState(), destState));
            resource->SetUsageState(destState);
        }
    }

    if (barriers.size() > 0)
    {
        commandList->ResourceBarrier(barriers.size(), barriers.data());
    }
}

void Renderer::WaitForAllGpuFrames()
{
    for (m_frameIndex = 0; m_frameIndex < RHI::gFrameCount; m_frameIndex++)
    {
        WaitForGpu();
    }
}

void Renderer::ExecuteCommandListOnce()
{
    // Close the command list and execute it to begin the vertex buffer copy into
    // the default heap.
    ID3D12CommandList* ppCommandLists[] = { m_commandContext->GetCurrentCommandList().Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(gDevice->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&m_fence)));
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
    gTotalFrameIndex++;

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

    {
        // Indicate that the back buffer will be used as a render target.
        const CD3DX12_RESOURCE_BARRIER framebufferBarrier = gRenderer->GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandContext->GetCurrentCommandList()->ResourceBarrier(1, &framebufferBarrier);
    }
}

void Renderer::EndFrame()
{
    {
        SCOPED_TIMER("Renderer::EndFrame::1_FramebufferBarrier")
        // Indicate that the back buffer will now be used to present.
        const CD3DX12_RESOURCE_BARRIER framebufferBarrier = gRenderer->GetFramebufferTransitionBarrier(D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandContext->GetCurrentCommandList()->ResourceBarrier(1, &framebufferBarrier);
    }

    // Close the command list
    m_commandContext->EndFrame(m_commandQueue);

    {
        SCOPED_TIMER("Renderer::EndFrame::4_Present")
        
        // Present the frame.
        ThrowIfFailed(m_swapChain->Present(gVSync, gVSync ? 0 : DXGI_PRESENT_ALLOW_TEARING ));
    }

    {
        SCOPED_TIMER("Renderer::EndFrame::5_CommandQueueSignal")
        // Schedule a Signal command in the queue.
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue));
    }
}

PPK::RHI::GPUResource* GetGlobalGPUResource(const std::string& resourceName)
{
    RHI::GPUResource* resource = gResourcesMap[resourceName];
    Logger::Assert(resource);

    return resource;
}
