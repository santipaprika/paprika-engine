#pragma once

#include <Renderer.h>
#include <Scene.h>

namespace PPK
{
    class Application
    {
    public:
        Application(UINT width, UINT height, std::wstring name);
        ~Application() = default;

        void OnInit(HWND hwnd);
        void OnUpdate();
        void RenderImGui();
        void OnRender();
        void OnDestroy();

        void Resize(UINT width, UINT height);

        // Samples override the event handlers to handle specific messages.
        virtual void OnKeyDown(UINT8 /*key*/) {}
        virtual void OnKeyUp(UINT8 /*key*/) {}

        // Accessors.
        const WCHAR* GetTitle() const { return m_name.c_str(); }
        static HWND GetHwnd() { return m_hwnd; }

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

        // Scene
        std::unique_ptr<Scene> m_scene{};

        // Application time (used to get delta time, among others)
        double m_time;

        uint8_t m_bInitialized : 1;
        uint8_t m_bShouldResize : 1;

        UINT m_pendingResizeX;
        UINT m_pendingResizeY;
    };
}
