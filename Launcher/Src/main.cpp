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

#include <Application.h>
#include <imgui.h>
#include <InputController.h>
#include <windowsx.h>


extern "C" { __declspec(dllexport) extern const int D3D12SDKVersion = 610; }

extern "C" { __declspec(dllexport) extern const char8_t* D3D12SDKPath = u8".\\D3D12\\"; }
// extern "C" { __declspec(dllexport) extern const char8_t* DXCSDKPath = u8".\\DXC\\"; }

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main message handler for the sample.
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	PPK::Application* application = reinterpret_cast<PPK::Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (application /*&& application->OnEventReceived(hWnd, message, wParam, lParam)*/)
	{
		//return true;
	}

	switch (message)
	{
	case WM_CREATE:
		{
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
			InputController::Initialize();
		}
		return 0;

	case WM_PAINT:
		if (application)
		{
			application->OnUpdate();
			application->OnRender();
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	{
		WORD vkCode = LOWORD(wParam);                                 // virtual-key code
		InputController::SetKeyPressed(vkCode, message == WM_KEYDOWN);
		return 0;
	}

	case WM_MOUSEMOVE:
		InputController::SetMousePixelAfterOffset(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

int Run(PPK::Application* application, HINSTANCE hInstance, int nCmdShow)
{
	// Parse the command line parameters
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	application->ParseCommandLineArgs(argv, argc);
	LocalFree(argv);

	// Initialize the window class.
	WNDCLASSEXW windowClass = {0};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"ApplicationClass";
	RegisterClassExW(&windowClass);

	RECT windowRect = {0, 0, static_cast<LONG>(gRenderer->GetWidth()), static_cast<LONG>(gRenderer->GetHeight())};
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	HWND hwnd = CreateWindowW(
		windowClass.lpszClassName,
		application->GetTitle(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, // We have no parent window.
		nullptr, // We aren't using menus.
		hInstance,
		application);

	// Initialize the sample. OnInit is defined in each child-implementation of Application.
	application->OnInit(hwnd);

	ShowWindow(hwnd, nCmdShow);

	// Main sample loop.
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	application->OnDestroy();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

_Use_decl_annotations_

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	PPK::Application application(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, L"Paprika Toy Renderer");
	return Run(&application, hInstance, nCmdShow);
}
