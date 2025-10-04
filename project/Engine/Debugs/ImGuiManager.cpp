#include "ImGuiManager.h"

#include <filesystem>

#include "externals/imgui/imgui_internal.h"
#include "externals/ImGuiFileDialog.h"
#define STB_IMAGE_IMPLEMENTATION
#include "externals/stb_image.h"

void ImGuiManager::Initialize(
    HWND hwnd,
    ID3D12Device* device,
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc,
    const DXGI_SWAP_CHAIN_DESC1& swapChainDesc,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // ドッキング有効化
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

    // フォント設定（日本語対応）
    std::string fontPath = "Resources/fonts/GenJyuuGothic-Bold.ttf";
    float fontSize = 16.0f;

    ImFontConfig font_config;
    static const ImWchar ranges[] = {
        0x0020, 0x00FF,   // 基本ラテン文字＋補助
        0x3040, 0x309F,   // ひらがな
        0x30A0, 0x30FF,   // カタカナ
        0x4E00, 0x9FFF,   // 漢字
        0xFF00, 0xFFEF,   // 半角・全角記号
        0,
    };

    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &font_config, ranges);

    if (!font) {
        io.Fonts->AddFontDefault();
        OutputDebugStringA("Failed to load font. Using default.\n");
    }

    ImGuiStyle& style = ImGui::GetStyle();

    // ダークテーマベース
    ImGui::StyleColorsDark();

    // 色設定
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.95f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

    // ボタン・スライダー
    style.Colors[ImGuiCol_Button] = ImVec4(0.03f, 0.06f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.15f, 0.4f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.01f, 0.04f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.15f, 0.22f, 0.4f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.07f, 0.18f, 0.35f, 1.0f);

    // 入力枠背景
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.2f, 0.7f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1f, 0.15f, 0.4f, 0.9f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.2f, 0.5f, 1.0f);

    // その他UI色
    style.Colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.35f, 0.25f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3f, 0.3f, 0.65f, 0.75f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.75f, 1.0f);

    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.95f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.01f, 0.01f, 0.05f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.03f, 0.03f, 0.1f, 1.0f);

    // スクロールバー
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.15f, 0.6f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.7f, 0.8f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4f, 0.4f, 0.85f, 0.9f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5f, 0.5f, 0.9f, 1.0f);

    // チェックボックス・ラジオボタン
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.4f, 0.8f, 1.0f);

    // ポップアップ・プログレスバー
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.03f, 0.03f, 0.03f, 0.95f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.15f, 0.25f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.20f, 0.35f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.12f, 0.22f, 0.40f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.18f, 0.30f, 0.50f, 1.0f);

    // ヘッダー（Disabled含む）
    style.Colors[ImGuiCol_Header] = ImVec4(0.05f, 0.05f, 0.2f, 0.7f);      
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1f, 0.15f, 0.4f, 0.9f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.2f, 0.5f, 1.0f); 

    // タブ
    style.Colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.10f, 0.25f, 1.0f);  
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.25f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.35f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.03f, 0.06f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.12f, 0.25f, 1.0f);

    // テキスト色
    style.Colors[ImGuiCol_Text] = ImVec4(0.85f, 0.9f, 1.0f, 0.85f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4f, 0.5f, 0.7f, 1.0f);

    // メニューバー背景
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.03f, 0.03f, 0.07f, 1.0f);

    // スタイル設定
    style.Alpha = 1.0f;
    style.WindowPadding = ImVec2(6, 6);
    style.FramePadding = ImVec2(4, 3);
    style.ItemSpacing = ImVec2(4, 4);
    style.ScrollbarSize = 10;
    style.GrabMinSize = 10;
    style.WindowRounding = 1.0f;
    style.FrameRounding = 10.0f;       // ボタン・スライダーの角丸
    style.ScrollbarRounding = 10.0f;   // スクロールバーの角丸
    style.GrabRounding = 10.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    io.FontGlobalScale = 1.0f;

    // ImGui初期化
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(device,
        swapChainDesc.BufferCount,
        rtvDesc.Format,
        srvDescriptorHeap,
        cpuHandle,
        gpuHandle);
}

void ImGuiManager::BeginFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //DrawMenuBar();

    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

      // 初回のみDock構造を作成
    static bool dockInitialized = false;
    if (!dockInitialized) {
        dockInitialized = true;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_down, dock_id_right;

        // 下30%を分割
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, &dock_id_down, &dock_main_id);

        // 右35%を分割、dock_main_idは残り部分
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.35f, &dock_id_right, &dock_main_id);

        // 割り当て
        ImGui::DockBuilderDockWindow("ログやデバッグ出力", dock_id_down);
        ImGui::DockBuilderDockWindow("全体のデバッグ情報", dock_id_down);
        ImGui::DockBuilderDockWindow("Scene", dock_main_id);
        ImGui::DockBuilderDockWindow("プレイヤー", dock_id_right);
        ImGui::DockBuilderDockWindow("敵", dock_id_right);
        ImGui::DockBuilderDockWindow("Global Variables", dock_id_down);
        ImGui::DockBuilderDockWindow("Ground", dock_id_down);
        ImGui::DockBuilderDockWindow("プレイシーン", dock_id_down);
        ImGui::DockBuilderDockWindow("FollowCamera", dock_id_down);
        ImGui::DockBuilderDockWindow("タイトルシーン", dock_id_down);
        ImGui::DockBuilderDockWindow("天球", dock_id_down);

        ImGui::DockBuilderFinish(dockspace_id);
    }


    // メインDockSpaceの背景ウィンドウを描画
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); 
    ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpaceを作成
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End(); // "DockSpaceWindow"
}

void ImGuiManager::DrawMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
            {
                // ファイルダイアログ表示フラグを立てる
                IGFD::FileDialogConfig config;
                config.path = ".";  // 初期ディレクトリ指定

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseFileDlgKey",
                    "Choose File",
                    ".png,.txt,.cpp,.h",
                    config);
            }
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                // セーブ処理を呼ぶ（後述）
                SaveFile();
            }
            if (ImGui::MenuItem("Exit"))
            {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Toggle Debug Panel")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
                // Aboutダイアログ表示など
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    //ファイルダイアログの表示処理
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
        // OKボタン押された場合
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

            // ここでファイルを開いて読み込みなどの処理
            OpenFile(filePathName);
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

void ImGuiManager::OpenFile(const std::string& filename)
{
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4); // RGBAに変換
    if (!data)
    {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return;
    }

    // 画像データを DirectX/OpenGL のテクスチャに変換して ImGui::Image() に渡す
    // ※この部分は使っているレンダラー（DirectX11, OpenGLなど）により異なります。

    // テクスチャ作成後は、dataは解放してOK
    stbi_image_free(data);
}

void ImGuiManager::SaveFile()
{
    // 実際のファイル保存処理
    // 例：保存ダイアログを開く・ファイル書き込み処理など
    // ここでは簡易的に固定のファイル名に保存
    std::string fileName = "output.txt";
    std::ofstream ofs(fileName);
    if (ofs.is_open())
    {
        ofs << "保存したいデータなどを書き込む\n";
        ofs.close();
        OutputDebugStringA("ファイルを保存しました\n");
    }
    else
    {
        OutputDebugStringA("ファイル保存に失敗しました\n");
    }
}

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList)
{
    // ImGuiの内部コマンドを生成する
    ImGui::Render();

    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
// ImGuiの終了処理
void ImGuiManager::Finalize()
{
    // ImGuiの終了処理。詳細はさして重要ではないので開設は省略する
    // 初期化と逆順に行う
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

