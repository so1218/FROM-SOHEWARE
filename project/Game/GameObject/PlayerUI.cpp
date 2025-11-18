#include "PlayerUI.h"
#include "TextureHandle.h"

void PlayerUI::Initialize()
{
    // ゲージ背景
    xpBarBgSprite_ = std::make_unique<Sprite>(engine_);
    xpBarBgSprite_->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));
    xpBarBgSprite_->SetPosition({ (1280.0f - 800.0f) / 2.0f, 10.0f });
    xpBarBgSprite_->SetSize({ 800.0f, 20.0f });
    xpBarBgSprite_->SetColor(0x444444FF);

    // ゲージ本体
    xpBarSprite_ = std::make_unique<Sprite>(engine_);
    xpBarSprite_->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));
    xpBarSprite_->SetPosition({ (1280.0f - 800.0f) / 2.0f, 10.0f });
    xpBarSprite_->SetSize({ 0.0f, 20.0f });
    xpBarSprite_->SetColor(0x00FF00FF);

    // HPバー背景
    hpBarBgSprite_ = std::make_unique<Sprite>(engine_);
    hpBarBgSprite_->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));
    hpBarBgSprite_->SetSize(kHpBarSize_);
    hpBarBgSprite_->SetColor(0x330000FF); 

    // HPバー本体
    hpBarSprite_ = std::make_unique<Sprite>(engine_);
    hpBarSprite_->SetTextureHandle(TextureHandle::Get(TextureID::white1x1));
    hpBarSprite_->SetSize(kHpBarSize_);  
    hpBarSprite_->SetColor(0xFF0000FF);

    std::array<TextureID, 10> idMap =
    {
        TextureID::num0,
        TextureID::num1,
        TextureID::num2,
        TextureID::num3,
        TextureID::num4,
        TextureID::num5,
        TextureID::num6,
        TextureID::num7,
        TextureID::num8,
        TextureID::num9 
    };

    for (int i = 0; i < 10; ++i)
    {
        digitTextureHandles_[i] = TextureHandle::Get(idMap[i]);
    }

    // 表示更新用の変数を初期化
    currentDisplayLevel_ = -1;
    levelNumberSprites_.clear();
}

void PlayerUI::Update()
{
    if (!player_) return;

    // Playerから割合を取得して長さを計算
    float ratio = player_->GetXpRatio();
    float currentWidth = 800.0f * std::clamp(ratio, 0.0f, 1.0f);

    xpBarSprite_->SetSize({ currentWidth, 20.0f });

    // HPバーの長さを更新
    float hpRatio = player_->GetHpRatio(); 
    float currentHPWidth = kHpBarSize_.x * std::clamp(hpRatio, 0.0f, 1.0f);
    hpBarSprite_->SetSize({ currentHPWidth, kHpBarSize_.y });


    // 3D位置から2Dスクリーン位置への変換

    // Playerのワールド座標を取得
    Vector3 playerPos = player_->GetWorldPosition();

    playerPos.y += kHpBarOffsetHeight_;

    // ビュープロジェクション行列を取得
    Matrix4x4 viewProjection = player_->GetCamera()->GetViewProjectionMatrix();

    // スクリーン座標に変換 
    Vector2 screenPos = Math::WorldToScreen(playerPos, viewProjection, kClientWidth, kClientHeight);

    // スプライトの位置設定

    // バーの幅の半分だけ左にずらす
    Vector2 centeredPosBg = { screenPos.x - kHpBarSize_.x / 2.0f, screenPos.y };
    Vector2 centeredPosFg = { screenPos.x - kHpBarSize_.x / 2.0f, screenPos.y };

    hpBarBgSprite_->SetPosition(centeredPosBg);
    hpBarSprite_->SetPosition(centeredPosFg);

    // レベル数値の更新処理
    int currentLevel = player_->GetLevel();

    // レベルが前回と変わった場合のみ、スプライトを作り直す
    if (currentDisplayLevel_ != currentLevel)
    {
        currentDisplayLevel_ = currentLevel;

        // 古いスプライトを全消去
        levelNumberSprites_.clear();

        // 数値を文字列に変換 ("12" など)
        std::string levelStr = std::to_string(currentLevel);

        // 文字列の各桁をループ処理
        for (size_t i = 0; i < levelStr.size(); ++i)
        {
            // 文字 '0'～'9' を 整数 0～9 に変換
            int digit = levelStr[i] - '0';

            // 新しいスプライトを作成
            auto sprite = std::make_unique<Sprite>(engine_);

            // 対応する数字のテクスチャをセット
            sprite->SetTextureHandle(digitTextureHandles_[digit]);

            // 座標計算 (開始位置 + インデックス * 間隔)
            Vector2 pos = kLevelNumberPos_;
            pos.x += i * kNumberSpace_;

            sprite->SetPosition(pos);
            sprite->SetSize(kNumberSize_);
            sprite->SetColor(0xFFFFFFFF); 

            // リストに追加
            levelNumberSprites_.push_back(std::move(sprite));
        }
    }
}

void PlayerUI::Draw()
{
    xpBarBgSprite_->Draw();
    xpBarSprite_->Draw();

    hpBarBgSprite_->Draw();
    hpBarSprite_->Draw();

    for (auto& sprite : levelNumberSprites_)
    {
        sprite->Draw();
    }
}