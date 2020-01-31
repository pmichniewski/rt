#pragma once

#include <cmath>
#include <iostream>

class vector3
{
public:
	float x;
	float y;
	float z;

	vector3(): x(0.0f), y(0.0f), z(0.0f) {}
	vector3(float x_, float y_, float z_): x(x_), y(y_), z(z_) {}

	vector3 normalized();

	float length() const;
	float dot(const vector3 &o) const;
	vector3 &operator +=(const vector3 &o);
	vector3 &operator -=(const vector3 &o);
	vector3 &operator *=(float f);
	friend std::ostream& operator<<(std::ostream& os, const vector3& v);
};

vector3 operator +(const vector3 &a, const vector3 &b);
vector3 operator -(const vector3 &a, const vector3 &b);
vector3 operator *(const vector3 &a, float f);
