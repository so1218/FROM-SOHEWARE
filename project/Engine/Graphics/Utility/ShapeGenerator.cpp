#include "ShapeGenerator.h"

void ShapeGenerator::SkyBoxGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices)
{
    // 頂点座標（スカイボックスは通常法線・UV不要）
    const float size = 1.0f; // 2mの立方体

    // 頂点（外側から見えるように裏面を表示）
    const Vector4 positions[8] = 
    {
        {-size, -size, -size, 1.0f}, 
        { size, -size, -size, 1.0f}, 
        { size,  size, -size, 1.0f}, 
        {-size,  size, -size, 1.0f}, 
        {-size, -size,  size, 1.0f}, 
        { size, -size,  size, 1.0f}, 
        { size,  size,  size, 1.0f}, 
        {-size,  size,  size, 1.0f}, 
    };

    // 各頂点を追加
    vertices.clear();
    for (int i = 0; i < 8; ++i) 
    {
        VertexData v{};
        v.position = positions[i];
        v.normal = { 0, 0, 0 };       
        v.texcoord = { 0.0f, 0.0f };  
        vertices.push_back(v);
    }

    // 立方体の各面のインデックス（時計回り：裏面が見えるように）
    static const uint32_t skyboxIndices[] = {
        // -Z
        0, 2, 1, 0, 3, 2,
        // +Z
        4, 5, 6, 4, 6, 7,
        // -X
        0, 7, 3, 0, 4, 7,
        // +X
        1, 2, 6, 1, 6, 5,
        // -Y
        0, 1, 5, 0, 5, 4,
        // +Y
        3, 7, 6, 3, 6, 2
    };

    indices.assign(std::begin(skyboxIndices), std::end(skyboxIndices));
}
