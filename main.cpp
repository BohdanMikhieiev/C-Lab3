#include <windows.h>
#include <string>
#include <sstream>

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetProcessDPIAware();
    WNDCLASSW wc = {};
    wc.style =  CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"Bohdan And Sasha";

    if (!RegisterClassW(&wc))
        return -1;

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"Bohdan And Sasha",
        WS_OVERLAPPEDWINDOW | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, hInstance, nullptr
        );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hwindow, UINT message, WPARAM wParam, LPARAM lParam) {
    static int windowWidth = 0;
    static int windowHeight = 0;

    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwindow, &ps);

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            int scrollWidth = 0, scrollHeight = 0;
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            if (GetScrollInfo(hwindow, SB_HORZ, &si)) {
                scrollWidth = si.nMax - si.nMin;
                scrollHeight = GetSystemMetrics(SM_CYHSCROLL);
            }

            std::wstringstream info;
            info << L"Screen: " << screenWidth << L"x" << screenHeight << L" | ";
            info << L"Scroll (H): " << scrollWidth << L"x" << scrollHeight << L" | ";
            info << L"Window: " << windowWidth << L"x" << windowHeight;

            std::wstring text = info.str();

            SIZE textSize;
            GetTextExtentPoint32W(hdc, text.c_str(), static_cast<int>(text.size()), &textSize);

            RECT rect;
            GetClientRect(hwindow, &rect);
            int clientWidth = rect.right - rect.left;
            int x = (clientWidth - textSize.cx) / 2;
            int y = 10;

            TextOutW(hdc, x, y, text.c_str(), static_cast<int>(text.size()));

            EndPaint(hwindow, &ps);
            return 0;
        }
        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(hwindow, &clientRect);
            windowWidth = clientRect.right - clientRect.left;
            windowHeight = clientRect.bottom - clientRect.top + GetSystemMetrics(SM_CYHSCROLL);

            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE;
            si.nMin = 0;
            si.nMax = windowWidth;
            si.nPage = windowWidth;
            SetScrollInfo(hwindow, SB_HORZ, &si, TRUE);

            InvalidateRect(hwindow, nullptr, TRUE);
            return 0;
        }
        case WM_MOVE: {
            InvalidateRect(hwindow, nullptr, TRUE);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            break;
        }
        default: ;
    }

    return DefWindowProcW(hwindow, message, wParam, lParam);
}
