#include "pch.h"
#include "framework.h"
#include "ZposDesktop.h"
#include <map>
#include <vector>
#include <memory>

#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

enum TIMER
{
    TIMER_SHOWDESKTOP = 1,
    TIMER_RESUME = 2
};

enum INTERVAL
{
    INTERVAL_SHOWDESKTOP = 250,
    INTERVAL_RESTOREWINDOWS = 100,
    INTERVAL_RESUME = 1000
};

struct WindowInfo
{
    HWND hwnd;
    bool isVisible;
};

class CZposDesktop::Impl
{
public:
    Impl() :
        m_hInstance(nullptr),
        m_hSystemWindow(nullptr),
        m_hHelperWindow(nullptr),
        m_hWinEventHook(nullptr),
        m_showDesktop(false),
        m_callback(nullptr)
    {
    }

    ~Impl()
    {
        Finalize();
    }

    bool Initialize(HINSTANCE hInstance);
    void Finalize();
    bool RegisterWindow(HWND hwnd);
    bool UnregisterWindow(HWND hwnd);
    DesktopState GetDesktopState() const;
    void SetDesktopStateCallback(DesktopStateCallback callback);
    void RefreshWindowPositions();
    bool IsWindowRegistered(HWND hwnd) const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
        LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

    HWND GetDefaultShellWindow();
    HWND GetDesktopIconsHostWindow();
    bool ShouldUseShellWindowAsDesktopIconsHost();
    bool BelongToSameProcess(HWND hwndA, HWND hwndB);
    void PrepareHelperWindow(HWND desktopIconsHostWindow);
    bool CheckDesktopState(HWND desktopIconsHostWindow);
    void PositionWindowsOnDesktop();

    HINSTANCE m_hInstance;
    HWND m_hSystemWindow;
    HWND m_hHelperWindow;
    HWINEVENTHOOK m_hWinEventHook;
    bool m_showDesktop;
    DesktopStateCallback m_callback;
    std::map<HWND, WindowInfo> m_windows;

    static Impl* s_instance;
};

CZposDesktop::Impl* CZposDesktop::Impl::s_instance = nullptr;

bool CZposDesktop::Impl::Initialize(HINSTANCE hInstance)
{
    if (m_hInstance != nullptr)
        return false; // Already initialized

    m_hInstance = hInstance;
    s_instance = this;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ZposDesktopSystem";
    ATOM className = RegisterClass(&wc);

    m_hSystemWindow = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        MAKEINTATOM(className),
        L"ZposSystem",
        WS_POPUP | WS_DISABLED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    m_hHelperWindow = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        MAKEINTATOM(className),
        L"ZposPositioningHelper",
        WS_POPUP | WS_DISABLED,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr);

    if (!m_hSystemWindow || !m_hHelperWindow)
        return false;

    SetWindowPos(m_hSystemWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
    SetWindowPos(m_hHelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

    m_hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        nullptr,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    SetTimer(m_hSystemWindow, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);

    return true;
}

void CZposDesktop::Impl::Finalize()
{
    if (m_hSystemWindow)
    {
        KillTimer(m_hSystemWindow, TIMER_SHOWDESKTOP);
        KillTimer(m_hSystemWindow, TIMER_RESUME);
    }

    if (m_hWinEventHook)
    {
        UnhookWinEvent(m_hWinEventHook);
        m_hWinEventHook = nullptr;
    }

    if (m_hHelperWindow)
    {
        DestroyWindow(m_hHelperWindow);
        m_hHelperWindow = nullptr;
    }

    if (m_hSystemWindow)
    {
        DestroyWindow(m_hSystemWindow);
        m_hSystemWindow = nullptr;
    }

    m_windows.clear();
    s_instance = nullptr;
    m_hInstance = nullptr;
}

bool CZposDesktop::Impl::RegisterWindow(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return false;

    WindowInfo info;
    info.hwnd = hwnd;
    info.isVisible = IsWindowVisible(hwnd);

    m_windows[hwnd] = info;
    RefreshWindowPositions();
    return true;
}

bool CZposDesktop::Impl::UnregisterWindow(HWND hwnd)
{
    auto it = m_windows.find(hwnd);
    if (it != m_windows.end())
    {
        m_windows.erase(it);
        return true;
    }
    return false;
}

DesktopState CZposDesktop::Impl::GetDesktopState() const
{
    return m_showDesktop ? DesktopState::ShowingDesktop : DesktopState::ShowingWindows;
}

void CZposDesktop::Impl::SetDesktopStateCallback(DesktopStateCallback callback)
{
    m_callback = callback;
}

void CZposDesktop::Impl::RefreshWindowPositions()
{
    PositionWindowsOnDesktop();
}

bool CZposDesktop::Impl::IsWindowRegistered(HWND hwnd) const
{
    return m_windows.find(hwnd) != m_windows.end();
}

HWND CZposDesktop::Impl::GetDefaultShellWindow()
{
    static HWND s_shellW = nullptr;
    HWND shellW = GetShellWindow();

    if (shellW)
    {
        if (shellW == s_shellW)
        {
            return shellW;
        }
        else
        {
            const int classLen = 8;
            WCHAR className[classLen];
            if (!(GetClassName(shellW, className, classLen) > 0 &&
                wcscmp(className, L"Progman") == 0))
            {
                shellW = nullptr;
            }
        }
    }

    s_shellW = shellW;
    return shellW;
}

bool CZposDesktop::Impl::ShouldUseShellWindowAsDesktopIconsHost()
{
    // Check for Windows 11 24H2+ by looking for GetCurrentMonitorTopologyId function
    static bool checked = false;
    static bool result = false;

    if (!checked)
    {
        result = GetProcAddress(GetModuleHandle(L"user32"), "GetCurrentMonitorTopologyId") != nullptr;
        checked = true;
    }

    return result;
}

HWND CZposDesktop::Impl::GetDesktopIconsHostWindow()
{
    static HWND s_defView = nullptr;
    HWND shellW = GetDefaultShellWindow();
    if (!shellW) return nullptr;

    if (ShouldUseShellWindowAsDesktopIconsHost())
    {
        if (FindWindowEx(shellW, nullptr, L"SHELLDLL_DefView", L""))
        {
            return shellW;
        }
        return nullptr;
    }

    if (s_defView && IsWindow(s_defView))
    {
        HWND parent = GetAncestor(s_defView, GA_PARENT);
        if (parent)
        {
            if (parent == shellW)
            {
                return nullptr;
            }
            else
            {
                const int classLen = 8;
                WCHAR className[classLen];
                if (GetClassName(parent, className, classLen) > 0 &&
                    wcscmp(className, L"WorkerW") == 0)
                {
                    return parent;
                }
            }
        }
    }

    HWND workerW = nullptr;
    HWND defView = FindWindowEx(shellW, nullptr, L"SHELLDLL_DefView", L"");
    if (defView == nullptr)
    {
        while (workerW = FindWindowEx(nullptr, workerW, L"WorkerW", L""))
        {
            if (IsWindowVisible(workerW) &&
                BelongToSameProcess(shellW, workerW) &&
                (defView = FindWindowEx(workerW, nullptr, L"SHELLDLL_DefView", L"")))
            {
                break;
            }
        }
    }

    s_defView = defView;
    return workerW;
}

bool CZposDesktop::Impl::BelongToSameProcess(HWND hwndA, HWND hwndB)
{
    DWORD procAId = 0, procBId = 0;
    GetWindowThreadProcessId(hwndA, &procAId);
    GetWindowThreadProcessId(hwndB, &procBId);
    return (procAId == procBId);
}

void CZposDesktop::Impl::PrepareHelperWindow(HWND desktopIconsHostWindow)
{
    SetWindowPos(m_hSystemWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

    if (m_showDesktop && desktopIconsHostWindow)
    {
        SetWindowPos(m_hHelperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);

        HWND hwnd = desktopIconsHostWindow;
        while (hwnd = ::GetNextWindow(hwnd, GW_HWNDPREV))
        {
            if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
            {
                if (0 != SetWindowPos(m_hHelperWindow, hwnd, 0, 0, 0, 0, ZPOS_FLAGS))
                {
                    return;
                }
            }
        }
    }
    else
    {
        SetWindowPos(m_hHelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
    }
}

bool CZposDesktop::Impl::CheckDesktopState(HWND desktopIconsHostWindow)
{
    HWND hwnd = nullptr;

    if (desktopIconsHostWindow && IsWindowVisible(desktopIconsHostWindow))
    {
        hwnd = FindWindowEx(nullptr, desktopIconsHostWindow, L"ZposDesktopSystem", L"ZposSystem");
    }

    bool stateChanged = (hwnd && !m_showDesktop) || (!hwnd && m_showDesktop);

    if (stateChanged)
    {
        m_showDesktop = !m_showDesktop;

        PrepareHelperWindow(desktopIconsHostWindow);
        PositionWindowsOnDesktop();

        if (m_callback)
        {
            m_callback(GetDesktopState());
        }

        if (m_showDesktop)
        {
            SetTimer(m_hSystemWindow, TIMER_SHOWDESKTOP, INTERVAL_RESTOREWINDOWS, nullptr);
        }
        else
        {
            SetTimer(m_hSystemWindow, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);
        }
    }

    return stateChanged;
}

void CZposDesktop::Impl::PositionWindowsOnDesktop()
{
    // Position all registered windows to stay visible on desktop
    for (auto& pair : m_windows)
    {
        WindowInfo& info = pair.second;

        if (m_showDesktop)
        {
            // When desktop is showing, position window above desktop but below topmost
            SetWindowPos(info.hwnd, m_hHelperWindow, 0, 0, 0, 0, ZPOS_FLAGS);
        }
        else
        {
            // When windows are showing, position at bottom
            SetWindowPos(info.hwnd, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
        }
    }
}

LRESULT CALLBACK CZposDesktop::Impl::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!s_instance)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    if (hWnd != s_instance->m_hSystemWindow)
    {
        if (uMsg == WM_WINDOWPOSCHANGING)
        {
            ((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
            return 0;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        ((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
        break;

    case WM_TIMER:
        if (wParam == TIMER_SHOWDESKTOP)
        {
            s_instance->CheckDesktopState(s_instance->GetDesktopIconsHostWindow());
        }
        else if (wParam == TIMER_RESUME)
        {
            KillTimer(hWnd, TIMER_RESUME);
            s_instance->RefreshWindowPositions();
        }
        break;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        s_instance->RefreshWindowPositions();
        break;

    case WM_POWERBROADCAST:
        if (wParam == PBT_APMRESUMESUSPEND)
        {
            SetTimer(hWnd, TIMER_RESUME, INTERVAL_RESUME, nullptr);
        }
        return TRUE;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

void CALLBACK CZposDesktop::Impl::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (!s_instance || event != EVENT_SYSTEM_FOREGROUND)
        return;

    if (!s_instance->m_showDesktop)
    {
        if (s_instance->ShouldUseShellWindowAsDesktopIconsHost())
        {
            if (hwnd == s_instance->GetDefaultShellWindow())
            {
                const int max = 5;
                int loop = 0;
                while (loop < max && !s_instance->CheckDesktopState(hwnd))
                {
                    Sleep(2);
                    ++loop;
                }
            }
            return;
        }

        const int classLen = 8;
        WCHAR className[classLen];
        if (GetClassName(hwnd, className, classLen) > 0 &&
            wcscmp(className, L"WorkerW") == 0 &&
            s_instance->BelongToSameProcess(s_instance->GetDefaultShellWindow(), hwnd))
        {
            const int max = 5;
            int loop = 0;
            while (loop < max && FindWindowEx(hwnd, nullptr, L"SHELLDLL_DefView", L"") == nullptr)
            {
                Sleep(2);
                ++loop;
            }

            if (loop < max)
            {
                loop = 0;
                while (loop < max && !s_instance->CheckDesktopState(hwnd))
                {
                    Sleep(2);
                    ++loop;
                }
            }
        }
    }
}

// CZposDesktop class implementation
CZposDesktop::CZposDesktop(void) : m_pImpl(new Impl())
{
}

CZposDesktop::~CZposDesktop(void)
{
    delete m_pImpl;
}

bool CZposDesktop::Initialize(HINSTANCE hInstance)
{
    return m_pImpl->Initialize(hInstance);
}

void CZposDesktop::Finalize()
{
    m_pImpl->Finalize();
}

bool CZposDesktop::RegisterWindow(HWND hwnd)
{
    return m_pImpl->RegisterWindow(hwnd);
}

bool CZposDesktop::UnregisterWindow(HWND hwnd)
{
    return m_pImpl->UnregisterWindow(hwnd);
}

DesktopState CZposDesktop::GetDesktopState() const
{
    return m_pImpl->GetDesktopState();
}

void CZposDesktop::SetDesktopStateCallback(DesktopStateCallback callback)
{
    m_pImpl->SetDesktopStateCallback(callback);
}

void CZposDesktop::RefreshWindowPositions()
{
    m_pImpl->RefreshWindowPositions();
}

bool CZposDesktop::IsWindowRegistered(HWND hwnd) const
{
    return m_pImpl->IsWindowRegistered(hwnd);
}

// Global instance for C exports
static std::unique_ptr<CZposDesktop> g_instance;

// C-style exports
extern "C"
{
    ZPOSDESKTOP_API bool __stdcall ZD_Initialize(HINSTANCE hInstance)
    {
        if (!g_instance)
        {
            g_instance = std::make_unique<CZposDesktop>();
        }
        return g_instance->Initialize(hInstance);
    }

    ZPOSDESKTOP_API void __stdcall ZD_Finalize()
    {
        if (g_instance)
        {
            g_instance->Finalize();
            g_instance.reset();
        }
    }

    ZPOSDESKTOP_API bool __stdcall ZD_RegisterWindow(HWND hwnd)
    {
        if (g_instance)
        {
            return g_instance->RegisterWindow(hwnd);
        }
        return false;
    }

    ZPOSDESKTOP_API bool __stdcall ZD_UnregisterWindow(HWND hwnd)
    {
        if (g_instance)
        {
            return g_instance->UnregisterWindow(hwnd);
        }
        return false;
    }

    ZPOSDESKTOP_API int __stdcall ZD_GetDesktopState()
    {
        if (g_instance)
        {
            return static_cast<int>(g_instance->GetDesktopState());
        }
        return 0;
    }

    ZPOSDESKTOP_API void __stdcall ZD_SetDesktopStateCallback(DesktopStateCallback callback)
    {
        if (g_instance)
        {
            g_instance->SetDesktopStateCallback(callback);
        }
    }

    ZPOSDESKTOP_API void __stdcall ZD_RefreshWindowPositions()
    {
        if (g_instance)
        {
            g_instance->RefreshWindowPositions();
        }
    }

    ZPOSDESKTOP_API bool __stdcall ZD_IsWindowRegistered(HWND hwnd)
    {
        if (g_instance)
        {
            return g_instance->IsWindowRegistered(hwnd);
        }
        return false;
    }
}