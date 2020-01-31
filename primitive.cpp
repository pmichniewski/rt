#include "primitive.h"
#include "shape.h"
#include <iostream>

#include <memory>

GeometricPrimitive::GeometricPrimitive(std::unique_ptr<Shape> &&shape)
{
	m_shape = std::move(shape);
}

GeometricPrimitive::~GeometricPrimitive()
{
}

bool GeometricPrimitive::Intersect(const Ray &r, Hit *hit) const
{
	float tHit;
	if (!m_shape->Intersect(r, &tHit, hit)) return false;
	r.tMax = tHit;
	hit->primitive = this;
	return true;
}

LoosePrimitives::LoosePrimitives(std::vector<std::unique_ptr<Primitive>> &&prims)
{
	m_primitives = std::move(prims);
}

LoosePrimitives::~LoosePrimitives()
{
}

bool LoosePrimitives::Intersect(const Ray &r, Hit *hit) const
{
	bool result = false;
	for (auto &primitive: m_primitives)
	{
		if (primitive->Intersect(r, hit))
		{
			result = true;
		}
	}
	return result;
}
