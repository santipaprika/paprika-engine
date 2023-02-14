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

#include <stdafx.h>
#include <Application.h>

using namespace Microsoft::WRL;
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
}

void Application::OnInit(HWND hwnd)
{
    Logger::Info("Initializing Application...");

    m_hwnd = hwnd;
    m_renderer->OnInit();

    Logger::Info("Application initialized successfully!");
}

void Application::OnUpdate()
{
}

void Application::OnRender()
{
    m_renderer->OnRender();
}

void Application::OnDestroy()
{
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