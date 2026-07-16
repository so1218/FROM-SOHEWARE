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

void ShapeGenerator::SkydomeGenerator(
    std::vector<VertexData>& vertices,
    std::vector<uint32_t>& indices,
    uint32_t slices,
    uint32_t stacks)
{
    vertices.clear();
    indices.clear();

    const float radius = 1.0f;

    // 頂点データの生成
    for (uint32_t i = 0; i <= stacks; ++i)
    {
        float v = static_cast<float>(i) / stacks;
        float phi = v * std::numbers::pi_v<float>;

        for (uint32_t j = 0; j <= slices; ++j)
        {
            float u = static_cast<float>(j) / slices;
            float theta = u * std::numbers::pi_v<float> *2.0f;

            VertexData vertex{};

            // 球面の座標計算
            float x = radius * std::sin(phi) * std::cos(theta);
            float y = radius * std::cos(phi);
            float z = radius * std::sin(phi) * std::sin(theta);
            vertex.position = { x, y, z, 1.0f };

            // 法線は中心からの方向
            vertex.normal = { x, y, z };

            // 雲を流すためのUV座標（球の極座標をそのまま2DのUVにマッピング）
            vertex.texcoord = { u, v };

            vertices.push_back(vertex);
        }
    }

    // インデックスデータの生成
    for (uint32_t i = 0; i < stacks; ++i)
    {
        for (uint32_t j = 0; j < slices; ++j)
        {
            uint32_t p0 = i * (slices + 1) + j;
            uint32_t p1 = p0 + 1;
            uint32_t p2 = (i + 1) * (slices + 1) + j;
            uint32_t p3 = p2 + 1;

            // 外側向きの三角形(内側から見ると描画される）
            indices.push_back(p0);
            indices.push_back(p2);
            indices.push_back(p1);

            indices.push_back(p1);
            indices.push_back(p2);
            indices.push_back(p3);
        }
    }
}


}