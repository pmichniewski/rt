#pragma once

#include <cmath>

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
