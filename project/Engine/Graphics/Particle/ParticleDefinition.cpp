#include "pch.h"
#include "ParticleDefinition.h"
#include "AnimationModel.h" 

namespace FE
{

Vector3 ShapeModule::GetInitialPositionOffset(const ModelData* overrideModelData, const AnimationModel* overrideAnimModel) const
{
    switch (type)
    {
    case Type::Point:
        return { 0.0f, 0.0f, 0.0f }; // 原点のみ

    case Type::Box:
        // ボックス内部のランダムな位置
        return {
            Math::RandomFloat(-boxSize.x / 2.0f, boxSize.x / 2.0f),
            Math::RandomFloat(-boxSize.y / 2.0f, boxSize.y / 2.0f),
            Math::RandomFloat(-boxSize.z / 2.0f, boxSize.z / 2.0f)
        };

    case Type::Sphere:
    {
        // 単位球上のランダムな点を生成
        float phi = Math::RandomFloat(0.0f, 2.0f * 3.14159f);
        float cosTheta = Math::RandomFloat(-1.0f, 1.0f);
        float theta = acosf(cosTheta);

        Vector3 unitSpherePoint = {
            sinf(theta) * cosf(phi),
            sinf(theta) * sinf(phi),
            cosf(theta)
        };

        // 半径0の軸は0に固定
        if (radius.x == 0.0f) unitSpherePoint.x = 0.0f;
        if (radius.y == 0.0f) unitSpherePoint.y = 0.0f;
        if (radius.z == 0.0f) unitSpherePoint.z = 0.0f;

        // 単位ベクトル化（縁上に配置）
        unitSpherePoint = unitSpherePoint.Normalize();

        // 各軸に沿って拡大（楕円体化）
        Vector3 ellipsoidPoint = {
            unitSpherePoint.x * radius.x,
            unitSpherePoint.y * radius.y,
            unitSpherePoint.z * radius.z
        };

        // emitFromEdgeがfalseなら中心寄りに縮小
        if (!emitFromEdge)
            ellipsoidPoint = ellipsoidPoint * cbrtf(Math::RandomFloat(0.0f, 1.0f));

        return ellipsoidPoint;
    }
    }

    return { 0.0f, 0.0f, 0.0f };
}

}