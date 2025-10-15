#include "ShapeGenerator.h"

// 球を作成する関数
void ShapeGenerator::SphereGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices)
{
    const uint32_t kSubdivision = 16;
    const float kLonEvery = (2.0f * float(M_PI)) / float(kSubdivision);
    const float kLatEvery = (float(M_PI)) / float(kSubdivision);

    // 頂点生成
    for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) 
    {
        float lat = -float(M_PI) / 2.0f + kLatEvery * latIndex;
        for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex)
        {
            float lon = kLonEvery * lonIndex;

            Vector4 pos = { std::cosf(lat) * std::cosf(lon),std::sinf(lat),std::cosf(lat) * std::sinf(lon),1.0f };

            float u = float(lonIndex) / float(kSubdivision);
            float v = 1.0f - float(latIndex) / float(kSubdivision);

            // 法線は正規化された位置ベクトル
            Vector3 normal = { pos.x,pos.y,pos.z };
            float len = std::sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
            if (len > 0.0f) { normal.x /= len; normal.y /= len; normal.z /= len; }

            VertexData vertex;
            vertex.position = pos;
            vertex.texcoord = { u, v };
            vertex.normal = normal;

            vertices.push_back(vertex);
        }
    }

    // インデックス生成
    for (uint32_t lat = 0; lat < kSubdivision; ++lat)
    {
        for (uint32_t lon = 0; lon < kSubdivision; ++lon) 
        {
            uint32_t current = lat * (kSubdivision + 1) + lon;
            uint32_t next = current + kSubdivision + 1;

            // 三角形1
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            // 三角形2
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

void ShapeGenerator::CubeGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices) {
    // 立方体の各面の頂点とUV・法線
    static const VertexData cubeVertices[24] = 
    {
        // +X面
        {{+0.5f,-0.5f,-0.5f,1.0f},{0.0f,1.0f},{1,0,0}},
        {{+0.5f,+0.5f,-0.5f,1.0f},{0.0f,0.0f},{1,0,0}},
        {{+0.5f,+0.5f,+0.5f,1.0f},{1.0f,0.0f},{1,0,0}},
        {{+0.5f,-0.5f,+0.5f,1.0f},{1.0f,1.0f},{1,0,0}},
        // -X面
        {{-0.5f,-0.5f,+0.5f,1.0f},{0.0f,1.0f},{-1,0,0}},
        {{-0.5f,+0.5f,+0.5f,1.0f},{0.0f,0.0f},{-1,0,0}},
        {{-0.5f,+0.5f,-0.5f,1.0f},{1.0f,0.0f},{-1,0,0}},
        {{-0.5f,-0.5f,-0.5f,1.0f},{1.0f,1.0f},{-1,0,0}},
        // +Y面
        {{-0.5f,+0.5f,-0.5f,1.0f},{0.0f,1.0f},{0,1,0}},
        {{-0.5f,+0.5f,+0.5f,1.0f},{0.0f,0.0f},{0,1,0}},
        {{+0.5f,+0.5f,+0.5f,1.0f},{1.0f,0.0f},{0,1,0}},
        {{+0.5f,+0.5f,-0.5f,1.0f},{1.0f,1.0f},{0,1,0}},
        // -Y面
        {{-0.5f,-0.5f,+0.5f,1.0f},{0.0f,1.0f},{0,-1,0}},
        {{-0.5f,-0.5f,-0.5f,1.0f},{0.0f,0.0f},{0,-1,0}},
        {{+0.5f,-0.5f,-0.5f,1.0f},{1.0f,0.0f},{0,-1,0}},
        {{+0.5f,-0.5f,+0.5f,1.0f},{1.0f,1.0f},{0,-1,0}},
        // +Z面
        {{-0.5f,-0.5f,+0.5f,1.0f},{0.0f,1.0f},{0,0,1}},
        {{+0.5f,-0.5f,+0.5f,1.0f},{1.0f,1.0f},{0,0,1}},
        {{+0.5f,+0.5f,+0.5f,1.0f},{1.0f,0.0f},{0,0,1}},
        {{-0.5f,+0.5f,+0.5f,1.0f},{0.0f,0.0f},{0,0,1}},
        // -Z面
        {{+0.5f,-0.5f,-0.5f,1.0f},{0.0f,1.0f},{0,0,-1}},
        {{-0.5f,-0.5f,-0.5f,1.0f},{1.0f,1.0f},{0,0,-1}},
        {{-0.5f,+0.5f,-0.5f,1.0f},{1.0f,0.0f},{0,0,-1}},
        {{+0.5f,+0.5f,-0.5f,1.0f},{0.0f,0.0f},{0,0,-1}},
    };

    static const uint32_t cubeIndices[] = {
        0, 1, 2, 0, 2, 3,      // +X
        4, 5, 6, 4, 6, 7,      // -X
        8, 9,10, 8,10,11,      // +Y
       12,13,14,12,14,15,      // -Y
       16,17,18,16,18,19,      // +Z
       20,21,22,20,22,23       // -Z
    };

    vertices.assign(std::begin(cubeVertices), std::end(cubeVertices));
    indices.assign(std::begin(cubeIndices), std::end(cubeIndices));
}

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

void ShapeGenerator::GenerateSphere(
    std::vector<VertexData>& outVertices,
    uint32_t subdivision)
{
    outVertices.clear();
    outVertices.resize(subdivision * subdivision * 6);  // 必要サイズ確保
}