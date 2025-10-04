#pragma once

#include "Vector.h"
#include "Matrix.h"
#include "Structures.h"
#include "Mesh.h"

struct RenderData
{
	Mesh mesh;
	MaterialHandle materialHandle;
	Matrix4x4 worldMatrix;
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	TransformationMatrix* mappedData = nullptr;
};