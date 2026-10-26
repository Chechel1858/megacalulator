@echo off
rem Запуск сервера заказа воды. Если сервер ещё не собран, сначала собирает его.
chcp 65001 > nul
cd /d "%~dp0"

if not exist water_server.exe (
    call build.bat || exit /b 1
)

start "Сервер заказа воды" water_server.exe %*
rem Даём серверу секунду на запуск и открываем сайт в браузере.
timeout /t 1 /nobreak > nul
start "" http://localhost:8080/
