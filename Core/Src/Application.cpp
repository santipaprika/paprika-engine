#include <Application.h>
#include <ApplicationHelper.h>
#include <GLTFReader.h>
#include <InputController.h>
#include <stdafx.h>
#include <Timer.h>
#include <imgui.h>
#include <ImGuiHelper.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

using namespace PPK;

HWND Application::m_hwnd = nullptr;

Application::Application(UINT width, UINT height, std::wstring name) :
    m_name(name)
{
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
	// This may happen if the application is launched through the PIX UI.
#ifdef PPK_USE_PIX
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        // Load Pix GPU capturer
        LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
    }
#endif

    gRenderer = new Renderer(width, height);
    m_scene = std::make_unique<Scene>();
}

void Application::OnInit(HWND hwnd)
{
    Logger::Info("Initializing Application...");

    m_time = Timer::GetApplicationTimeInSeconds();
    m_hwnd = hwnd;
    gRenderer->OnInit(hwnd);

    // Load models
    Microsoft::glTF::Document document = GLTFReader::GetDocument("Models/Sponza.gltf");

    // Generate scene form GLTF document
    m_scene->InitializeScene(document);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplWin32_Init(hwnd);

    // TODO: Find way to set safely in descriptor heap, either on its own, or in both frame heaps (now it's only on one)
    RHI::DescriptorHeapHandle ImGuiFontTextureHandle = gDescriptorHeapManager->GetNewShaderHeapBlockHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0, RHI::HeapLocation::TEXTURES);
    ImGui_ImplDX12_Init(gDevice.Get(), RHI::gFrameCount, gRenderer->GetSwapchainFormat(),
        gDescriptorHeapManager->GetShaderDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0)->GetHeap(),
        ImGuiFontTextureHandle.GetCPUHandle(),
        ImGuiFontTextureHandle.GetGPUHandle()
    );

    Logger::Info("Application initialized successfully!");
}

void Application::OnUpdate()
{
    const double deltaTime = Timer::GetApplicationTimeInSeconds() - m_time;
    m_time = Timer::GetApplicationTimeInSeconds();
    InputController::UpdateMouseMovement();

    m_scene->OnUpdate(deltaTime);
}

void Application::OnRender()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowMetricsWindow();

    bool open = true; // Ensure the window is open
    if (ImGui::Begin("PPK Settings", &open))
    {
        ImGui::Checkbox("VSync", &gVSync);
        ImGui::Checkbox("Denoise", &gDenoise);
        ImGui::SliderInt("RT Samples", &gPassManager->m_basePass.m_numSamples, 0, 100, "%d", ImGuiSliderFlags_Logarithmic);
    }
    ImGui::End();

    ShowGPUMemory();
    ShowProfilerWindow();

    m_scene->OnRender();
}

void Application::OnDestroy()
{
    // Make sure resource references for in-fly frames are freed
    gRenderer->WaitForAllGpuFrames();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_scene = nullptr;
    gRenderer->OnDestroy();
    delete gRenderer;
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_
void Application::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
    for (int i = 1; i < argc; ++i)
    {
        if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
            _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
        {
            m_useWarpDevice = true;
            m_name = m_name + L" (WARP)";
        }
    }
}