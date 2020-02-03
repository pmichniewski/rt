#pragma once

#include "ray.h"
#include "hit.h"
#include "shape.h"

#include <memory>
#include <vector>

class Hit;
class Shape;
class Material;

class Primitive
{
public:
	virtual ~Primitive() {};
	virtual bool Intersect(const Ray &r, Hit *hit) const = 0;
	virtual Material *GetMaterial() const = 0;
};

class GeometricPrimitive: public Primitive
{
public:
	GeometricPrimitive(std::unique_ptr<Shape> &&shape, Material *m);
	~GeometricPrimitive() override;
	bool Intersect(const Ray &r, Hit *hit) const override;
	Material *GetMaterial() const override { return m_material; }

private:
	std::unique_ptr<Shape> m_shape;
	Material *m_material;
};

class LoosePrimitives: public Primitive
{
public:
	LoosePrimitives(std::vector<std::unique_ptr<Primitive>> &&prims);
	~LoosePrimitives() override;
	bool Intersect(const Ray &r, Hit *hit) const override;
	Material *GetMaterial() const override {return nullptr; };

private:
	std::vector<std::unique_ptr<Primitive>> m_primitives;
};
