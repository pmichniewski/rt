#pragma once

#include "math.h"

class Light
{
public:
	Light(vector3 pos_, vector3 color_, float strength_): pos(pos_), color(color_), strength(strength_) {}

	vector3 pos;
	vector3 color;
	float strength;
};
