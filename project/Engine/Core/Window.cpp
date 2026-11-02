#include "pch.h"
#include "Window.h"

#ifdef ENABLE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);
#endif

namespace FS
{

// ウィンドウプロシージャ
LRESULT CALLBACK Window::WindowProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
#ifdef ENABLE_IMGUI
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }
#endif

    // メッセージに応じてゲーム固有の処理を行う
    switch (msg)
    {

    case WM_DESTROY:

        // OSに対して、アプリの終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    // 標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Window::RegisterWindowClass()
{
    // ウィンドウクラスを登録
    WNDCLASS wc = {};
    // ウィンドウプロシージャ
    wc.lpfnWndProc = Window::WindowProc;
    // ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    // カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    // ウィンドウクラスを登録する
    RegisterClass(&wc);
}

void Window::Create(std::wstring windowTitle_, bool isFullscreen)
{
    WNDCLASS wc = {};
    // ウィンドウプロシージャ
    wc.lpfnWndProc = Window::WindowProc;
    // ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
    hInstance_ = GetModuleHandle(nullptr);
    // カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // 初期化中に背景を黒にする
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));

    // ウィンドウクラスを登録する
    RegisterClass(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT windowRect = { 0, 0, clientWidth_, clientHeight_ };

    if (isFullscreen)
    {
        // メインモニター全体を覆う枠なしのウィンドウ (ボーダーレスフルスクリーン)
        MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
        GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);
        style = WS_POPUP;
        windowRect = monitorInfo.rcMonitor;
    }
    else
    {
        // クライアント領域を元に実際のサイズにwrcを変更してもらう
        AdjustWindowRect(&windowRect, style, false);
    }

    // ウィンドウの生成
    hwnd_ = CreateWindow(
        wc.lpszClassName,       // 利用するクラス名
        windowTitle_.c_str(),   // タイトルバーの文字
        style,                  // ウィンドウスタイル
        isFullscreen ? windowRect.left : CW_USEDEFAULT,
        isFullscreen ? windowRect.top : CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance_,
        nullptr
    );

    // ウィンドウを表示する
    ShowWindow(hwnd_, SW_SHOW);

    // スワップチェーンはこの大きさで作る
    RECT clientRect{};
    GetClientRect(hwnd_, &clientRect);
    clientWidth_ = clientRect.right - clientRect.left;
    clientHeight_ = clientRect.bottom - clientRect.top;
}

}
