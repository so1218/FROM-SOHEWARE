#pragma once

#define _USE_MATH_DEFINES
#include "Structures.h"


class ShapeGenerator
{
public:
    // スカイボックスを作成する関数
    static void SkyBoxGenerator(std::vector<VertexData>& vertices, std::vector<uint32_t>& indices);
};

