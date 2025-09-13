using System;
using System.Runtime.InteropServices;

namespace ZposDesktop
{
    /// <summary>
    /// Desktop state enumeration
    /// </summary>
    public enum DesktopState
    {
        ShowingWindows = 0,
        ShowingDesktop = 1
    }

    /// <summary>
    /// Delegate for desktop state change callbacks
    /// </summary>
    /// <param name="state">The new desktop state</param>
    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void DesktopStateCallback(DesktopState state);

    /// <summary>
    /// Windows Desktop Z-Order Manager
    /// Allows windows to stay visible when "Show Desktop" is used
    /// </summary>
    public static class ZposDesktop
    {
        private const string DllName = "ZposDesktop.dll";

        #region Native Methods

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern bool ZD_Initialize(IntPtr hInstance);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern void ZD_Finalize();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern bool ZD_RegisterWindow(IntPtr hwnd);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern bool ZD_UnregisterWindow(IntPtr hwnd);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern int ZD_GetDesktopState();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern void ZD_SetDesktopStateCallback(DesktopStateCallback callback);

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern void ZD_RefreshWindowPositions();

        [DllImport(DllName, CallingConvention = CallingConvention.StdCall, SetLastError = true)]
        private static extern bool ZD_IsWindowRegistered(IntPtr hwnd);

        #endregion

        #region Public API

        /// <summary>
        /// Initialize the desktop manager
        /// </summary>
        /// <returns>True if initialization succeeded</returns>
        public static bool Initialize()
        {
            return ZD_Initialize(IntPtr.Zero);
        }

        /// <summary>
        /// Initialize the desktop manager with a specific module instance
        /// </summary>
        /// <param name="hInstance">Module instance handle</param>
        /// <returns>True if initialization succeeded</returns>
        public static bool Initialize(IntPtr hInstance)
        {
            return ZD_Initialize(hInstance);
        }

        /// <summary>
        /// Cleanup resources and shutdown the desktop manager
        /// </summary>
        public static void Shutdown()
        {
            ZD_Finalize();
        }

        /// <summary>
        /// Register a window to stay visible when "Show Desktop" is used
        /// </summary>
        /// <param name="windowHandle">Handle to the window</param>
        /// <returns>True if registration succeeded</returns>
        public static bool RegisterWindow(IntPtr windowHandle)
        {
            if (windowHandle == IntPtr.Zero)
                throw new ArgumentException("Window handle cannot be zero", nameof(windowHandle));

            return ZD_RegisterWindow(windowHandle);
        }

        /// <summary>
        /// Unregister a previously registered window
        /// </summary>
        /// <param name="windowHandle">Handle to the window</param>
        /// <returns>True if unregistration succeeded</returns>
        public static bool UnregisterWindow(IntPtr windowHandle)
        {
            if (windowHandle == IntPtr.Zero)
                throw new ArgumentException("Window handle cannot be zero", nameof(windowHandle));

            return ZD_UnregisterWindow(windowHandle);
        }

        /// <summary>
        /// Get the current desktop state
        /// </summary>
        /// <returns>Current desktop state</returns>
        public static DesktopState GetDesktopState()
        {
            return (DesktopState)ZD_GetDesktopState();
        }

        /// <summary>
        /// Set a callback to be notified of desktop state changes
        /// </summary>
        /// <param name="callback">Callback function to be called on state changes</param>
        public static void SetDesktopStateCallback(DesktopStateCallback callback)
        {
            ZD_SetDesktopStateCallback(callback);
        }

        /// <summary>
        /// Force refresh of all managed window positions
        /// </summary>
        public static void RefreshWindowPositions()
        {
            ZD_RefreshWindowPositions();
        }

        /// <summary>
        /// Check if a window is currently registered
        /// </summary>
        /// <param name="windowHandle">Handle to the window</param>
        /// <returns>True if the window is registered</returns>
        public static bool IsWindowRegistered(IntPtr windowHandle)
        {
            if (windowHandle == IntPtr.Zero)
                return false;

            return ZD_IsWindowRegistered(windowHandle);
        }

        #endregion

        #region Helper Methods for Common UI Frameworks

#if WPF
        /// <summary>
        /// Register a WPF Window
        /// </summary>
        /// <param name="window">WPF Window instance</param>
        /// <returns>True if registration succeeded</returns>
        public static bool RegisterWindow(System.Windows.Window window)
        {
            if (window == null)
                throw new ArgumentNullException(nameof(window));

            var helper = new System.Windows.Interop.WindowInteropHelper(window);
            return RegisterWindow(helper.Handle);
        }
#endif

#if WINFORMS
        /// <summary>
        /// Register a Windows Forms Form
        /// </summary>
        /// <param name="form">Windows Forms Form instance</param>
        /// <returns>True if registration succeeded</returns>
        public static bool RegisterWindow(System.Windows.Forms.Form form)
        {
            if (form == null)
                throw new ArgumentNullException(nameof(form));

            return RegisterWindow(form.Handle);
        }
#endif

        #endregion
    }

    /// <summary>
    /// Disposable wrapper for automatic cleanup
    /// </summary>
    public sealed class ZposDesktopManager : IDisposable
    {
        private bool _initialized = false;
        private bool _disposed = false;

        /// <summary>
        /// Initialize the desktop manager
        /// </summary>
        public ZposDesktopManager()
        {
            _initialized = ZposDesktop.Initialize();
            if (!_initialized)
                throw new InvalidOperationException("Failed to initialize ZposDesktop");
        }

        /// <summary>
        /// Register a window to stay visible during "Show Desktop"
        /// </summary>
        /// <param name="windowHandle">Window handle</param>
        /// <returns>True if successful</returns>
        public bool RegisterWindow(IntPtr windowHandle)
        {
            ThrowIfDisposed();
            return ZposDesktop.RegisterWindow(windowHandle);
        }

#if WPF
        /// <summary>
        /// Register a WPF Window
        /// </summary>
        /// <param name="window">WPF Window</param>
        /// <returns>True if successful</returns>
        public bool RegisterWindow(System.Windows.Window window)
        {
            ThrowIfDisposed();
            return ZposDesktop.RegisterWindow(window);
        }
#endif

#if WINFORMS
        /// <summary>
        /// Register a Windows Forms Form
        /// </summary>
        /// <param name="form">Windows Forms Form</param>
        /// <returns>True if successful</returns>
        public bool RegisterWindow(System.Windows.Forms.Form form)
        {
            ThrowIfDisposed();
            return ZposDesktop.RegisterWindow(form);
        }
#endif

        /// <summary>
        /// Unregister a window
        /// </summary>
        /// <param name="windowHandle">Window handle</param>
        /// <returns>True if successful</returns>
        public bool UnregisterWindow(IntPtr windowHandle)
        {
            ThrowIfDisposed();
            return ZposDesktop.UnregisterWindow(windowHandle);
        }

        /// <summary>
        /// Get current desktop state
        /// </summary>
        public DesktopState GetDesktopState()
        {
            ThrowIfDisposed();
            return ZposDesktop.GetDesktopState();
        }

        /// <summary>
        /// Set desktop state change callback
        /// </summary>
        /// <param name="callback">Callback function</param>
        public void SetDesktopStateCallback(DesktopStateCallback callback)
        {
            ThrowIfDisposed();
            ZposDesktop.SetDesktopStateCallback(callback);
        }

        /// <summary>
        /// Refresh window positions
        /// </summary>
        public void RefreshWindowPositions()
        {
            ThrowIfDisposed();
            ZposDesktop.RefreshWindowPositions();
        }

        /// <summary>
        /// Check if window is registered
        /// </summary>
        /// <param name="windowHandle">Window handle</param>
        /// <returns>True if registered</returns>
        public bool IsWindowRegistered(IntPtr windowHandle)
        {
            ThrowIfDisposed();
            return ZposDesktop.IsWindowRegistered(windowHandle);
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(ZposDesktopManager));
        }

        /// <summary>
        /// Dispose and cleanup resources
        /// </summary>
        public void Dispose()
        {
            if (!_disposed && _initialized)
            {
                ZposDesktop.Shutdown();
                _disposed = true;
            }
        }
    }
}