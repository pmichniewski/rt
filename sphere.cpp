#include "sphere.h"
#include "hit.h"

#include <iostream>

bool Sphere::Intersect(const Ray &ray, float *t, Hit *h)
{
	vector3 L = ray.origin - m_center;
	float a = ray.direction.dot(ray.direction);
	float b = 2 * ray.direction.dot(L);
	float c = L.dot(L) - m_radius * m_radius;

	float delta = b * b - 4 * a * c;
	if (delta < 0.0f) return false;
	if (delta == 0.0f)
	{
		float t0 = -0.5f * b / a;
		if (t0 > ray.tMax)
		{
			return false;
		}
		*t = t0;
	}
	if (delta > 0.0f)
	{
		float q = (b > 0.0f) ?
			-0.5f * (b + sqrt(delta)) :
			-0.5f * (b - sqrt(delta));

		float t0 = q / a;
		float t1 = c / q;

		if (t0 > t1) std::swap(t0, t1);
		if (t0 <= 0.0f)
		{
			t0 = t1;
			if (t0 <= 0.0f) return false;
		}
		if (t0 >= ray.tMax)
		{
			return false;
		}
		*t = t0;
	}

	if (h)
	{
		h->position = ray.origin + ray.direction * (*t);
		h->normal = (h->position - m_center).normalized();
	}

	return true;
}
