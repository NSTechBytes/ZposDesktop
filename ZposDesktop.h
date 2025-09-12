#pragma once

#ifdef ZPOSDESKTOP_EXPORTS
#define ZPOSDESKTOP_API __declspec(dllexport)
#else
#define ZPOSDESKTOP_API __declspec(dllimport)
#endif

// Desktop state
enum class DesktopState
{
    ShowingWindows = 0,
    ShowingDesktop = 1
};

// Callback for desktop state changes
typedef void(__stdcall* DesktopStateCallback)(DesktopState state);

class ZPOSDESKTOP_API CZposDesktop
{
public:
    CZposDesktop(void);
    ~CZposDesktop(void);

    // Initialize the desktop manager
    bool Initialize(HINSTANCE hInstance);

    // Cleanup resources
    void Finalize();

    // Register a window to stay visible on desktop
    bool RegisterWindow(HWND hwnd);

    // Unregister a window
    bool UnregisterWindow(HWND hwnd);

    // Get current desktop state
    DesktopState GetDesktopState() const;

    // Set callback for desktop state changes
    void SetDesktopStateCallback(DesktopStateCallback callback);

    // Force refresh of all managed windows
    void RefreshWindowPositions();

    // Check if a window is registered
    bool IsWindowRegistered(HWND hwnd) const;

private:
    class Impl;
    Impl* m_pImpl;
};

// C-style exports for easier P/Invoke
extern "C"
{
    ZPOSDESKTOP_API bool __stdcall ZD_Initialize(HINSTANCE hInstance);
    ZPOSDESKTOP_API void __stdcall ZD_Finalize();
    ZPOSDESKTOP_API bool __stdcall ZD_RegisterWindow(HWND hwnd);
    ZPOSDESKTOP_API bool __stdcall ZD_UnregisterWindow(HWND hwnd);
    ZPOSDESKTOP_API int __stdcall ZD_GetDesktopState();
    ZPOSDESKTOP_API void __stdcall ZD_SetDesktopStateCallback(DesktopStateCallback callback);
    ZPOSDESKTOP_API void __stdcall ZD_RefreshWindowPositions();
    ZPOSDESKTOP_API bool __stdcall ZD_IsWindowRegistered(HWND hwnd);
}