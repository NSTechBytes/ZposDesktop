#include "pch.h"
#include "ZposDesktop.h"
#include <algorithm>
#include <vector>

#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)
#define TIMER_DESKTOP_CHECK 1001
#define INTERVAL_DESKTOP_CHECK 250

// Static instance for callback handling
CZposDesktop* CZposDesktop::s_instance = nullptr;

CZposDesktop::CZposDesktop()
    : m_helperWindow(nullptr)
    , m_winEventHook(nullptr)
    , m_desktopState(DesktopState::UNKNOWN)
{
    s_instance = this;
}

CZposDesktop::~CZposDesktop()
{
    Finalize();
    if (s_instance == this)
        s_instance = nullptr;
}

bool CZposDesktop::Initialize(HWND targetWindow)
{
    if (m_helperWindow)
        return true; // Already initialized

    if (!CreateHelperWindow())
        return false;

    if (!SetupWindowsEventHook())
    {
        DestroyHelperWindow();
        return false;
    }

    // Start timer for desktop state checking
    SetTimer(m_helperWindow, TIMER_DESKTOP_CHECK, INTERVAL_DESKTOP_CHECK, nullptr);

    // Initial desktop state check
    UpdateDesktopState();

    return true;
}

void CZposDesktop::Finalize()
{
    if (m_helperWindow)
        KillTimer(m_helperWindow, TIMER_DESKTOP_CHECK);

    RemoveWindowsEventHook();
    DestroyHelperWindow();
    m_managedWindows.clear();
}

bool CZposDesktop::RegisterWindow(HWND hwnd, WindowZPosition zpos)
{
    if (!hwnd || !IsWindow(hwnd))
        return false;

    // Remove if already exists
    UnregisterWindow(hwnd);

    // Add to managed windows
    m_managedWindows.push_back(std::make_pair(hwnd, zpos));

    // Update window position immediately
    UpdateWindowPositions();

    return true;
}

bool CZposDesktop::UnregisterWindow(HWND hwnd)
{
    auto it = std::find_if(m_managedWindows.begin(), m_managedWindows.end(),
        [hwnd](const std::pair<HWND, WindowZPosition>& pair) {
            return pair.first == hwnd;
        });

    if (it != m_managedWindows.end())
    {
        m_managedWindows.erase(it);
        return true;
    }

    return false;
}

bool CZposDesktop::SetWindowZPosition(HWND hwnd, WindowZPosition zpos)
{
    auto it = std::find_if(m_managedWindows.begin(), m_managedWindows.end(),
        [hwnd](std::pair<HWND, WindowZPosition>& pair) {
            return pair.first == hwnd;
        });

    if (it != m_managedWindows.end())
    {
        it->second = zpos;
        UpdateWindowPositions();
        return true;
    }

    return false;
}

DesktopState CZposDesktop::GetDesktopState() const
{
    return m_desktopState;
}

void CZposDesktop::UpdateDesktopState()
{
    CheckDesktopState();
}

bool CZposDesktop::IsShowDesktopActive() const
{
    return m_desktopState == DesktopState::DESKTOP_VISIBLE;
}

LRESULT CALLBACK CZposDesktop::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!s_instance)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
        ((LPWINDOWPOS)lParam)->flags |= SWP_NOZORDER;
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_DESKTOP_CHECK)
        {
            s_instance->CheckDesktopState();
        }
        break;

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        // Desktop layout might have changed, update positions
        s_instance->UpdateWindowPositions();
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

void CALLBACK CZposDesktop::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
    LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
    if (!s_instance || event != EVENT_SYSTEM_FOREGROUND)
        return;

    if (s_instance->m_desktopState != DesktopState::DESKTOP_VISIBLE)
    {
        HWND shellWindow = s_instance->GetDefaultShellWindow();

        if (s_instance->ShouldUseShellWindowAsDesktopIconsHost())
        {
            if (hwnd == shellWindow)
            {
                // Windows 11 24H2+ behavior
                for (int i = 0; i < 5 && !s_instance->CheckDesktopState(); ++i)
                {
                    Sleep(2);
                }
            }
        }
        else
        {
            // Pre-Windows 11 24H2 behavior
            WCHAR className[16] = { 0 };
            if (GetClassName(hwnd, className, 16) > 0 &&
                wcscmp(className, L"WorkerW") == 0 &&
                s_instance->BelongToSameProcess(shellWindow, hwnd))
            {
                // Wait for SHELLDLL_DefView to be created
                for (int i = 0; i < 5 && !FindWindowEx(hwnd, nullptr, L"SHELLDLL_DefView", L""); ++i)
                {
                    Sleep(2);
                }

                // Check desktop state
                for (int i = 0; i < 5 && !s_instance->CheckDesktopState(); ++i)
                {
                    Sleep(2);
                }
            }
        }
    }
}

bool CZposDesktop::CreateHelperWindow()
{
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ZposDesktopHelper";

    RegisterClass(&wc);

    m_helperWindow = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        L"ZposDesktopHelper",
        L"ZposDesktopHelper",
        WS_POPUP | WS_DISABLED,
        0, 0, 0, 0,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (m_helperWindow)
    {
        SetWindowPos(m_helperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
        return true;
    }

    return false;
}

void CZposDesktop::DestroyHelperWindow()
{
    if (m_helperWindow)
    {
        DestroyWindow(m_helperWindow);
        m_helperWindow = nullptr;
    }
}

bool CZposDesktop::SetupWindowsEventHook()
{
    m_winEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        nullptr,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

    return m_winEventHook != nullptr;
}

void CZposDesktop::RemoveWindowsEventHook()
{
    if (m_winEventHook)
    {
        UnhookWinEvent(m_winEventHook);
        m_winEventHook = nullptr;
    }
}

HWND CZposDesktop::GetDefaultShellWindow()
{
    static HWND cachedShell = nullptr;
    HWND shellW = GetShellWindow();

    if (shellW && shellW != cachedShell)
    {
        WCHAR className[16] = { 0 };
        if (GetClassName(shellW, className, 16) > 0 && wcscmp(className, L"Progman") == 0)
        {
            cachedShell = shellW;
        }
        else
        {
            shellW = nullptr;
        }
    }

    return shellW;
}

bool CZposDesktop::ShouldUseShellWindowAsDesktopIconsHost()
{
    // Check for Windows 11 24H2+ by looking for GetCurrentMonitorTopologyId function
    static bool checked = false;
    static bool useShellWindow = false;

    if (!checked)
    {
        HMODULE user32 = GetModuleHandle(L"user32");
        useShellWindow = (user32 && GetProcAddress(user32, "GetCurrentMonitorTopologyId") != nullptr);
        checked = true;
    }

    return useShellWindow;
}

HWND CZposDesktop::GetDesktopIconsHostWindow()
{
    static HWND cachedDefView = nullptr;
    HWND shellWindow = GetDefaultShellWindow();

    if (!shellWindow)
        return nullptr;

    if (ShouldUseShellWindowAsDesktopIconsHost())
    {
        // Windows 11 24H2+: SHELLDLL_DefView is child of Progman
        if (FindWindowEx(shellWindow, nullptr, L"SHELLDLL_DefView", L""))
        {
            return shellWindow;
        }
        return nullptr;
    }

    // Pre-Windows 11 24H2: Check cached DefView
    if (cachedDefView && IsWindow(cachedDefView))
    {
        HWND parent = GetAncestor(cachedDefView, GA_PARENT);
        if (parent && parent != shellWindow)
        {
            WCHAR className[16] = { 0 };
            if (GetClassName(parent, className, 16) > 0 && wcscmp(className, L"WorkerW") == 0)
            {
                return parent;
            }
        }
    }

    // Find WorkerW with SHELLDLL_DefView
    HWND workerW = nullptr;
    HWND defView = FindWindowEx(shellWindow, nullptr, L"SHELLDLL_DefView", L"");

    if (!defView)
    {
        while (workerW = FindWindowEx(nullptr, workerW, L"WorkerW", L""))
        {
            if (IsWindowVisible(workerW) &&
                BelongToSameProcess(shellWindow, workerW) &&
                (defView = FindWindowEx(workerW, nullptr, L"SHELLDLL_DefView", L"")))
            {
                cachedDefView = defView;
                return workerW;
            }
        }
    }

    cachedDefView = defView;
    return workerW;
}

bool CZposDesktop::BelongToSameProcess(HWND hwndA, HWND hwndB)
{
    DWORD procAId = 0, procBId = 0;
    GetWindowThreadProcessId(hwndA, &procAId);
    GetWindowThreadProcessId(hwndB, &procBId);
    return (procAId == procBId);
}

bool CZposDesktop::CheckDesktopState()
{
    HWND desktopHost = GetDesktopIconsHostWindow();
    HWND systemWindow = nullptr;
    bool showDesktop = false;

    if (desktopHost && IsWindowVisible(desktopHost))
    {
        systemWindow = FindWindowEx(nullptr, desktopHost, L"ZposDesktopHelper", L"ZposDesktopHelper");
        showDesktop = (systemWindow != nullptr);
    }

    DesktopState newState = showDesktop ? DesktopState::DESKTOP_VISIBLE : DesktopState::WINDOWS_VISIBLE;
    bool stateChanged = (newState != m_desktopState);

    if (stateChanged)
    {
        m_desktopState = newState;
        PrepareHelperWindow();
        UpdateWindowPositions();
    }

    return stateChanged;
}

void CZposDesktop::PrepareHelperWindow()
{
    if (!m_helperWindow)
        return;

    HWND desktopHost = GetDesktopIconsHostWindow();

    if (m_desktopState == DesktopState::DESKTOP_VISIBLE && desktopHost)
    {
        // Set as topmost to detect desktop state
        SetWindowPos(m_helperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);

        // Find the backmost topmost window and insert after it
        HWND hwnd = desktopHost;
        while (hwnd = GetNextWindow(hwnd, GW_HWNDPREV))
        {
            if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
            {
                if (SetWindowPos(m_helperWindow, hwnd, 0, 0, 0, 0, ZPOS_FLAGS))
                    return;
            }
        }
    }
    else
    {
        // Normal state - keep at bottom
        SetWindowPos(m_helperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
    }
}

void CZposDesktop::UpdateWindowPositions()
{
    for (size_t i = 0; i < m_managedWindows.size(); ++i)
    {
        HWND hwnd = m_managedWindows[i].first;
        WindowZPosition zpos = m_managedWindows[i].second;

        if (!IsWindow(hwnd))
            continue;

        switch (zpos)
        {
        case WindowZPosition::DESKTOP:
            if (m_desktopState == DesktopState::DESKTOP_VISIBLE)
            {
                // Keep window visible on desktop
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
            }
            else
            {
                // Normal positioning when desktop not shown
                SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
            }
            break;

        case WindowZPosition::TOPMOST:
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
            break;

        case WindowZPosition::BOTTOM:
            SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
            break;

        case WindowZPosition::NORMAL:
        default:
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
            break;
        }
    }
}

// C-style exports for C# interop
extern "C" {
    static CZposDesktop* g_instance = nullptr;

    ZPOSDESKTOP_API bool ZD_Initialize(HWND targetWindow)
    {
        if (!g_instance)
        {
            g_instance = new CZposDesktop();
        }
        return g_instance->Initialize(targetWindow);
    }

    ZPOSDESKTOP_API void ZD_Finalize()
    {
        if (g_instance)
        {
            g_instance->Finalize();
            delete g_instance;
            g_instance = nullptr;
        }
    }

    ZPOSDESKTOP_API bool ZD_RegisterWindow(HWND hwnd, int zposition)
    {
        if (!g_instance) return false;
        return g_instance->RegisterWindow(hwnd, static_cast<WindowZPosition>(zposition));
    }

    ZPOSDESKTOP_API bool ZD_UnregisterWindow(HWND hwnd)
    {
        if (!g_instance) return false;
        return g_instance->UnregisterWindow(hwnd);
    }

    ZPOSDESKTOP_API bool ZD_SetWindowZPosition(HWND hwnd, int zposition)
    {
        if (!g_instance) return false;
        return g_instance->SetWindowZPosition(hwnd, static_cast<WindowZPosition>(zposition));
    }

    ZPOSDESKTOP_API int ZD_GetDesktopState()
    {
        if (!g_instance) return 0;
        return static_cast<int>(g_instance->GetDesktopState());
    }

    ZPOSDESKTOP_API void ZD_UpdateDesktopState()
    {
        if (g_instance)
            g_instance->UpdateDesktopState();
    }

    ZPOSDESKTOP_API bool ZD_IsShowDesktopActive()
    {
        if (!g_instance) return false;
        return g_instance->IsShowDesktopActive();
    }
}