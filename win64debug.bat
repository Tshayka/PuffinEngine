@echo off
setlocal

set "BUILD_DIR=build64d"
set "BUILD_TYPE=Debug"
set "APP_EXE=%CD%\PuffinEngine\work\debug\Debug\PuffinEngine.exe"

call :configure_build_run
exit /b %ERRORLEVEL%

:configure_build_run
if not defined VULKAN_SDK (
    for /f "delims=" %%D in ('dir /b /ad /o-n "C:\VulkanSDK" 2^>nul') do (
        set "VULKAN_SDK=C:\VulkanSDK\%%D"
        goto :vulkan_found
    )
)

:vulkan_found
if defined VULKAN_SDK (
    set "PATH=%VULKAN_SDK%\Bin;%PATH%"
)

where cmake >nul 2>nul
if errorlevel 1 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\CMake\bin;%PATH%"
    )
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install CMake or add it to PATH.
    exit /b 1
)

cmake --help | findstr /c:"Visual Studio 18 2026" >nul
if not errorlevel 1 (
    set "CMAKE_GENERATOR=Visual Studio 18 2026"
    goto :generator_found
)

cmake --help | findstr /c:"Visual Studio 17 2022" >nul
if not errorlevel 1 (
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    goto :generator_found
)

cmake --help | findstr /c:"Visual Studio 16 2019" >nul
if not errorlevel 1 (
    set "CMAKE_GENERATOR=Visual Studio 16 2019"
    goto :generator_found
)

echo No supported Visual Studio CMake generator was found.
exit /b 1

:generator_found
cmake --fresh -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -G "%CMAKE_GENERATOR%" -A x64 -Wdev
if errorlevel 1 exit /b %ERRORLEVEL%

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --target PuffinEngine
if errorlevel 1 exit /b %ERRORLEVEL%

if not exist "%APP_EXE%" (
    echo Built executable was not found at "%APP_EXE%".
    exit /b 1
)

start "" /D "%CD%\%BUILD_DIR%" "%APP_EXE%"
exit /b 0
