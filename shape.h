#pragma once

#include "ray.h"

class Shape
{
public:
	virtual bool intersect(Ray r, float &t) = 0;
};
