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

#pragma once

#include <StreamReader.h>
#include <Renderer.h>

namespace PPK
{
    class Application
    {
    public:
        Application(UINT width, UINT height, std::wstring name);
        ~Application() = default;

        void OnInit(HWND hwnd);
        void OnUpdate();
        void OnRender();
        void OnDestroy();

        // Samples override the event handlers to handle specific messages.
        virtual void OnKeyDown(UINT8 /*key*/) {}
        virtual void OnKeyUp(UINT8 /*key*/) {}

        // Accessors.
        const WCHAR* GetTitle() const { return m_name.c_str(); }
        static HWND GetHwnd() { return m_hwnd; }
        UINT GetWidth() const { return m_renderer->GetWidth(); }
        UINT GetHeight() const { return m_renderer->GetHeight(); }

        void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

    protected:
        //void SetCustomWindowText(LPCWSTR text);

        // Adapter info.
        bool m_useWarpDevice;

    private:
        // Root assets path.
        std::wstring m_assetsPath;

        // Window title.
        std::wstring m_name;

        // Window handle
        static HWND m_hwnd;

        // Renderer
        std::unique_ptr<Renderer> m_renderer{};

        // GLTF Reader
        std::unique_ptr<GLTFReader> m_gltfReader{};
    };
}
