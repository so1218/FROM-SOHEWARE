#include "Line.h"
#include "Engine.h" 
#include "Math.h" 

Line::Line(Engine* engine)
    : engine_(engine)
{
    Initialize();
}

void Line::Initialize()
{
    transform_.Initialize();
}

void Line::Update()
{
    transform_.UpdateMatrix();
}

void Line::Draw()
{
    // ローカル座標を行列でワールド座標に変換する
    Vector3 worldStart = transform_.matWorld_.Transform(localStart_);
    Vector3 worldEnd = transform_.matWorld_.Transform(localEnd_);

    engine_->renderer_->SubmitLine(worldStart, worldEnd, color_);
}
