#include <windows.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

const int CIRCLE_RADIUS = 20;
const int MOVE_SPEED = 5;
const int TIMER_ID = 1;

struct Circle {
    int x, y;
    int dx, dy;
    COLORREF color;
    HANDLE threadHandle;
    DWORD threadId;
    bool active;
};

std::vector<Circle> g_circles;
CRITICAL_SECTION g_cs;
HWND g_hwnd;
volatile bool g_running = true;

DWORD WINAPI CircleThreadProc(LPVOID lpParam);
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    SetProcessDPIAware();
    srand(static_cast<unsigned int>(time(nullptr)));

    InitializeCriticalSection(&g_cs);

    WNDCLASSW wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
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
            800, 600,
            nullptr, nullptr, hInstance, nullptr
    );

    g_hwnd = hwnd;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    SetTimer(hwnd, TIMER_ID, 30, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwnd, TIMER_ID);

    g_running = false;

    EnterCriticalSection(&g_cs);
    for (auto& circle : g_circles) {
        circle.active = false;
    }

    std::vector<HANDLE> threadHandles;
    for (auto& circle : g_circles) {
        if (circle.threadHandle) {
            threadHandles.push_back(circle.threadHandle);
        }
    }
    LeaveCriticalSection(&g_cs);

    if (!threadHandles.empty()) {
        WaitForMultipleObjects(threadHandles.size(), threadHandles.data(), TRUE, 5000);
    }

    EnterCriticalSection(&g_cs);
    for (auto& circle : g_circles) {
        if (circle.threadHandle) {
            CloseHandle(circle.threadHandle);
            circle.threadHandle = nullptr;
        }
    }
    LeaveCriticalSection(&g_cs);

    DeleteCriticalSection(&g_cs);

    return static_cast<int>(msg.wParam);
}

DWORD WINAPI CircleThreadProc(LPVOID lpParam) {
    int circleIndex = *reinterpret_cast<int*>(lpParam);
    delete reinterpret_cast<int*>(lpParam);

    while (g_running) {
        EnterCriticalSection(&g_cs);

        if (circleIndex >= g_circles.size() || !g_circles[circleIndex].active) {
            LeaveCriticalSection(&g_cs);
            break;
        }

        Circle& circle = g_circles[circleIndex];

        RECT clientRect;
        GetClientRect(g_hwnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        circle.x += circle.dx;
        circle.y += circle.dy;

        if (circle.x - CIRCLE_RADIUS <= 0 || circle.x + CIRCLE_RADIUS >= width) {
            circle.dx = -circle.dx;
            circle.x = std::max(CIRCLE_RADIUS, std::min(circle.x, width - CIRCLE_RADIUS));
        }

        if (circle.y - CIRCLE_RADIUS <= 0 || circle.y + CIRCLE_RADIUS >= height) {
            circle.dy = -circle.dy;
            circle.y = std::max(CIRCLE_RADIUS, std::min(circle.y, height - CIRCLE_RADIUS));
        }

        LeaveCriticalSection(&g_cs);

        InvalidateRect(g_hwnd, nullptr, FALSE);

        Sleep(30);
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

            RECT clientRect;
            GetClientRect(hwindow, &clientRect);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

            FillRect(memDC, &clientRect, (HBRUSH)(COLOR_WINDOW + 1));

            EnterCriticalSection(&g_cs);

            for (const auto& circle : g_circles) {
                if (circle.active) {
                    HBRUSH brush = CreateSolidBrush(circle.color);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);
                    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
                    HPEN oldPen = (HPEN)SelectObject(memDC, pen);

                    Ellipse(memDC,
                            circle.x - CIRCLE_RADIUS,
                            circle.y - CIRCLE_RADIUS,
                            circle.x + CIRCLE_RADIUS,
                            circle.y + CIRCLE_RADIUS);

                    SelectObject(memDC, oldBrush);
                    SelectObject(memDC, oldPen);
                    DeleteObject(brush);
                    DeleteObject(pen);
                }
            }

            LeaveCriticalSection(&g_cs);

            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);

            EndPaint(hwindow, &ps);
            return 0;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                break;
            }

            RECT clientRect;
            GetClientRect(hwindow, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;

            if (width <= 2 * CIRCLE_RADIUS || height <= 2 * CIRCLE_RADIUS) {
                return 0;
            }

            Circle newCircle;
            newCircle.x = CIRCLE_RADIUS + rand() % (width - 2 * CIRCLE_RADIUS);
            newCircle.y = CIRCLE_RADIUS + rand() % (height - 2 * CIRCLE_RADIUS);

            int direction = rand() % 2;
            if (direction == 0) {
                newCircle.dx = (rand() % 2 == 0 ? 1 : -1) * MOVE_SPEED;
                newCircle.dy = 0;
            } else {
                newCircle.dx = 0;
                newCircle.dy = (rand() % 2 == 0 ? 1 : -1) * MOVE_SPEED;
            }

            newCircle.color = RGB(rand() % 256, rand() % 256, rand() % 256);
            newCircle.active = true;

            EnterCriticalSection(&g_cs);
            g_circles.push_back(newCircle);
            int* pIndex = new int(g_circles.size() - 1);
            g_circles.back().threadHandle = CreateThread(
                    nullptr, 0, CircleThreadProc, pIndex, 0, &g_circles.back().threadId);
            LeaveCriticalSection(&g_cs);

            return 0;
        }
        case WM_TIMER: {
            InvalidateRect(hwindow, nullptr, FALSE);
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
        case WM_CLOSE: {
            g_running = false;

            EnterCriticalSection(&g_cs);
            for (auto& circle : g_circles) {
                circle.active = false;
            }
            LeaveCriticalSection(&g_cs);

            Sleep(100);

            DestroyWindow(hwindow);
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
        default: ;
    }

    return DefWindowProcW(hwindow, message, wParam, lParam);
}