#pragma once

#include "math.h"

class Light
{
public:
	Light(vector3 pos_, vector3 color_): pos(pos_), color(color_) {}

	vector3 pos;
	vector3 color;
};
