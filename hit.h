#pragma once

#include "primitive.h"
#include "math.h"

class Primitive;

class Hit
{
public:
	vector3 position;
	vector3 normal;
	const Primitive *primitive = nullptr;
};