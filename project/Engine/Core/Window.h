#pragma once

class Window
{
public:
    Window(int width, int height)
        : clientWidth_(width), clientHeight_(height) {}

	// ウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void RegisterWindowClass();
    void Create(std::wstring windowTitle_);

    // ゲッター
    HINSTANCE GetHInstance() const { return hInstance_; }
    HWND GetHwnd() const { return hwnd_; }

private:
    int width_;
    int height_;
    std::wstring title_;
    HWND hwnd_;
    HINSTANCE hInstance_;

    int clientWidth_;
    int clientHeight_;
};

