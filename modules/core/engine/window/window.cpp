#include "window.h"
#include <stdexcept>

Window::Window()
#ifdef _WIN32
    : hwnd(nullptr)
    , hInstance(nullptr)
    , width(0)
    , height(0)
    , fullscreen(false)
    , borderless(false)
#endif
{
}

Window::~Window()
{
#ifdef _WIN32
    windowShutdown();
#endif
}

#ifdef _WIN32

std::wstring Window::UTF8ToWide(const std::string& str)
{
    if (str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

bool Window::windowInit(
    int width,
    int height,
    const std::string& title,
    bool fullscreen,
    bool borderless)
{
    this->hInstance = GetModuleHandle(NULL);
    if (!this->hInstance)
    {
        return false;
    }

    this->width = width;
    this->height = height;
    this->title = title;
    this->fullscreen = fullscreen;
    this->borderless = borderless;

    std::wstring wideTitle = UTF8ToWide(title);
    std::wstring className = L"QuarkWindowClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProcStatic;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className.c_str();

    if (!RegisterClassExW(&wc))
    {
        return false;
    }

    DWORD style;
    DWORD exStyle = WS_EX_APPWINDOW;

    if (fullscreen)
    {
        style = WS_POPUP;

        DEVMODE dmScreenSettings = {};
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            fullscreen = false;
            style = WS_OVERLAPPEDWINDOW;
        }
    }
    else if (borderless)
    {
        style = WS_POPUP;
    }
    else
    {
        style = WS_OVERLAPPEDWINDOW;
    }

    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    if (fullscreen)
    {
        posX = 0;
        posY = 0;
        windowWidth = width;
        windowHeight = height;
    }

    hwnd = CreateWindowExW(
        exStyle,
        className.c_str(),
        wideTitle.c_str(),
        style,
        posX, posY,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!hwnd)
    {
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    return true;
}

void Window::windowShutdown()
{
    if (fullscreen)
    {
        ChangeDisplaySettings(nullptr, 0);
    }

    if (hwnd)
    {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }

    if (hInstance)
    {
        UnregisterClassW(L"QuarkWindowClass", hInstance);
    }
}

void Window::pollMessages()
{
    MSG msg = {};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void Window::setWindowTitle(const std::string& newTitle)
{
    title = newTitle;
    std::wstring wideTitle = UTF8ToWide(newTitle);
    SetWindowTextW(hwnd, wideTitle.c_str());
}

void Window::setWindowSize(int newWidth, int newHeight)
{
    if (!hwnd) return;

    width = newWidth;
    height = newHeight;

    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    SetWindowPos(hwnd, nullptr, posX, posY, windowWidth, windowHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CALLBACK Window::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow = nullptr;

    if (msg == WM_CREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    }
    else
    {
        pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pWindow)
    {
        return pWindow->WndProc(hwnd, msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        if (closeEventCallback)
        {
            closeEventCallback();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        width = LOWORD(lParam);
        height = HIWORD(lParam);
        break;
    }

    if (eventCallback)
    {
        eventCallback(msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

#endif