#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>

class vector3
{
public:
	float x;
	float y;
	float z;

	vector3(float x_, float y_, float z_): x(x_), y(y_), z(z_) {}

	void normalize();

	float length() const;
	float dot(vector3 &o) const;
	vector3 operator +=(vector3 &o);
	vector3 operator -=(vector3 &o);
};

void vector3::normalize()
{
	float l = length();
	x /= l;
	y /= l;
	z /= l;
}

vector3 operator +(vector3 a, vector3 b)
{
	vector3 result = a;
	result += b;
	return result;
}

vector3 operator -(vector3 a, vector3 b)
{
	vector3 result = a;
	result -= b;
	return result;
}

float vector3::length() const
{
	return sqrt(x*x + y*y + z*z);
}

float vector3::dot(vector3 &o) const
{
	return x * o.x + y * o.y + z * o.z;
}

vector3 vector3::operator +=(vector3 &o)
{
	x += o.x;
	y += o.y;
	z += o.z;
	return *this;
}

vector3 vector3::operator -=(vector3 &o)
{
	x -= o.x;
	y -= o.y;
	z -= o.z;
	return *this;
}

class Ray
{
public:
	Ray(vector3 o, vector3 d): origin(o), direction(d) {}

	vector3 origin;
	vector3 direction;
};

class Shape
{
public:
	virtual bool intersect(Ray r, float &t) = 0;
};

class Sphere: public Shape
{
public:
	Sphere(vector3 center, float radius): m_center(center), m_radius(radius) {}

	vector3 m_center;
	float m_radius;
	bool intersect(Ray ray, float &t) override;
};

bool Sphere::intersect(Ray ray, float &t)
{
	vector3 L = ray.origin - m_center;
	float a = ray.direction.dot(ray.direction);
	float b = 2 * ray.direction.dot(L);
	float c = L.dot(L) - m_radius * m_radius;

	float delta = b * b - 4 * a * c;
	if (delta < 0.0f) return false;
	if (delta == 0.0f)
	{
		t = -0.5f * b / a;
		return true;
	}
	if (delta > 0.0f)
	{
		float q = (b > 0.0f) ?
			-0.5f * (b + sqrt(delta)) :
			-0.5f * (b - sqrt(delta));

		float t0 = q / a;
		float t1 = c / q;

		if (t0 > t1) std::swap(t0, t1);
		if (t0 < 0.0f)
		{
			t0 = t1;
			if (t0 < 0.0f) return false;
		}

		t = t0;
		return true;
	}
}

int main() {
	std::vector<uint8_t> data(192 * 108 * 3);
	std::vector<Shape*> shapes;

	Sphere s(vector3(0.0f, 0.0f, -3.0f), 1.0f);
	shapes.push_back(&s);

	vector3 cameraOrigin(0.0f, 0.0f, 0.0f);
	float near = 0.01f;
	float fov = 90.0f;
	float aspect = 16.0f / 9.0f;
	float filmW = aspect * tan(fov) * near;
	float filmH = tan(fov) * near;

	for(int y = 0; y < 108; ++y)
	{
		for (int x = 0; x < 192; ++x)
		{
			float filmX = ((float)x / 191.0f - 0.5f) * filmW;
			float filmY = ((float)y / 107.0f - 0.5f) * filmH;
			float filmZ = -near;
			vector3 filmDir(filmX, filmY, filmZ);
			filmDir.normalize();
			Ray camRay(vector3(0.0f, 0.0f, 0.0f), filmDir);

			uint8_t r = 0;
			uint8_t g = 0;
			uint8_t b = 0;

			bool hit = false;
			float nearestHit = std::numeric_limits<float>::infinity();

			for (Shape *s : shapes)
			{
				float t;
				if (s->intersect(camRay, t) && t < nearestHit)
				{
					hit = true;
					r = 255;
					g = 0;
					b = 0;
				}
			}

			if (!hit)
			{
				r = 0;
				g = 50;
				b = 127;
			}

			data[(y * 192 + x) * 3 + 0] = r;
			data[(y * 192 + x) * 3 + 1] = g;
			data[(y * 192 + x) * 3 + 2] = b;
		}
	}

	std::fstream f("out.ppm", std::fstream::binary | std::fstream::out);

	f << "P6\n";
	f << "192 108\n";
	f << "255\n";
	f.write(reinterpret_cast<char*>(data.data()), data.size());

	return 0;
}
