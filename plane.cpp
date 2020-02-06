#include "plane.h"
#include "math.h"

#include <iostream>

bool Plane::Intersect(const Ray &r, float *t, Hit *h)
{
	float denom = r.direction.dot(normal);
	if (std::abs(denom) > 1e-6f)
	{
		float t0 = -(r.origin.dot(normal) - d) / denom;
		if (t0 > 0.0f && t0 < r.tMax)
		{
			*t = t0;
			if (h)
			{
				h->position = r.origin + r.direction * (*t);
				h->normal = normal;
			}
			return true;
		}
	}
	return false;
}
