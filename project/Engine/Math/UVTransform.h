#pragma once
#include "Vector.h"
#include "Matrix.h"
#include "Quaternion.h"

namespace FS
{

// テクスチャの UV をずらす・回す・伸ばすための値
struct UVTransform
{
    Vector3 scale = { 1.0f, 1.0f, 1.0f };
    Vector3 rotation = { 0.0f, 0.0f, 0.0f };
    Vector3 translation = { 0.0f, 0.0f, 0.0f };

    Matrix4x4 MakeMatrix() const
    {
        return Matrix4x4::MakeAffine(scale, Quaternion::QuaternionFromEuler(rotation), translation);
    }
};

}
