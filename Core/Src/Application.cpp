#include <Application.h>
#include <ApplicationHelper.h>
#include <stdafx.h>

using namespace PPK;

HWND Application::m_hwnd = nullptr;

Application::Application(UINT width, UINT height, std::wstring name) :
    m_name(name)
{
    // Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
	// This may happen if the application is launched through the PIX UI. 
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        // Load Pix GPU capturer
        LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str());
    }

    m_renderer = make_unique<Renderer>(width, height);
    m_scene = std::make_unique<Scene>(m_renderer);
}

void Application::OnInit(HWND hwnd)
{
    Logger::Info("Initializing Application...");

    m_hwnd = hwnd;
    m_renderer->OnInit(hwnd);

    // Load models
    Microsoft::glTF::Document document = GLTFReader::GetDocument("Models/Duck.gltf");

    // Generate scene form GLTF document
    m_scene->InitializeScene(document);

    Logger::Info("Application initialized successfully!");
}

void Application::OnUpdate()
{
}

void Application::OnRender()
{
    m_scene->OnRender();
    //m_renderer->OnRender();
}

void Application::OnDestroy()
{
    m_scene = nullptr;
    m_renderer->OnDestroy();
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