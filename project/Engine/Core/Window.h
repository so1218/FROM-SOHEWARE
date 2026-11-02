#pragma once

namespace FS
{

class Window
{
public:
    // width, height は窓のときのクライアント領域の大きさ
    Window(int width, int height)
        : clientWidth_(width), clientHeight_(height) {}

	// ウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

	void RegisterWindowClass();
    // isFullscreen なら、メインモニター全体を覆う枠なしのウィンドウにする
    void Create(std::wstring windowTitle_, bool isFullscreen);

    // ゲッター
    HINSTANCE GetHInstance() const { return hInstance_; }
    HWND GetHwnd() const { return hwnd_; }
    // 実際のクライアント領域の大きさ (フルスクリーンならモニターの大きさ)
    int GetClientWidth() const { return clientWidth_; }
    int GetClientHeight() const { return clientHeight_; }

private:
    std::wstring title_;
    HWND hwnd_;
    HINSTANCE hInstance_;

    int clientWidth_;
    int clientHeight_;
};

}

