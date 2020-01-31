#include "math.h"

vector3 vector3::normalized()
{
	vector3 result = *this;
	float rcpLength = 1.0f / result.length();
	result.x *= rcpLength;
	result.y *= rcpLength;
	result.z *= rcpLength;
	return result;
}

vector3 operator +(const vector3 &a, const vector3 &b)
{
	vector3 result = a;
	result += b;
	return result;
}

vector3 operator -(const vector3 &a, const vector3 &b)
{
	vector3 result = a;
	result -= b;
	return result;
}

vector3 operator *(const vector3 &a, float f)
{
	vector3 result = a;
	result *= f;
	return result;
}

float vector3::length() const
{
	return sqrt(x*x + y*y + z*z);
}

float vector3::dot(const vector3 &o) const
{
	return x * o.x + y * o.y + z * o.z;
}

vector3 &vector3::operator +=(const vector3 &o)
{
	x += o.x;
	y += o.y;
	z += o.z;
	return *this;
}

vector3 &vector3::operator -=(const vector3 &o)
{
	x -= o.x;
	y -= o.y;
	z -= o.z;
	return *this;
}

vector3 &vector3::operator *=(float f)
{
	x *= f;
	y *= f;
	z *= f;
	return *this;
}

std::ostream &operator<<(std::ostream &os, const vector3 &v)
{
	os << "(" << v.x << ", " << v.y << ", " << v.z << ")";

	return os;
}
