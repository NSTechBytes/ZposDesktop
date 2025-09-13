@echo off
echo Building ZposDesktop for all platforms...

REM Check what configurations are available
echo Available configurations:
msbuild ZposDesktop.sln /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal

REM Clean first
echo Cleaning previous builds...
msbuild ZposDesktop.sln /t:Clean /p:Configuration=Release /p:Platform=x64 /nologo /verbosity:minimal
msbuild ZposDesktop.sln /t:Clean /p:Configuration=Release /p:Platform=x86 /nologo /verbosity:minimal

echo.
echo Building x64 Release...
msbuild ZposDesktop.sln /t:Build /p:Configuration=Release /p:Platform=x64 /m /nologo
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build x64 Release
    pause
    exit /b 1
)
echo x64 Release build completed successfully!

echo.
echo Building Win32 Release...
REM Try different platform specifications
msbuild ZposDesktop.sln /t:Build /p:Configuration=Release /p:Platform="Win32" /m /nologo
if %ERRORLEVEL% NEQ 0 (
    echo Trying alternative platform name...
    msbuild ZposDesktop.sln /t:Build /p:Configuration=Release /p:Platform="x86" /m /nologo
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Failed to build Win32/x86 Release
        echo Available platforms in solution:
        msbuild ZposDesktop.sln /t:Build /p:Configuration=Release /verbosity:diagnostic | findstr "Platform"
        pause
        exit /b 1
    )
)
echo Win32 Release build completed successfully!

echo.
echo All builds completed successfully!

REM List output files
echo.
echo Output files:
if exist "x64\Release\ZposDesktop.dll" echo - x64\Release\ZposDesktop.dll
if exist "x64\Release\ZposDesktop.lib" echo - x64\Release\ZposDesktop.lib
if exist "Release\ZposDesktop.dll" echo - Release\ZposDesktop.dll (x86)
if exist "Release\ZposDesktop.lib" echo - Release\ZposDesktop.lib (x86)

REM Check if NuGet executable exists
echo.
echo Checking for NuGet...
where nuget >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo NuGet not found in PATH. Please install NuGet CLI or add it to PATH.
    echo You can download it from: https://www.nuget.org/downloads
    echo.
    echo Alternative: Use dotnet CLI instead:
    echo dotnet pack (if you have a .NET project file)
    pause
    exit /b 1
)

echo Creating NuGet package...
nuget pack ZposDesktop.nuspec
if %ERRORLEVEL% NEQ 0 (
    echo Failed to create NuGet package
    pause
    exit /b 1
)


pause