#pragma once
#include "Vector3.h"

namespace FE
{

class CameraPath
{
public:
	std::vector<Vector3> points;
	bool isLoop = false;

	/*Vector3 GetPositionAt(float t) const;*/
};

}