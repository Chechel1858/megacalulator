@echo off
rem Сборка сервера без CMake и без прав администратора.
rem Сам ищет компилятор: Visual Studio (cl) или MinGW (g++).
rem Можно указать компилятор явно:  build.bat msvc   или   build.bat gcc
chcp 65001 > nul
setlocal
cd /d "%~dp0"

set "SOURCES=server\main.cpp server\api.cpp server\catalog.cpp server\order.cpp server\order_store.cpp"
set "CHOICE=%~1"

if /i "%CHOICE%"=="msvc" goto :find_msvc
if /i "%CHOICE%"=="gcc" goto :find_gcc

rem 1. Компилятор уже доступен (например, открыт Developer Command Prompt)
where cl >nul 2>nul && goto :msvc
where g++ >nul 2>nul && goto :gcc

:find_msvc
rem 2. Ищем установленную Visual Studio
where cl >nul 2>nul && goto :msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
)
if defined VSPATH if exist "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
    goto :msvc
)
if /i "%CHOICE%"=="msvc" goto :not_found

:find_gcc
rem 3. Ищем MinGW в обычных местах (в том числе распакованный рядом w64devkit)
where g++ >nul 2>nul && goto :gcc
for %%d in (
    "%~dp0w64devkit\bin"
    "C:\w64devkit\bin"
    "C:\msys64\ucrt64\bin"
    "C:\msys64\mingw64\bin"
    "C:\mingw64\bin"
    "%ProgramFiles%\CodeBlocks\MinGW\bin"
    "%ProgramFiles(x86)%\CodeBlocks\MinGW\bin"
) do (
    if exist "%%~d\g++.exe" (
        set "PATH=%%~d;%PATH%"
        goto :gcc
    )
)
goto :not_found

:msvc
echo Сборка компилятором Visual Studio (cl)...
if not exist build mkdir build
cl /nologo /EHsc /std:c++17 /O2 /utf-8 /W3 /Iserver /Ithird_party ^
   /D_WIN32_WINNT=0x0A00 /DNOMINMAX ^
   %SOURCES% /Fobuild\ /Fewater_server.exe /link ws2_32.lib
if errorlevel 1 goto :failed
goto :done

:gcc
echo Сборка компилятором MinGW (g++)...
g++ -std=c++17 -O2 -Iserver -Ithird_party -D_WIN32_WINNT=0x0A00 ^
    %SOURCES% -o water_server.exe -lws2_32 -static
if errorlevel 1 goto :failed
goto :done

:done
echo.
echo Готово: water_server.exe
echo Запуск: run.bat  (или просто water_server.exe)
exit /b 0

:not_found
echo.
echo Не найден компилятор C++.
echo Варианты без прав администратора:
echo   1. Если есть Visual Studio, откройте WaterOrderSystem.sln и нажмите F5.
echo   2. Скачайте w64devkit (https://github.com/skeeto/w64devkit/releases),
echo      распакуйте папку w64devkit рядом с этим файлом и запустите build.bat снова.
if not defined CI pause
exit /b 1

:failed
echo.
echo Сборка не удалась, текст ошибки выше.
if not defined CI pause
exit /b 1
