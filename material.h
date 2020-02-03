#include "math.h"

class Material
{
public:
	Material(vector3 color_, float roughness_, float metalness_): color(color_), roughness(roughness_), metalness(metalness_) {}

	vector3 color;
	float roughness;
	float metalness;
};

