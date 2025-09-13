# ZposDesktop

A Windows library that allows applications to keep windows visible when users press "Show Desktop" (Win+D). Perfect for desktop widgets, always-on-top utilities, and applications that need to remain visible on the desktop.

## Features

- **Show Desktop Compatibility** - Keep your windows visible when Win+D is pressed
- **Cross-Platform Architecture Support** - Works on both x64 and x86 Windows systems
- **Multiple UI Framework Support** - Direct support for WPF, WinForms, and native Win32
- **Desktop State Monitoring** - Get callbacks when desktop state changes
- **Windows 11 24H2+ Support** - Fully compatible with the latest Windows versions
- **Easy Integration** - Simple C# wrapper with disposable pattern support

## Quick Start

### Installation

1. Download the latest release from the [Releases](../../releases) page
2. Extract the appropriate DLL (x64 or x86) for your application
3. Add the `ZposDesktop.cs` file to your C# project, or reference the native DLL directly

### Basic Usage (C#)

```csharp
using ZposDesktop;

// Initialize the desktop manager
using var manager = new ZposDesktopManager();

// Register your window to stay visible during "Show Desktop"
manager.RegisterWindow(yourWindow);

// Optional: Set up a callback to monitor desktop state changes
manager.SetDesktopStateCallback(state =>
{
    Console.WriteLine($"Desktop state changed to: {state}");
});
```

### WPF Example

```csharp
public partial class MainWindow : Window
{
    private ZposDesktopManager _desktopManager;

    public MainWindow()
    {
        InitializeComponent();
        
        // Initialize ZposDesktop
        _desktopManager = new ZposDesktopManager();
        
        // Register this window
        _desktopManager.RegisterWindow(this);
    }

    protected override void OnClosed(EventArgs e)
    {
        _desktopManager?.Dispose();
        base.OnClosed(e);
    }
}
```

### Windows Forms Example

```csharp
public partial class Form1 : Form
{
    private ZposDesktopManager _desktopManager;

    public Form1()
    {
        InitializeComponent();
        
        _desktopManager = new ZposDesktopManager();
        _desktopManager.RegisterWindow(this);
    }

    protected override void OnFormClosed(FormClosedEventArgs e)
    {
        _desktopManager?.Dispose();
        base.OnFormClosed(e);
    }
}
```

## API Reference

### C# API

#### ZposDesktop Static Class

```csharp
// Core methods
bool Initialize()
void Finalize()
bool RegisterWindow(IntPtr windowHandle)
bool UnregisterWindow(IntPtr windowHandle)
DesktopState GetDesktopState()
void SetDesktopStateCallback(DesktopStateCallback callback)
void RefreshWindowPositions()
bool IsWindowRegistered(IntPtr windowHandle)

// Framework-specific helpers
bool RegisterWindow(System.Windows.Window window)        // WPF
bool RegisterWindow(System.Windows.Forms.Form form)      // WinForms
```

#### ZposDesktopManager Class (Disposable Wrapper)

```csharp
// Constructor
ZposDesktopManager()

// All methods from ZposDesktop static class
// Plus automatic cleanup via IDisposable
```

#### Enums and Delegates

```csharp
public enum DesktopState
{
    ShowingWindows = 0,
    ShowingDesktop = 1
}

public delegate void DesktopStateCallback(DesktopState state);
```

### Native C++ API

```cpp
class CZposDesktop
{
    bool Initialize(HINSTANCE hInstance);
    void Finalize();
    bool RegisterWindow(HWND hwnd);
    bool UnregisterWindow(HWND hwnd);
    DesktopState GetDesktopState() const;
    void SetDesktopStateCallback(DesktopStateCallback callback);
    void RefreshWindowPositions();
    bool IsWindowRegistered(HWND hwnd) const;
};
```

## How It Works

ZposDesktop works by:

1. **Desktop State Detection** - Monitoring Windows shell messages to detect when "Show Desktop" is activated
2. **Z-Order Management** - Dynamically repositioning registered windows in the Z-order to keep them visible
3. **Windows Version Compatibility** - Using different strategies for Windows 10, 11, and 11 24H2+
4. **Event Hooking** - Listening for system events to maintain proper window positioning

The library creates invisible helper windows that act as Z-order anchors, ensuring your registered windows stay visible above the desktop but below normal application windows when "Show Desktop" is active.

## Building from Source

### Prerequisites

- Visual Studio 2019 or later
- Windows SDK 10.0.19041.0 or later
- MSBuild
- NuGet CLI (optional, for package creation)

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/ZposDesktop.git
   cd ZposDesktop
   ```

2. Build using the provided script:
   ```batch
   Build-Package.bat
   ```

   Or manually using MSBuild:
   ```batch
   msbuild ZposDesktop.sln /p:Configuration=Release /p:Platform=x64
   msbuild ZposDesktop.sln /p:Configuration=Release /p:Platform=Win32
   ```

### Output Files

After building, you'll find:
- `x64/Release/ZposDesktop.dll` - 64-bit library
- `x64/Release/ZposDesktop.lib` - 64-bit import library
- `Release/ZposDesktop.dll` - 32-bit library
- `Release/ZposDesktop.lib` - 32-bit import library

## System Requirements

- **Windows 10** version 1903 or later
- **Windows 11** all versions (including 24H2+)
- **.NET Framework 4.6.1** or later (for C# wrapper)
- **Visual C++ Redistributable** (latest version recommended)

## Supported Architectures

- x64 (64-bit)
- x86 (32-bit)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

### Development Guidelines

- Follow the existing code style and conventions
- Add appropriate error handling and logging
- Test on multiple Windows versions when possible
- Update documentation for any API changes

## Known Issues

- Some third-party desktop enhancement tools may interfere with Z-order management
- In rare cases, window positioning may need manual refresh after display configuration changes
- Minimized windows cannot be kept visible (by Windows design)

## Troubleshooting

**Q: I get compilation errors about missing WPF or WinForms references**
A: The updated C# wrapper uses `IntPtr` handles instead of framework-specific types to avoid dependency issues. Use the appropriate method to get your window handle (see "Getting Window Handles" section above).

**Q: My window doesn't stay visible after registering it**
A: Ensure the window is visible and not minimized when registering. Try calling `RefreshWindowPositions()` after registration.

**Q: The library doesn't work on my Windows version**
A: ZposDesktop requires Windows 10 1903 or later. Check your Windows version with `winver`.

**Q: I get a DLL load error**
A: Make sure you're using the correct architecture (x64/x86) and that the Visual C++ Redistributable is installed.

## Support

If you encounter any issues or have questions:

1. Check the [Issues](../../issues) page for known problems
2. Create a new issue with detailed information about your environment
3. Include error messages and steps to reproduce

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for a detailed history of changes.

---

**Made with ❤️ for the Windows desktop development community**