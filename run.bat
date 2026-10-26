@echo off
rem Сборка и запуск сервера заказа воды на Windows.
rem Нужны CMake и компилятор C++ (Visual Studio 2019+ или MinGW-w64 из MSYS2).
chcp 65001 > nul
cd /d "%~dp0"

cmake -S . -B build || goto :error
cmake --build build --config Release || goto :error

if exist build\Release\water_server.exe (
    build\Release\water_server.exe %*
) else (
    build\water_server.exe %*
)
goto :eof

:error
echo.
echo Сборка не удалась. Проверьте, что установлены CMake и компилятор C++.
pause
exit /b 1
