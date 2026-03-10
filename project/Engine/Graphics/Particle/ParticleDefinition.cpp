#include "pch.h"
#include "ParticleDefinition.h"
#include "AnimationModel.h" 

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
    case Type::Mesh:
    {
        // アニメーションモデルが渡された場合は、そのModelDataを使う
        const ModelData* targetModelData = nullptr;
        if (overrideAnimModel) {
            targetModelData = overrideAnimModel->GetModelData();
        }
        else {
            targetModelData = overrideModelData ? overrideModelData : sourceModelData;
        }

        if (!targetModelData || targetModelData->meshes.empty()) return { 0.0f, 0.0f, 0.0f };

        int randomMeshIndex = Math::RandomInt(0, (int)targetModelData->meshes.size() - 1);
        const auto& mesh = targetModelData->meshes[randomMeshIndex];

        if (mesh.indices.empty() || mesh.vertices.empty()) return { 0.0f, 0.0f, 0.0f };

        int triangleCount = (int)mesh.indices.size() / 3;
        int randomTri = Math::RandomInt(0, triangleCount - 1) * 3;

        // 頂点のインデックスを取得
        uint32_t indexA = mesh.indices[randomTri];
        uint32_t indexB = mesh.indices[randomTri + 1];
        uint32_t indexC = mesh.indices[randomTri + 2];

        Vector3 A, B, C;

        // アニメーションモデルならスキニング計算後の座標をもらう
        if (overrideAnimModel)
        {
            A = overrideAnimModel->GetSkinnedVertexPosition(randomMeshIndex, indexA);
            B = overrideAnimModel->GetSkinnedVertexPosition(randomMeshIndex, indexB);
            C = overrideAnimModel->GetSkinnedVertexPosition(randomMeshIndex, indexC);
        }
        else
        {
            A = { mesh.vertices[indexA].position.x, mesh.vertices[indexA].position.y, mesh.vertices[indexA].position.z };
            B = { mesh.vertices[indexB].position.x, mesh.vertices[indexB].position.y, mesh.vertices[indexB].position.z };
            C = { mesh.vertices[indexC].position.x, mesh.vertices[indexC].position.y, mesh.vertices[indexC].position.z };
        }

        // 重心座標系を使って、面上のランダムな点を計算
        float r1 = sqrtf(Math::RandomFloat(0.0f, 1.0f));
        float r2 = Math::RandomFloat(0.0f, 1.0f);

        float u = 1.0f - r1;
        float v = r1 * (1.0f - r2);
        float w = r1 * r2;

        // ここで計算されたローカル座標をそのまま返すだけでOK！
        return (A * u) + (B * v) + (C * w);
    }
    }

    return { 0.0f, 0.0f, 0.0f };
}