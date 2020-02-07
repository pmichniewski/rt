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
	vector3(float f) : x(f), y(f), z(f) {}

	vector3 normalized();

	float length() const;
	float dot(const vector3 &o) const;
	vector3 cross(const vector3 &o) const;
	vector3 &operator+=(const vector3 &o);
	vector3 &operator-=(const vector3 &o);
	vector3 &operator*=(const vector3 &o);
	vector3 &operator/=(const vector3 &o);
	vector3 operator-() const;
	vector3 &operator*=(float f);
	vector3 &operator/=(float f);
	friend std::ostream& operator<<(std::ostream& os, const vector3& v);
};

float lerp(float a, float b, float t);
vector3 lerp(const vector3 &a, const vector3 &b, float t);
float clamp(float v, float a, float b);

vector3 operator+(const vector3 &a, const vector3 &b);
vector3 operator-(const vector3 &a, const vector3 &b);
vector3 operator*(const vector3 &a, const vector3 &b);
vector3 operator/(const vector3 &a, const vector3 &b);
vector3 operator*(const vector3 &a, float f);
vector3 operator/(const vector3 &a, float f);
vector3 operator*(float f, const vector3 &b);

inline uint32_t divideRoundingUp(uint32_t a, uint32_t b)
{
	return (a + b - 1) / b;
}

constexpr float pi = 3.1415927f;
constexpr float rcpPi = 1.0f / pi;
