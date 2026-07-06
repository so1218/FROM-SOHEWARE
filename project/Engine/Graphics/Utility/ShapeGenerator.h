#pragma once
#include "Structures.h"

namespace FE
{

class ShapeGenerator
{
public:
    // スカイボックスを作成する関数
    static void SkyBoxGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);

    // スカイドームを作成する関数
    static void SkydomeGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices, uint32_t slices = 32, uint32_t stacks = 16);

    // 円柱を作成する関数
    static void CylinderGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices, float radius = 1.0f, float height = 2.0f, uint32_t slices = 32);

    // リングを作成する関数
    static void RingGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices, float outerRadius = 1.0f, float tubeRadius = 0.3f, uint32_t slices = 32, uint32_t stacks = 16);
};

}

