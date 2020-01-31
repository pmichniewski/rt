#include "ray.h"

vector3 Ray::GetPoint()
{
	return origin * tMax;
}