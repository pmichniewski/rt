#pragma once

#include "math.h"
#include "shape.h"
#include "ray.h"

class Sphere: public Shape
{
public:
	Sphere(vector3 center, float radius): m_center(center), m_radius(radius) {}
	~Sphere() override {}
	bool Intersect(const Ray &r, float *t, Hit *h) override;

	vector3 m_center;
	float m_radius;
};
