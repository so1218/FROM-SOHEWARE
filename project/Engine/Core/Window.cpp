#include "Window.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ウィンドウプロシージャ
LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    // メッセージに応じてゲーム固有の処理を行う
    switch (msg)
    {
        // ウィンドウが破壊された
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

void Window::Create(std::wstring windowTitle_)
{
    // ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = { 0,0,clientWidth_ ,clientHeight_ };

    // クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    WNDCLASS wc = {};
    // ウィンドウプロシージャ
    wc.lpfnWndProc = Window::WindowProc;
    // ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
    hInstance_ = GetModuleHandle(nullptr);
    // カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    // ウィンドウクラスを登録する
    RegisterClass(&wc);

    // ウィンドウの生成
    hwnd_ = CreateWindow(
        wc.lpszClassName,       // 利用するクラス名
        windowTitle_.c_str(),                 // タイトルバーの文字
        WS_OVERLAPPEDWINDOW,    // ウィンドウスタイル
        CW_USEDEFAULT,          // 表示X座標
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        hInstance_,
        nullptr
    );

    // ウィンドウを表示する
    ShowWindow(hwnd_, SW_SHOW);
}