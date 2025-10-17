#pragma once

#define _USE_MATH_DEFINES
#include "Structures.h"


class ShapeGenerator
{
public:
    // 球を作成する関数
    static void SphereGenerator(std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices);
    // 立方体を作成する関数
    static void CubeGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);
    // スカイボックスを作成する関数
    static void SkyBoxGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);

    void GenerateSphere(
        std::vector<VertexData>& outVertices,
        uint32_t subdivision = 16);
};

