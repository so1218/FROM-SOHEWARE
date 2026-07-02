#include "pch.h"
#include "CameraRail.h"
#include "DebugDraw.h"
#include "CameraManager.h"

namespace FE
{

CameraRail::CameraRail(Engine* engine, Camera* targetCamera, const std::string& railName) :
    engine_(engine), targetCamera_(targetCamera), railName_(railName)
{
    binder_ = std::make_unique<PropertyBinder>(engine_, "CameraRail_" + railName_);
}

void CameraRail::Initialize()
{
    binder_->Bind("FrameCount", &frameCount_, 0);

    if (frameCount_ > 0)
    {
        keyframes_.resize(frameCount_);
        for (int i = 0; i < frameCount_; ++i)
        {
            std::string prefix = "Keyframe_" + std::to_string(i);

            binder_->Bind(prefix + "_Pos", &keyframes_[i].position, { 0,0,0 });
            binder_->BindRotation(prefix + "_Rot", &keyframes_[i].euler, &keyframes_[i].rotation);
            binder_->Bind(prefix + "_Time", &keyframes_[i].time, 1.0f);
            binder_->Bind(prefix + "_FOV", &keyframes_[i].fov, 0.45f);
            binder_->Bind(prefix + "_Wait", &keyframes_[i].waitTime, 0.0f);
        }
    }
}

void CameraRail::AddKeyframe(const CameraKeyframe& kf) 
{
    // ベクターに追加
    keyframes_.push_back(kf);
    frameCount_ = (int32_t)keyframes_.size();

    // ベクターのメモリ再配置が起きた可能性があるので、古いバインドを全てリセット
    binder_->Clear(true);

    // 全ての住所（ポインタ）を最初から教え直す
    binder_->Bind("FrameCount", &frameCount_, 0);

    for (size_t i = 0; i < keyframes_.size(); ++i)
    {
        std::string prefix = "Keyframe_" + std::to_string(i);

        // 既存の値を維持したままアドレスだけを再登録
        binder_->Bind(prefix + "_Pos", &keyframes_[i].position, keyframes_[i].position);
        binder_->BindRotation(prefix + "_Rot", &keyframes_[i].euler, &keyframes_[i].rotation);
        binder_->Bind(prefix + "_Time", &keyframes_[i].time, keyframes_[i].time);
        binder_->Bind(prefix + "_FOV", &keyframes_[i].fov, keyframes_[i].fov);
        binder_->Bind(prefix + "_Wait", &keyframes_[i].waitTime, keyframes_[i].waitTime);
    }
}

CameraKeyframe CameraRail::Evaluate(float currentTime) const
{
    // キーフレームが無い、または1つしかない場合の安全対策
    if (keyframes_.empty()) return CameraKeyframe{};
    if (keyframes_.size() == 1) return keyframes_[0];

    // 現在、どのインデックスにいるのかを計算
    float localTime = currentTime;
    size_t p1Index = 0;

    for (size_t i = 0; i < keyframes_.size() - 1; ++i)
    {
        // 待機時間の枠内かどうかを判定
        if (localTime <= keyframes_[i].waitTime)
        {
            // 待機中なので、その点の座標をそのまま返す（完全に静止）
            CameraKeyframe holdFrame = keyframes_[i];
            holdFrame.time = 0.0f; 
            return holdFrame;
        }
        // 待機時間を過ぎていたら、その分の時間を引く
        localTime -= keyframes_[i].waitTime;

        // 移動時間の枠内かどうかを判定
        if (localTime <= keyframes_[i].time)
        {
            p1Index = i;
            break;
        }
        // 移動時間もオーバーしていたら、次の区間へ
        localTime -= keyframes_[i].time;
        p1Index = i + 1;
    }

    // もし指定時間がレールの総時間を超えていたら、最後のキーフレームを返す
    if (p1Index >= keyframes_.size() - 1)
    {
        return keyframes_.back();
    }

    // その区間の中での進行度tを計算
    float segmentDuration = keyframes_[p1Index].time;
    float t = (segmentDuration > 0.0f) ? (localTime / segmentDuration) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    // Catmull-Rom補間に必要な4つの点を用意
    size_t p0Index = (p1Index > 0) ? p1Index - 1 : p1Index;
    size_t p2Index = p1Index + 1;
    size_t p3Index = (p2Index + 1 < keyframes_.size()) ? p2Index + 1 : p2Index;

    const CameraKeyframe& p0 = keyframes_[p0Index];
    const CameraKeyframe& p1 = keyframes_[p1Index];
    const CameraKeyframe& p2 = keyframes_[p2Index];
    const CameraKeyframe& p3 = keyframes_[p3Index];

    CameraKeyframe result;

    // Catmull-Romスプライン曲線の計算
    float t2 = t * t;
    float t3 = t2 * t;

    result.position = (
        (p1.position * 2.0f) +
        (p0.position * -1.0f + p2.position) * t +
        (p0.position * 2.0f - p1.position * 5.0f + p2.position * 4.0f - p3.position) * t2 +
        (p0.position * -1.0f + p1.position * 3.0f - p2.position * 3.0f + p3.position) * t3
        ) * 0.5f;

    // 球面線形補間 
    result.rotation = Quaternion::Slerp(p1.rotation, p2.rotation, t);

    // FOV:線形補間
    result.fov = p1.fov + (p2.fov - p1.fov) * t;

    result.time = 0.0f;

    return result;
}

float CameraRail::GetTotalTime() const 
{
    float total = 0.0f;
    if (keyframes_.empty()) return 0.0f;

    // 最後のキーフレームを除いた待機時間と移動時間の合計
    for (size_t i = 0; i < keyframes_.size() - 1; ++i)
    {
        total += keyframes_[i].waitTime; 
        total += keyframes_[i].time;
    }

    total += keyframes_.back().waitTime;

    return total;
}

bool CameraRail::DebugDraw()
{
    bool playRequested = false;
#ifdef IS_DEVELOPMENT
    ImGui::Begin("カメラレールエディタ");
    ImGui::Text("編集中のレール: %s", railName_.c_str());

    // プレビュー/シークバー
    static float previewTime = 0.0f;
    float totalTime = GetTotalTime();
    ImGui::SliderFloat("タイムライン再生", &previewTime, 0.0f, totalTime);

    if (ImGui::Button("現在の時間をカメラに適用"))
    {
        CameraKeyframe kf = Evaluate(previewTime);
        targetCamera_->SetTranslation(kf.position);
        targetCamera_->SetRotation(kf.rotation);
        targetCamera_->SetFov(kf.fov);
    }

    ImGui::Separator();
    if (ImGui::Button("現在のカメラ位置にキーフレームを追加"))
    {
        if (targetCamera_)
        {
            CameraKeyframe kf;
            kf.position = targetCamera_->GetTranslation();
            kf.rotation = targetCamera_->GetRotation();
            kf.euler = targetCamera_->GetWorldRotationEuler();
            kf.fov = targetCamera_->GetFov();
            kf.time = 2.0f; 
            kf.waitTime = 0.0f;

            AddKeyframe(kf);

            // JSONに現在の個数を保存
            GlobalVariables::GetInstance()->SetValue(binder_->GetGroupPath(), "FrameCount", frameCount_);
        }
    }

    ImGui::Separator();

    // ループの前に宣言しておく
    int deleteIndex = -1;

    for (size_t i = 0; i < keyframes_.size(); ++i)
    {
        std::string label = "フレーム [" + std::to_string(i) + "]";
        if (ImGui::CollapsingHeader(label.c_str())) {
            std::string prefix = "Keyframe_" + std::to_string(i);
            binder_->Draw(prefix + "_Pos", "座標");
            binder_->Draw(prefix + "_Rot", "回転");
            binder_->Draw(prefix + "_Time", "次の点への時間");
            binder_->Draw(prefix + "_FOV", "画角");
            binder_->Draw(prefix + "_Wait", "この点での待機時間");

            // 削除ボタンが押されたら、消す番号を記録
            if (ImGui::Button(("このフレームを削除##" + std::to_string(i)).c_str()))
            {
                deleteIndex = (int)i;
            }
        }
    }

    // ループを抜けた後で、削除と再構築を実行
    if (deleteIndex != -1)
    {
        // フレームを削除
        keyframes_.erase(keyframes_.begin() + deleteIndex);
        frameCount_ = (int32_t)keyframes_.size();

        // 古いデータをImGuiとGlobalVariablesから完全に消し去る
        binder_->Clear(true);

        // 再構築
        binder_->Bind("FrameCount", &frameCount_, 0);

        // 残ったキーフレームを0番から順番にBindし直す
        for (size_t i = 0; i < keyframes_.size(); ++i)
        {
            std::string prefix = "Keyframe_" + std::to_string(i);

            binder_->Bind(prefix + "_Pos", &keyframes_[i].position, keyframes_[i].position);
            binder_->BindRotation(prefix + "_Rot", &keyframes_[i].euler, &keyframes_[i].rotation);
            binder_->Bind(prefix + "_Time", &keyframes_[i].time, keyframes_[i].time);
            binder_->Bind(prefix + "_FOV", &keyframes_[i].fov, keyframes_[i].fov);
            binder_->Bind(prefix + "_Wait", &keyframes_[i].waitTime, keyframes_[i].waitTime);
        }

        // 新しい状態でJSONを上書き保存し、ゴーストデータを消滅
        GlobalVariables::GetInstance()->SaveFile(binder_->GetGroupPath());
    }

    ImGui::Separator();

    if (ImGui::Button("レールを再生"))
    {
        // ボタンが押されたらフラグを立てる
        if (keyframes_.size() >= 2)
        {
            playRequested = true;
        }
    }

    ImGui::End();
#endif

    return playRequested;
}

void CameraRail::DrawDebugSpline() const
{
    if (keyframes_.size() < 2) return;

    // レールの総時間を計算
    float totalTime = 0.0f;
    for (size_t i = 0; i < keyframes_.size() - 1; ++i)
    {
        totalTime += keyframes_[i].time;
    }

    // 色の定義
    Vector4 lineColor = { 0.0f, 1.0f, 0.0f, 1.0f };  
    Vector4 pointColor = { 1.0f, 0.0f, 0.0f, 1.0f }; 

    // 0.1秒刻みでEvaluateを呼び、点と点を線で結ぶ
    const float step = 0.1f;
    Vector3 prevPos = Evaluate(0.0f).position;

    for (float t = step; t <= totalTime; t += step)
    {
        Vector3 currentPos = Evaluate(t).position;

        // 線を描画
        DebugDraw::DrawLine(prevPos, currentPos, lineColor);

        prevPos = currentPos;
    }

    // ループの端数のズレを防ぐため、最後の隙間を結ぶ
    Vector3 lastPos = Evaluate(totalTime).position;
    DebugDraw::DrawLine(prevPos, lastPos, lineColor);

    // キーフレームの点自体も描画
    for (const auto& kf : keyframes_)
    {
        // コントロールポイントの位置を視覚化
        DebugDraw::DrawSphere(kf.position, 0.5f, pointColor);
    }
}

}