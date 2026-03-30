#include "pch.h"
#include "ShapeGenerator.h"

namespace FE
{

void ShapeGenerator::SkyBoxGenerator(
    std::vector<VertexData>& vertices,
    std::vector<uint32_t>& indices
)
{
    // スカイボックス用立方体の半径
    const float size = 1.0f;

    // 立方体の頂点座標
    const Vector4 positions[8] =
    {
        { -size, -size, -size, 1.0f },
        {  size, -size, -size, 1.0f },
        {  size,  size, -size, 1.0f },
        { -size,  size, -size, 1.0f },
        { -size, -size,  size, 1.0f },
        {  size, -size,  size, 1.0f },
        {  size,  size,  size, 1.0f },
        { -size,  size,  size, 1.0f },
    };

    // 頂点データを作成
    vertices.clear();
    for (int i = 0; i < 8; ++i)
    {
        VertexData v{};
        v.position = positions[i];

        // スカイボックスでは法線とUVは使用しない
        v.normal = { 0, 0, 0 };
        v.texcoord = { 0.0f, 0.0f };

        vertices.push_back(v);
    }

    // 内側から表示されるようにインデックスを設定
    static const uint32_t skyboxIndices[] =
    {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        0, 7, 3, 0, 4, 7,
        1, 2, 6, 1, 6, 5,
        0, 1, 5, 0, 5, 4,
        3, 7, 6, 3, 6, 2
    };

    // インデックスをコピー
    indices.assign(
        std::begin(skyboxIndices),
        std::end(skyboxIndices)
    );
}

}