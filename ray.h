#pragma once

#include "math.h"

#include <limits>

class Ray
{
public:
	Ray(vector3 o, vector3 d): origin(o), direction(d), tMax(std::numeric_limits<float>::infinity()) {}
	vector3 GetPoint();

	vector3 origin;
	vector3 direction;
	mutable float tMax;
};
