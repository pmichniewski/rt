#pragma once

#include "ray.h"
#include "hit.h"

class Shape
{
public:
	Shape() {}
	virtual ~Shape() {}
	virtual bool Intersect(const Ray &r, float *t, Hit *h) = 0;
};
