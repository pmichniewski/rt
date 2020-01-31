#pragma once

#include "math.h"
#include "shape.h"
#include "ray.h"

class Sphere: public Shape
{
public:
	Sphere(vector3 center, float radius): m_center(center), m_radius(radius) {}

	vector3 m_center;
	float m_radius;
	bool intersect(Ray ray, float &t) override;
};

bool Sphere::intersect(Ray ray, float &t)
{
	vector3 L = ray.origin - m_center;
	float a = ray.direction.dot(ray.direction);
	float b = 2 * ray.direction.dot(L);
	float c = L.dot(L) - m_radius * m_radius;

	float delta = b * b - 4 * a * c;
	if (delta < 0.0f) return false;
	if (delta == 0.0f)
	{
		t = -0.5f * b / a;
		return true;
	}
	if (delta > 0.0f)
	{
		float q = (b > 0.0f) ?
			-0.5f * (b + sqrt(delta)) :
			-0.5f * (b - sqrt(delta));

		float t0 = q / a;
		float t1 = c / q;

		if (t0 > t1) std::swap(t0, t1);
		if (t0 < 0.0f)
		{
			t0 = t1;
			if (t0 < 0.0f) return false;
		}

		t = t0;
		return true;
	}
}
