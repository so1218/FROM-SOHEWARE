#pragma once
#include "Structures.h"

namespace FE
{

class ShapeGenerator
{
public:
    // スカイボックスを作成する関数
    static void SkyBoxGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);
};

}

