#include "pch.h"
#include "ImGuiManager.h"
#include "WorldTransform.h"
#include "Camera.h"
#include "DebugDraw.h"

namespace FE
{

#ifdef ENABLE_IMGUI
bool ImGuiManager::showGui_ = true;
bool ImGuiManager::dockInitialized_ = false;
bool ImGuiManager::resetSceneSize_ = false;
int ImGuiManager::gizmoOperation_ = ImGuizmo::TRANSLATE;
Vector2 ImGuiManager::sceneRectMin_ = { 0.0f, 0.0f };
Vector2 ImGuiManager::sceneRectSize_ = { 0.0f, 0.0f };
bool ImGuiManager::isSceneHovered_ = false;
Matrix4x4 ImGuiManager::gizmoViewMatrix_ = Matrix4x4::MakeIdentity();
Matrix4x4 ImGuiManager::gizmoProjMatrix_ = Matrix4x4::MakeIdentity();
#endif

void ImGuiManager::Initialize(
    HWND hwnd,
    ID3D12Device* device,
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc,
    const DXGI_SWAP_CHAIN_DESC1& swapChainDesc,
    ID3D12DescriptorHeap* srvDescriptorHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
#ifdef ENABLE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // ドッキング有効化
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;

    io.IniFilename = "imgui_layout.ini";

    // フォント設定
    std::string fontPath = "Assets/Data/Fonts/NotoSansJP-Bold.ttf";
    float fontSize = 16.0f;

    ImFontConfig font_config;
    font_config.PixelSnapH = true; 

    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF,   // 基本ラテン文字＋補助
        0x3040, 0x309F,   // ひらがな
        0x30A0, 0x30FF,   // カタカナ
        0x4E00, 0x9FFF,   // 漢字
        0xFF00, 0xFFEF,   // 半角・全角記号
        0,
    };

    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &font_config, ranges);

    if (!font)
    {
        io.Fonts->AddFontDefault();
        OutputDebugStringA("Failed to load font. Using default.\n");
    }

    ImGuiStyle& style = ImGui::GetStyle();

    // ベーススタイルをクリア
    ImGui::StyleColorsDark();

    // =========================================================
    // カラーパレット
    // =========================================================
    const ImVec4 bgDark = ImVec4(0.03f, 0.03f, 0.03f, 0.95f); 
    const ImVec4 bgPanel = ImVec4(0.05f, 0.05f, 0.05f, 0.85f); 
    const ImVec4 textParchment = ImVec4(0.96f, 0.94f, 0.88f, 1.00f); 
    const ImVec4 textDisabled = ImVec4(0.45f, 0.43f, 0.40f, 1.00f); 

    const ImVec4 navyBase = ImVec4(0.03f, 0.07f, 0.15f, 1.00f); 
    const ImVec4 navyHover = ImVec4(0.06f, 0.12f, 0.25f, 1.00f); 
    const ImVec4 navyActive = ImVec4(0.10f, 0.18f, 0.35f, 1.00f);
    const ImVec4 navyBorder = ImVec4(0.08f, 0.14f, 0.26f, 0.85f);

    // ウィンドウ・背景
    style.Colors[ImGuiCol_WindowBg] = bgDark;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.96f);
    style.Colors[ImGuiCol_Border] = navyBorder;
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // フレーム
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f); 
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.04f, 0.08f, 0.16f, 0.90f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.06f, 0.12f, 0.22f, 0.95f);

    // タイトルバー・メニュー
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.07f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.02f, 0.02f, 0.02f, 0.75f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);

    // ボタン
    style.Colors[ImGuiCol_Button] = navyBase;
    style.Colors[ImGuiCol_ButtonHovered] = navyHover;
    style.Colors[ImGuiCol_ButtonActive] = navyActive;

    // ヘッダー
    style.Colors[ImGuiCol_Header] = ImVec4(0.04f, 0.08f, 0.16f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered] = navyHover;
    style.Colors[ImGuiCol_HeaderActive] = navyActive;

    // タブ
    style.Colors[ImGuiCol_Tab] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = navyHover;
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.05f, 0.10f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.03f, 0.03f, 0.03f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.03f, 0.06f, 0.12f, 1.00f);

    // スライダー・スプリッター・チェックマーク
    style.Colors[ImGuiCol_SliderGrab] = navyBase;
    style.Colors[ImGuiCol_SliderGrabActive] = navyActive;
    style.Colors[ImGuiCol_CheckMark] = navyActive;
    style.Colors[ImGuiCol_Separator] = navyBorder;
    style.Colors[ImGuiCol_SeparatorHovered] = navyHover;
    style.Colors[ImGuiCol_SeparatorActive] = navyActive;
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.04f, 0.08f, 0.16f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] = navyHover;
    style.Colors[ImGuiCol_ResizeGripActive] = navyActive;

    // スクロールバー
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.04f, 0.08f, 0.15f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = navyHover;
    style.Colors[ImGuiCol_ScrollbarGrabActive] = navyActive;

    // テキスト
    style.Colors[ImGuiCol_Text] = textParchment;
    style.Colors[ImGuiCol_TextDisabled] = textDisabled;

    // ドッキングプレビュー
    style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.08f, 0.14f, 0.26f, 0.40f);

    // =========================================================
    // レイアウト・丸み・枠線
    // =========================================================
    style.Alpha = 1.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(6, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 2.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 0.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

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
#endif
}

void ImGuiManager::BeginFrame()
{
#ifdef ENABLE_IMGUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ImGuizmoのフレーム開始処理
    ImGuizmo::BeginFrame();
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    // F1 キーで ImGui 全体を切り替え
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
    {
        showGui_ = !showGui_;
        DebugDraw::SetEnabled(showGui_);
    }

    // メニューバーは GUI 有効時のみ描画
    if (showGui_)
    {
        DrawMenuBar();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImVec2 dockPos = viewport->Pos;
    ImVec2 dockSize = viewport->Size;

    // メニューバーが表示されている場合のみ位置とサイズを調整
    if (showGui_ && ImGui::GetItemRectSize().y > 0.0f)
    {
        dockPos.y += ImGui::GetItemRectSize().y;
        dockSize.y -= ImGui::GetItemRectSize().y;
    }

    ImGuiIO& io = ImGui::GetIO();
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
        dockInitialized_ = true;
    }

    // DockSpace自体は非表示時でも維持し続ける
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

    // DockSpaceを作成
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::End();
#endif
}

void ImGuiManager::DrawMenuBar()
{
#ifdef ENABLE_IMGUI
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("ビュー"))
        {
            if (ImGui::MenuItem("レイアウトの初期化")) 
            {
                // .iniファイルを削除
                ImGuiIO& io = ImGui::GetIO();
                if (io.IniFilename != nullptr)
                {
                    std::filesystem::remove(io.IniFilename);
                }

                // 再ビルド
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

void ImGuiManager::EndFrame(ID3D12GraphicsCommandList* commandList)
{
#ifdef ENABLE_IMGUI
    // ImGuiの内部コマンドを生成する
    ImGui::Render();

    // 実際のcommandListのImGuiの描画コマンドを積む
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif
}
// ImGuiの終了処理
void ImGuiManager::Finalize()
{
#ifdef ENABLE_IMGUI
    // ImGuiの終了処理
    // 初期化と逆順に行う
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
}

void ImGuiManager::DrawGizmo(WorldTransform& transform)
{
#ifdef ENABLE_IMGUI
    // 操作モードの切り替え
    if (ImGui::IsKeyPressed(ImGuiKey_1)) gizmoOperation_ = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) gizmoOperation_ = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) gizmoOperation_ = ImGuizmo::SCALE;

    // 左手系を右手系にするため、View行列のZ列を反転
    Matrix4x4 rhViewMatrix = gizmoViewMatrix_;
    rhViewMatrix.m[0][2] = -rhViewMatrix.m[0][2];
    rhViewMatrix.m[1][2] = -rhViewMatrix.m[1][2];
    rhViewMatrix.m[2][2] = -rhViewMatrix.m[2][2];
    rhViewMatrix.m[3][2] = -rhViewMatrix.m[3][2];

    // 左手系を右手系にするため、Proj行列のZ列を反転
    Matrix4x4 rhProjMatrix = gizmoProjMatrix_;
    rhProjMatrix.m[2][0] = -rhProjMatrix.m[2][0];
    rhProjMatrix.m[2][1] = -rhProjMatrix.m[2][1];
    rhProjMatrix.m[2][2] = -rhProjMatrix.m[2][2];
    rhProjMatrix.m[2][3] = -rhProjMatrix.m[2][3];

    // Transform -> Matrix
    Matrix4x4 worldMatrix = Matrix4x4::MakeAffine(transform.scale_, transform.rotation_, transform.translation_);

    ImGuizmo::PushID(reinterpret_cast<void*>(&transform));

    ImGui::PushID(reinterpret_cast<void*>(&transform));

    // Gizmo表示
    ImGuizmo::Manipulate(
        &rhViewMatrix.m[0][0],
        &rhProjMatrix.m[0][0],
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

    ImGuizmo::PopID();

    ImGui::PopID();
#endif
}

bool ImGuiManager::DrawGizmoMatrix(Matrix4x4& worldMatrix)
{
#ifdef ENABLE_IMGUI
    // 操作モードの切り替え
    if (ImGui::IsKeyPressed(ImGuiKey_1)) gizmoOperation_ = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_2)) gizmoOperation_ = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_3)) gizmoOperation_ = ImGuizmo::SCALE;

    // 左手系を右手系にするため、View行列のZ列を反転
    Matrix4x4 rhViewMatrix = gizmoViewMatrix_;
    rhViewMatrix.m[0][2] = -rhViewMatrix.m[0][2];
    rhViewMatrix.m[1][2] = -rhViewMatrix.m[1][2];
    rhViewMatrix.m[2][2] = -rhViewMatrix.m[2][2];
    rhViewMatrix.m[3][2] = -rhViewMatrix.m[3][2];

    // 左手系を右手系にするため、Proj行列のZ列を反転
    Matrix4x4 rhProjMatrix = gizmoProjMatrix_;
    rhProjMatrix.m[2][0] = -rhProjMatrix.m[2][0];
    rhProjMatrix.m[2][1] = -rhProjMatrix.m[2][1];
    rhProjMatrix.m[2][2] = -rhProjMatrix.m[2][2];
    rhProjMatrix.m[2][3] = -rhProjMatrix.m[2][3];

    bool isEdited = false;
    ImGuizmo::Manipulate(
        &rhViewMatrix.m[0][0],
        &rhProjMatrix.m[0][0],
        (ImGuizmo::OPERATION)gizmoOperation_,
        ImGuizmo::WORLD,
        &worldMatrix.m[0][0]
    );

    if (ImGuizmo::IsUsing()) {
        isEdited = true;
    }
    return isEdited;
#endif
    return false;
}

void ImGuiManager::SetGizmoCamera(const Matrix4x4& view, const Matrix4x4& proj)
{
#ifdef ENABLE_IMGUI
    gizmoViewMatrix_ = view;
    gizmoProjMatrix_ = proj;
#endif
}

void ImGuiManager::SetSceneViewRect(const Vector2& min, const Vector2& size, bool isHovered)
{
#ifdef ENABLE_IMGUI
    sceneRectMin_ = min;
    sceneRectSize_ = size;
    isSceneHovered_ = isHovered;
#endif
}

}