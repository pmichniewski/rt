#include "shape.h"
#include "hit.h"

class Plane: public Shape
{
public:
	Plane(vector3 normal_, float d_): normal(normal_), d(d_) {}
	virtual ~Plane() {}
	virtual bool Intersect(const Ray &r, float *t, Hit *h) override;

	vector3 normal;
	float d;
};

