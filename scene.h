#pragma once

#include "primitive.h"
#include "light.h"

#include <vector>
#include <memory>

class Material;

class Scene
{
public:
	Scene(Primitive &aggregate, std::vector<std::unique_ptr<Light>> &lights)
		: m_aggregate(aggregate)
		, m_lights(lights)
		{}
	Material *GetSkyMaterial() const;
	bool Intersect(const Ray &ray, Hit *hit) const;
	Primitive &m_aggregate;
	std::vector<std::unique_ptr<Light>> &m_lights;
	Material *m_skyMaterial;
};
