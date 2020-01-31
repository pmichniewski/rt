#pragma once

#include "primitive.h"
#include "light.h"

#include <vector>
#include <memory>

class Scene
{
public:
	Scene(Primitive &aggregate, std::vector<std::unique_ptr<Light>> &lights)
		: m_aggregate(aggregate)
		, m_lights(lights)
		{}
	bool Intersect(const Ray &ray, Hit *hit);
	Primitive &m_aggregate;
	std::vector<std::unique_ptr<Light>> &m_lights;
};
