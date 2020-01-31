#pragma once

class Ray
{
public:
	Ray(vector3 o, vector3 d): origin(o), direction(d) {}

	vector3 origin;
	vector3 direction;
};
