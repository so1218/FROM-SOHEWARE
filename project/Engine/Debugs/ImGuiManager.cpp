#include "ImGuiManager.h"

#include <filesystem>

#include "imgui_internal.h"
#include "externals/ImGuiFileDialog.h"
#include "WorldTransform.h"
#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include "externals/stb_image.h"

bool ImGuiManager::dockInitialized_ = false;
bool ImGuiManager::resetSceneSize_ = false;
int ImGuiManager::gizmoOperation_ = ImGuizmo::TRANSLATE;

void ImGuiManager::Initialize(
    HWND hwnd,
    ID3D12Device* device,
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc,
    const DXGI_SWAP_CHAIN_DESC1& swapChainDesc,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
#ifdef _DEBUG
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // ドッキング有効化
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

    io.IniFilename = "imgui_layout.ini";

    // フォント設定
    std::string fontPath = "Assets/Fonts/GenJyuuGothic-Bold.ttf";
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
    style.FrameRounding = 10.0f;       
    style.ScrollbarRounding = 10.0f;   
    style.GrabRounding = 10.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    io.FontGlobalScale = 16.0f / fontSize;

    // ImGui初期化
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(device,
        swapChainDesc.BufferCount,
        rtvDesc.Format,
        srvDescriptorHeap,
        cpuHandle,
        gpuHandle);
#endif
}

void ImGuiManager::BeginFrame()
{
#ifdef _DEBUG
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ImGuizmoのフレーム開始処理
    ImGuizmo::BeginFrame();
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext()); // コンテキスト設定

    // Gizmoを描画する画面範囲を指定（画面全体に設定）
    ImGuiIO& io = ImGui::GetIO();

    DrawMenuBar();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImVec2 dockPos = viewport->Pos;
    ImVec2 dockSize = viewport->Size;

    if (ImGui::GetItemRectSize().y > 0.0f)
    {
        dockPos.y += ImGui::GetItemRectSize().y;    // Y座標をメニューバーのぶん下げる
        dockSize.y -= ImGui::GetItemRectSize().y;   // 高さをメニューバーのぶん縮める
    }

    bool iniFileExists = (io.IniFilename != nullptr && std::filesystem::exists(io.IniFilename));

      // 初回のみDock構造を作成
    if (!dockInitialized_ && !iniFileExists) {
        dockInitialized_ = true;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, dockSize);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_down, dock_id_right;

        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.35f, &dock_id_right, &dock_main_id);

        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.3f, &dock_id_down, &dock_main_id);

        // 割り当て
        ImGui::DockBuilderDockWindow("全体のデバッグ情報・設定", dock_id_down);
        ImGui::DockBuilderDockWindow("Scene", dock_main_id);
        ImGui::DockBuilderDockWindow("パーティクルエディター", dock_id_down);
        ImGui::DockBuilderDockWindow("Global Variables", dock_id_down);
        ImGui::DockBuilderDockWindow("シーンの選択", dock_id_down);

        ImGui::DockBuilderFinish(dockspace_id);
    }
    else if (!dockInitialized_ && iniFileExists)
    {
        dockInitialized_ = true; // .iniから読み込んだので組んだ扱い
    }

    // メインDockSpaceの背景ウィンドウを描画
    ImGui::SetNextWindowPos(dockPos);  
    ImGui::SetNextWindowSize(dockSize);
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

  /*  ImGui::ShowStyleEditor();*/

    // DockSpaceを作成
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End(); 
#endif
}

void ImGuiManager::DrawMenuBar()
{
#ifdef _DEBUG
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("ファイル"))
        {
            if (ImGui::MenuItem("終了"))
            {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("ビュー"))
        {
            if (ImGui::MenuItem("レイアウトの初期化")) 
            {
                // .iniファイルを削除する
                ImGuiIO& io = ImGui::GetIO();
                if (io.IniFilename != nullptr)
                {
                    std::filesystem::remove(io.IniFilename);
                }

                // 再ビルドを強制する
                dockInitialized_ = false;
            }

            if (ImGui::MenuItem("シーン表示サイズのリセット"))
            {
                resetSceneSize_ = true; 
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#endif
}

void ImGuiManager::OpenFile(const std::string& filename)
{
#ifdef _DEBUG
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4); 
    if (!data)
    {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return;
    }

    // テクスチャ作成後は、dataは解放
    stbi_image_free(data);
#endif
}

void ImGuiManager::SaveFile(const std::string& filename)
{
    // 実際のファイル保存処理
    std::ofstream ofs(filename);
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
#ifdef _DEBUG
    // ImGuiの内部コマンドを生成する
    ImGui::Render();

    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}
// ImGuiの終了処理
void ImGuiManager::Finalize()
{
#ifdef _DEBUG
    // ImGuiの終了処理
    // 初期化と逆順に行う
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
}

void ImGuiManager::DrawGizmo(WorldTransform& transform, const Camera& camera)
{
    // 操作モードの切り替え
    if (ImGui::IsKeyPressed(ImGuiKey_1)) gizmoOperation_ = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) gizmoOperation_ = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) gizmoOperation_ = ImGuizmo::SCALE;

    // 行列の準備
    const Matrix4x4& viewMatrix = camera.GetViewMatrix();
    const Matrix4x4& projMatrix = camera.GetProjectionMatrix();

    // Transform -> Matrix
    Matrix4x4 worldMatrix = Matrix4x4::MakeAffine(transform.scale_, transform.rotation_, transform.translation_);

    ImGui::PushID(reinterpret_cast<void*>(&transform));

    // Gizmo表示
    ImGuizmo::Manipulate(
        &viewMatrix.m[0][0],
        &projMatrix.m[0][0],
        (ImGuizmo::OPERATION)gizmoOperation_,
        ImGuizmo::WORLD,
        &worldMatrix.m[0][0]
    );

    // 数値の書き戻し
    if (ImGuizmo::IsUsing())
    {
        Vector3 translation, rotationDeg, scale;
        ImGuizmo::DecomposeMatrixToComponents(
            &worldMatrix.m[0][0],
            &translation.x,
            &rotationDeg.x,
            &scale.x
        );

        // 結果をTransformに反映
        transform.translation_ = translation;
        transform.scale_ = scale;

        float toRadian = Math::PI / 180.0f;
        transform.rotation_.x = rotationDeg.x * toRadian;
        transform.rotation_.y = rotationDeg.y * toRadian;
        transform.rotation_.z = rotationDeg.z * toRadian;
		transform.rotationQuaternion_ = Quaternion::QuaternionFromEuler(transform.rotation_);
    }

    ImGui::PopID();
}

