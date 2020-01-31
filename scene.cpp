#include "scene.h"

bool Scene::Intersect(const Ray &ray, Hit *hit)
{
	return m_aggregate.Intersect(ray, hit);
}