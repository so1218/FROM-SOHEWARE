#include "pch.h"
#include "Line.h"
#include "Engine.h" 
#include "Math.h" 

namespace FS
{

Line::Line(Engine* engine)
    : engine_(engine)
{
    Initialize();
}

void Line::Initialize()
{
    transform_.Initialize();
}

void Line::Draw()
{
    // ローカル座標を行列でワールド座標に変換
    Vector3 worldStart = transform_.GetWorldMatrix().Transform(localStart_);
    Vector3 worldEnd = transform_.GetWorldMatrix().Transform(localEnd_);

    engine_->GetRendererManager()->SubmitLine(worldStart, worldEnd, color_);
}

}