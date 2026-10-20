/**
 * ============================================================================
 * Mini-GLUT Windows Native Compatibility Layer
 * ============================================================================
 * Provides drop-in native Win32 implementations of the GLUT functions used
 * by the project, using only built-in Windows APIs (GDI32, User32, OpenGL32).
 * 
 * This allows the project to compile with pure MinGW / GCC on Windows without
 * requiring any external freeglut.dll, glut32.dll, or libfreeglut.a files!
 * ============================================================================
 */

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <map>
#include <iostream>

// Re-define GLUT constants if needed
#ifndef GLUT_DOUBLE
    #define GLUT_DOUBLE 2
#endif
#ifndef GLUT_RGB
    #define GLUT_RGB 0
#endif
#ifndef GLUT_KEY_UP
    #define GLUT_KEY_UP 101
    #define GLUT_KEY_DOWN 103
    #define GLUT_KEY_LEFT 100
    #define GLUT_KEY_RIGHT 102
#endif

// Fonts dummy pointers
void* GLUT_BITMAP_HELVETICA_10 = (void*)1;
void* GLUT_BITMAP_HELVETICA_12 = (void*)2;
void* GLUT_BITMAP_HELVETICA_18 = (void*)3;
void* GLUT_BITMAP_9_BY_15      = (void*)4;
void* GLUT_BITMAP_8_BY_13      = (void*)5;

// Global Window & Callback State
static HWND      g_hWnd = NULL;
static HDC       g_hDC  = NULL;
static HGLRC     g_hRC  = NULL;
static int       g_winW = 1000;
static int       g_winH = 600;
static int       g_winX = 100;
static int       g_winY = 80;
static GLuint    g_fontBase = 0;

static void (*g_displayFunc)(void) = NULL;
static void (*g_reshapeFunc)(int, int) = NULL;
static void (*g_keyboardFunc)(unsigned char, int, int) = NULL;
static void (*g_specialFunc)(int, int, int) = NULL;

struct TimerCallbackData {
    void (*func)(int);
    int value;
};
static std::map<UINT_PTR, TimerCallbackData> g_timers;
static UINT_PTR g_nextTimerId = 1;

static void initFont(HDC hdc) {
    if (g_fontBase == 0) {
        g_fontBase = glGenLists(256);
        HFONT hFont = CreateFontA(
            -13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Arial"
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        wglUseFontBitmapsA(hdc, 0, 255, g_fontBase);
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            PIXELFORMATDESCRIPTOR pfd;
            ZeroMemory(&pfd, sizeof(pfd));
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 16;
            pfd.iLayerType = PFD_MAIN_PLANE;

            g_hDC = GetDC(hWnd);
            int format = ChoosePixelFormat(g_hDC, &pfd);
            SetPixelFormat(g_hDC, format, &pfd);
            g_hRC = wglCreateContext(g_hDC);
            wglMakeCurrent(g_hDC, g_hRC);

            initFont(g_hDC);
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (h == 0) h = 1;
            g_winW = w;
            g_winH = h;
            if (g_reshapeFunc) {
                g_reshapeFunc(w, h);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            if (g_displayFunc) {
                g_displayFunc();
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_CHAR: {
            if (g_keyboardFunc) {
                g_keyboardFunc((unsigned char)wParam, 0, 0);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                if (g_keyboardFunc) g_keyboardFunc(27, 0, 0);
            } else if (wParam == VK_UP) {
                if (g_specialFunc) g_specialFunc(GLUT_KEY_UP, 0, 0);
            } else if (wParam == VK_DOWN) {
                if (g_specialFunc) g_specialFunc(GLUT_KEY_DOWN, 0, 0);
            } else if (wParam == VK_LEFT) {
                if (g_specialFunc) g_specialFunc(GLUT_KEY_LEFT, 0, 0);
            } else if (wParam == VK_RIGHT) {
                if (g_specialFunc) g_specialFunc(GLUT_KEY_RIGHT, 0, 0);
            }
            return 0;
        }

        case WM_TIMER: {
            UINT_PTR timerId = (UINT_PTR)wParam;
            KillTimer(hWnd, timerId);
            std::map<UINT_PTR, TimerCallbackData>::iterator it = g_timers.find(timerId);
            if (it != g_timers.end()) {
                TimerCallbackData cb = it->second;
                g_timers.erase(it);
                if (cb.func) {
                    cb.func(cb.value);
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            if (g_fontBase != 0) {
                glDeleteLists(g_fontBase, 256);
                g_fontBase = 0;
            }
            wglMakeCurrent(NULL, NULL);
            if (g_hRC) wglDeleteContext(g_hRC);
            if (g_hDC) ReleaseDC(hWnd, g_hDC);
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// ----------------------------------------------------------------------------
// C-Linkage GLUT API Implementations
// ----------------------------------------------------------------------------
extern "C" {

void glutInit(int* pargc, char** argv) {
    (void)pargc;
    (void)argv;
}

void __glutInitWithExit(int* pargc, char** argv, void (*exit_func)(int)) {
    (void)pargc;
    (void)argv;
    (void)exit_func;
}

void glutInitDisplayMode(unsigned int mode) {
    (void)mode;
}

void glutInitWindowSize(int width, int height) {
    g_winW = width;
    g_winH = height;
}

void glutInitWindowPosition(int x, int y) {
    g_winX = x;
    g_winY = y;
}

int glutCreateWindow(const char* title) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "WalkingRobotGLUTWindowClass";

    RegisterClassA(&wc);

    RECT rect = { 0, 0, g_winW, g_winH };
    DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    AdjustWindowRect(&rect, dwStyle, FALSE);

    int actualW = rect.right - rect.left;
    int actualH = rect.bottom - rect.top;

    g_hWnd = CreateWindowExA(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        title,
        dwStyle,
        g_winX, g_winY,
        actualW, actualH,
        NULL, NULL,
        hInstance, NULL
    );

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    return 1;
}

int __glutCreateWindowWithExit(const char* title, void (*exit_func)(int)) {
    (void)exit_func;
    return glutCreateWindow(title);
}

void glutDisplayFunc(void (*func)(void)) {
    g_displayFunc = func;
}

void glutReshapeFunc(void (*func)(int, int)) {
    g_reshapeFunc = func;
}

void glutKeyboardFunc(void (*func)(unsigned char, int, int)) {
    g_keyboardFunc = func;
}

void glutSpecialFunc(void (*func)(int, int, int)) {
    g_specialFunc = func;
}

void glutTimerFunc(unsigned int msecs, void (*func)(int value), int value) {
    if (!g_hWnd) return;
    UINT_PTR id = g_nextTimerId++;
    TimerCallbackData cb = { func, value };
    g_timers[id] = cb;
    SetTimer(g_hWnd, id, msecs, NULL);
}

void glutPostRedisplay(void) {
    if (g_hWnd) {
        InvalidateRect(g_hWnd, NULL, FALSE);
    }
}

void glutSwapBuffers(void) {
    if (g_hDC) {
        SwapBuffers(g_hDC);
    }
}

void glutBitmapCharacter(void* font, int character) {
    (void)font;
    if (g_fontBase == 0) return;
    glPushAttrib(GL_LIST_BIT);
    glListBase(g_fontBase);
    GLubyte ch = (GLubyte)character;
    glCallLists(1, GL_UNSIGNED_BYTE, &ch);
    glPopAttrib();
}

void glutMainLoop(void) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

} // extern "C"

#endif // _WIN32
