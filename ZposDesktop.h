#pragma once

#ifndef ZPOSDESKTOP_H
#define ZPOSDESKTOP_H

#ifdef ZPOSDESKTOP_EXPORTS
#define ZPOSDESKTOP_API __declspec(dllexport)
#else
#define ZPOSDESKTOP_API __declspec(dllimport)
#endif

#include <windows.h>
#include <vector>
#include <utility>
#include <string>

// Desktop visibility states
enum class DesktopState
{
    UNKNOWN = 0,
    DESKTOP_VISIBLE = 1,
    WINDOWS_VISIBLE = 2
};

// Window positioning options
enum class WindowZPosition
{
    NORMAL = 0,
    TOPMOST = 1,
    BOTTOM = 2,
    DESKTOP = 3
};

// Main class for desktop Z-position management
class ZPOSDESKTOP_API CZposDesktop
{
public:
    CZposDesktop();
    ~CZposDesktop();

    // Initialize the desktop monitoring system
    bool Initialize(HWND targetWindow = nullptr);

    // Cleanup resources
    void Finalize();

    // Register a window to be managed
    bool RegisterWindow(HWND hwnd, WindowZPosition zpos = WindowZPosition::DESKTOP);

    // Unregister a window
    bool UnregisterWindow(HWND hwnd);

    // Set window Z-position behavior
    bool SetWindowZPosition(HWND hwnd, WindowZPosition zpos);

    // Get current desktop state
    DesktopState GetDesktopState() const;

    // Force update of desktop state
    void UpdateDesktopState();

    // Check if Show Desktop is active
    bool IsShowDesktopActive() const;

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

    bool CreateHelperWindow();
    void DestroyHelperWindow();
    bool SetupWindowsEventHook();
    void RemoveWindowsEventHook();

    HWND GetDefaultShellWindow();
    HWND GetDesktopIconsHostWindow();
    bool ShouldUseShellWindowAsDesktopIconsHost();
    bool BelongToSameProcess(HWND hwndA, HWND hwndB);
    bool CheckDesktopState();
    void UpdateWindowPositions();
    void PrepareHelperWindow();

    // Member variables
    HWND m_helperWindow;
    HWINEVENTHOOK m_winEventHook;
    DesktopState m_desktopState;
    std::vector<std::pair<HWND, WindowZPosition>> m_managedWindows;

    // Static instance for callbacks
    static CZposDesktop* s_instance;
};

// C-style exports for easier C# interop
extern "C" {
    ZPOSDESKTOP_API bool ZD_Initialize(HWND targetWindow);
    ZPOSDESKTOP_API void ZD_Finalize();
    ZPOSDESKTOP_API bool ZD_RegisterWindow(HWND hwnd, int zposition);
    ZPOSDESKTOP_API bool ZD_UnregisterWindow(HWND hwnd);
    ZPOSDESKTOP_API bool ZD_SetWindowZPosition(HWND hwnd, int zposition);
    ZPOSDESKTOP_API int ZD_GetDesktopState();
    ZPOSDESKTOP_API void ZD_UpdateDesktopState();
    ZPOSDESKTOP_API bool ZD_IsShowDesktopActive();
}

#endif // ZPOSDESKTOP_H