@echo off
REM =====================================================
REM  JAMMER 2.4GHz - compile + flash ke ESP-12E (COM9)
REM  arduino-cli 1.6.0: -b = FQBN (bukan baud rate)
REM =====================================================
set "CLI="
if exist "C:\Program Files\Arduino CLI\arduino-cli.exe" set "CLI=C:\Program Files\Arduino CLI\arduino-cli.exe"
if defined CLI goto run
where arduino-cli >nul 2>nul
if errorlevel 1 (
  echo arduino-cli tidak ditemukan.
  pause
  exit /b 1
)
set "CLI=arduino-cli"
:run
"%CLI%" board list
"%CLI%" compile --fqbn esp8266:esp8266:nodemcuv2 "%~dp0jammer24"
if errorlevel 1 (
  echo.
  echo GAGAL COMPILE. Cek pesan error di atas.
  pause
  exit /b 1
)
"%CLI%" upload -b esp8266:esp8266:nodemcuv2 -p COM9 "%~dp0jammer24"
if errorlevel 1 (
  echo.
  echo GAGAL UPLOAD. Cek kabel USB dan port COM via: arduino-cli board list
  pause
  exit /b 1
)
echo.
echo SELESAI. Buka Serial Monitor 115200 untuk melihat status jammer.
