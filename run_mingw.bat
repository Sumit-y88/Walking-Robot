@echo off
title Walking Robot OpenGL Build and Run
color 0b
echo =========================================================================
echo          Walking Robot with Obstacle Navigation (OpenGL)
echo =========================================================================
echo.
echo [1/2] Compiling main.cpp with MinGW g++ ...

g++ -O2 -Wall -DFREEGLUT_STATIC main.cpp glut_compat.cpp -o robot.exe -Iinclude -lopengl32 -lglu32 -lgdi32 -static-libgcc -static-libstdc++

if %errorlevel% equ 0 (
    echo.
    echo [2/2] Build Successful! Launching robot.exe ...
    echo.
    robot.exe
) else (
    echo.
    echo =====================================================================
    echo [ERROR] Compilation failed.
    echo Please make sure MinGW (g++.exe) is added to your Windows PATH.
    echo =====================================================================
)
echo.
pause
