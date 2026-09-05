@echo off
setlocal

REM ============================================================
REM  Config - ajusta estos valores a tu proyecto
REM ============================================================
set DRIVER_NAME=km
set SYS_PATH=%~dp0km.sys

REM ============================================================
REM  Verificar que se corre como administrador
REM ============================================================
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Este script necesita ejecutarse como Administrador.
    pause
    exit /b 1
)

echo ==============================================
echo   Redeploy de driver: %DRIVER_NAME%
echo ==============================================

REM ============================================================
REM  1. Detener el driver si esta corriendo
REM ============================================================
echo.
echo [1/4] Deteniendo servicio %DRIVER_NAME% (si esta activo)...
sc query %DRIVER_NAME% >nul 2>&1
if %errorlevel%==0 (
    sc stop %DRIVER_NAME%
    REM Esperar a que realmente pare
    :waitstop
    timeout /t 1 /nobreak >nul
    sc query %DRIVER_NAME% | find "STOPPED" >nul
    if errorlevel 1 goto waitstop
    echo Servicio detenido.
) else (
    echo El servicio no existe o no esta corriendo. Continuando...
)

REM ============================================================
REM  2. Eliminar el servicio existente
REM ============================================================
echo.
echo [2/4] Eliminando servicio %DRIVER_NAME%...
sc query %DRIVER_NAME% >nul 2>&1
if %errorlevel%==0 (
    sc delete %DRIVER_NAME%
    timeout /t 1 /nobreak >nul
    echo Servicio eliminado.
) else (
    echo No hay servicio previo que eliminar.
)

REM ============================================================
REM  3. Crear el servicio con el nuevo .sys
REM ============================================================
echo.
echo [3/4] Creando servicio con %SYS_PATH%...
if not exist "%SYS_PATH%" (
    echo [ERROR] No se encuentra el archivo: %SYS_PATH%
    pause
    exit /b 1
)

sc create %DRIVER_NAME% type= kernel binPath= "%SYS_PATH%"
if %errorlevel% neq 0 (
    echo [ERROR] Fallo al crear el servicio.
    pause
    exit /b 1
)
echo Servicio creado.

REM ============================================================
REM  4. Arrancar el driver
REM ============================================================
echo.
echo [4/4] Iniciando servicio %DRIVER_NAME%...
sc start %DRIVER_NAME%
if %errorlevel% neq 0 (
    echo [ERROR] Fallo al iniciar el servicio. Revisa WinDbg / Event Viewer para mas detalles.
    pause
    exit /b 1
)

echo.
echo ==============================================
echo   Driver %DRIVER_NAME% desplegado y corriendo.
echo ==============================================
pause