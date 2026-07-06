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

void ShapeGenerator::CylinderGenerator(
    std::vector<VertexData>& vertices,
    std::vector<uint32_t>& indices,
    float radius,
    float height,
    uint32_t slices)
{
    vertices.clear();
    indices.clear();

    float halfHeight = height * 0.5f;

    // 側面の頂点を生成
    for (uint32_t i = 0; i <= 1; ++i)
    {
        float y = (i == 0) ? -halfHeight : halfHeight;
        float v = (i == 0) ? 1.0f : 0.0f;

        for (uint32_t j = 0; j <= slices; ++j)
        {
            float u = static_cast<float>(j) / slices;
            float theta = u * std::numbers::pi_v<float> *2.0f;

            float x = radius * std::cos(theta);
            float z = radius * std::sin(theta);

            VertexData vertex{};
            vertex.position = { x, y, z, 1.0f };
            vertex.normal = { std::cos(theta), 0.0f, std::sin(theta) }; 
            vertex.texcoord = { u, v };
            vertices.push_back(vertex);
        }
    }

    // 側面のインデックス
    uint32_t ringVertexCount = slices + 1;
    for (uint32_t j = 0; j < slices; ++j)
    {
        uint32_t bottom = j;
        uint32_t top = bottom + ringVertexCount;

        indices.push_back(bottom);
        indices.push_back(top);
        indices.push_back(bottom + 1);

        indices.push_back(bottom + 1);
        indices.push_back(top);
        indices.push_back(top + 1);
    }

}

void ShapeGenerator::RingGenerator(
    std::vector<VertexData>& vertices,
    std::vector<uint32_t>& indices,
    float outerRadius,
    float tubeRadius,
    uint32_t slices,
    uint32_t stacks)
{
    vertices.clear();
    indices.clear();

    // 頂点の生成
    for (uint32_t i = 0; i <= stacks; ++i)
    {
        float v = static_cast<float>(i) / stacks;
        float phi = v * std::numbers::pi_v<float> *2.0f; // チューブの断面の角度

        // 断面の円周上の位置
        float cosPhi = std::cos(phi);
        float sinPhi = std::sin(phi);

        for (uint32_t j = 0; j <= slices; ++j)
        {
            float u = static_cast<float>(j) / slices;
            float theta = u * std::numbers::pi_v<float> *2.0f; // リング全体の角度

            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);

            VertexData vertex{};

            // トーラスの座標計算
            float x = (outerRadius + tubeRadius * cosPhi) * cosTheta;
            float y = tubeRadius * sinPhi;
            float z = (outerRadius + tubeRadius * cosPhi) * sinTheta;
            vertex.position = { x, y, z, 1.0f };

            // 法線の計算
            Vector3 centerOfTube = { outerRadius * cosTheta, 0.0f, outerRadius * sinTheta };
            float nx = x - centerOfTube.x;
            float ny = y - centerOfTube.y;
            float nz = z - centerOfTube.z;

            // 正規化 (厳密にはチューブ半径で割るのと同じ)
            float length = std::sqrt(nx * nx + ny * ny + nz * nz);
            vertex.normal = { nx / length, ny / length, nz / length };

            vertex.texcoord = { u, v };
            vertices.push_back(vertex);
        }
    }

    // インデックスの生成
    for (uint32_t i = 0; i < stacks; ++i)
    {
        for (uint32_t j = 0; j < slices; ++j)
        {
            uint32_t p0 = i * (slices + 1) + j;
            uint32_t p1 = p0 + 1;
            uint32_t p2 = (i + 1) * (slices + 1) + j;
            uint32_t p3 = p2 + 1;

            indices.push_back(p0);
            indices.push_back(p1);
            indices.push_back(p2);

            indices.push_back(p1);
            indices.push_back(p3);
            indices.push_back(p2);
        }
    }
}

}