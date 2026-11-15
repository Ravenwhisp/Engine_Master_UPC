// EngineDX.cpp : Defines the entry point for the application.
//

#include "Globals.h"

#include "framework.h"
#include "Engine.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleRender.h"

#include <shellapi.h>
#include <cwchar>

#include "dxgidebug.h"

#include "Keyboard.h"
#include "Mouse.h"

#include "backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

Application* app = nullptr;

#ifdef GAME_RELEASE
namespace
{
    bool fullscreen = false;
    WINDOWPLACEMENT windowedPlacement{ sizeof(WINDOWPLACEMENT) };
    LONG_PTR windowedStyle = 0;
    LONG_PTR windowedExStyle = 0;

    void toggleFullscreen(HWND hWnd)
    {
        if (!fullscreen)
        {
            windowedPlacement.length = sizeof(WINDOWPLACEMENT);
            if (!GetWindowPlacement(hWnd, &windowedPlacement)) return;
            windowedStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
            windowedExStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);

            MONITORINFO monitor{ sizeof(MONITORINFO) };
            if (!GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor)) return;
            const RECT& bounds = monitor.rcMonitor;
            SetWindowLongPtrW(hWnd, GWL_STYLE, (windowedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowLongPtrW(hWnd, GWL_EXSTYLE, windowedExStyle & ~WS_EX_WINDOWEDGE);
            SetWindowPos(hWnd, HWND_TOP, bounds.left, bounds.top,
                bounds.right - bounds.left, bounds.bottom - bounds.top,
                SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
            fullscreen = true;
        }
        else
        {
            SetWindowLongPtrW(hWnd, GWL_STYLE, windowedStyle);
            SetWindowLongPtrW(hWnd, GWL_EXSTYLE, windowedExStyle);
            SetWindowPlacement(hWnd, &windowedPlacement);
            SetWindowPos(hWnd, nullptr, 0, 0, 0, 0,
                SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
            fullscreen = false;
        }
    }
}
#endif

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef GAME_RELEASE
    // Relative asset and shader paths must resolve beside the executable even
    // when launched from a shortcut or another working directory.
    wchar_t executablePath[MAX_PATH];
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH) return FALSE;
    wchar_t* separator = wcsrchr(executablePath, L'\\');
    if (!separator) return FALSE;
    *separator = L'\0';
    if (!SetCurrentDirectoryW(executablePath)) return FALSE;

    constexpr const wchar_t* requiredFiles[] = { L"build.cfg", L"GameScripts.dll", L"Library" };
    for (const wchar_t* required : requiredFiles)
    {
        if (GetFileAttributesW(required) == INVALID_FILE_ATTRIBUTES)
        {
            std::wstring message = L"Missing ";
            message += required;
            message += L" beside BoundByDeath.exe. Complete the game release files before launching.";
            MessageBoxW(nullptr, message.c_str(), L"Bound By Death", MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }
#endif

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ENGINEDX, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (CoInitialize(nullptr) != S_OK)
    {
        return FALSE;
    }

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ENGINEDX));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (app && app->shouldQuit())
        {
            PostMessage(app->getWindowHandle(), WM_CLOSE, 0, 0);
        }

    }

    delete app;

    ComPtr<IDXGIDebug> dxgiControler;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiControler))))
    {
        dxgiControler->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
#ifdef GAME_RELEASE
    wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ENGINEDX));
#else
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
#endif
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL; 
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

#ifdef GAME_RELEASE
   // Establish the fullscreen client size before the D3D12 swap chain is
   // created. Resizing it during startup can race resource initialization.
   toggleFullscreen(hWnd);
#endif

   app = new Application(__argc, __wargv, hWnd);

   if(!app->init())
   {
       delete app;

       return FALSE;
   }

#ifndef GAME_RELEASE
   // Set the window to be the size of the monitor
   MONITORINFO monitor = {};
   monitor.cbSize = sizeof(monitor);
   GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor);
   RECT rect = monitor.rcMonitor;
   SetWindowPos(hWnd, nullptr, static_cast<int>(rect.left), static_cast<int>(rect.top), static_cast<int>(rect.left + rect.right), static_cast<int>(rect.top + rect.bottom), SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
   SetWindowPos(hWnd, nullptr, static_cast<int>(rect.left), static_cast<int>(rect.top), static_cast<int>(rect.left + rect.right), static_cast<int>(rect.top + rect.bottom), 0);
#endif

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_CREATE:
        {
#ifndef GAME_RELEASE
            MessageBox(hWnd, L"Initializing engine...", L"Starting Application", MB_OK | MB_USERICON);
#endif
        }
        break;
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        Mouse::ProcessMessage(message, wParam, lParam);
        break;
    case WM_INPUT:
    case WM_MOUSEMOVE:
        Mouse::ProcessMessage(message, wParam, lParam);
		break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        Mouse::ProcessMessage(message, wParam, lParam);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            app->update();
        }
        break;
    case WM_DESTROY:

        // application cleanup?

        PostQuitMessage(0);
        break;
    case WM_SIZE:
        if (!app) break;
        if (wParam == SIZE_MINIMIZED) {
            app->setPaused(true);
        }
        else {
            app->setPaused(false);
            app->getModuleD3D12()->getSwapChain()->resize();


        }
        break;
    case WM_SYSKEYDOWN:
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;
    case WM_KEYDOWN:
#ifdef GAME_RELEASE
        if (wParam == VK_F11)
        {
            // Ignore key repeat so holding F11 cannot switch modes repeatedly.
            if ((lParam & (1 << 30)) == 0) toggleFullscreen(hWnd);
            return 0;
        }
#endif
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard::ProcessMessage(message, wParam, lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
