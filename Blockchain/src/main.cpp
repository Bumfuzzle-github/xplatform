#include "core/Miner.hpp"

namespace {
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr const wchar_t* APP_TITLE = L"BLOCKCHAIN";

    std::wstring LoadStringResource(HINSTANCE hInstance, int resourceID) {
        wchar_t buffer[MAX_LOADSTRING];
        LoadStringW(hInstance, resourceID, buffer, MAX_LOADSTRING);
        return std::wstring(buffer);
    }

    ATOM RegisterMyClass(HINSTANCE hInstance, WNDPROC wndProc, const wchar_t* className) {
        WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = wndProc;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BAL));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = className;
        wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
        return RegisterClassExW(&wcex);
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
        case WM_COMMAND: {
            const int wmId = LOWORD(wParam);
            switch (wmId) {
            case IDM_ABOUT:
                DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, [](HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) -> INT_PTR {
                    switch (msg) {
                    case WM_INITDIALOG:
                        return TRUE;
                    case WM_COMMAND:
                        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                            EndDialog(hDlg, LOWORD(wParam));
                            return TRUE;
                        }
                        break;
                    }
                    return FALSE;
                });
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            static HFONT hFont = nullptr;
            if (!hFont)
                hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, 0x000000);
            SIZE textSize;
            std::wstring title(APP_TITLE);
            GetTextExtentPoint32W(hdc, title.c_str(), title.length(), &textSize);
            int x = (WINDOW_WIDTH - textSize.cx) / 2;
            int y = (WINDOW_HEIGHT - textSize.cy) / 2;
            TextOutW(hdc, x, y, title.c_str(), title.length());
            EndPaint(hWnd, &ps);
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    std::wstring appTitle = LoadStringResource(hInstance, IDS_APP_TITLE);
    std::wstring className = LoadStringResource(hInstance, IDC_BAL);
    
    if (!RegisterMyClass(hInstance, WndProc, className.c_str()))
        return 0;

    HWND hWnd = CreateWindowW(className.c_str(), APP_TITLE, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SimpleWeb::Server<SimpleWeb::HTTP> server;
    std::vector<int> peers;
    Core::BlockChain blockchain;
    auto miner = std::make_unique<Core::Miner>(std::make_shared<SimpleWeb::Server<SimpleWeb::HTTP>>(), peers, blockchain);

    std::thread serverThread([&]() { server.start(); });
    std::thread procThread([&]() { miner->process_input(hWnd, peers, blockchain); });

    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BAL));

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    server.stop();
    procThread.join();
    serverThread.join();

    return static_cast<int>(msg.wParam);
}
