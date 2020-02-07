#include "scene.h"

Material *Scene::GetSkyMaterial() const
{
	return m_skyMaterial;
}

bool Scene::Intersect(const Ray &ray, Hit *hit) const
{
	return m_aggregate.Intersect(ray, hit);
}