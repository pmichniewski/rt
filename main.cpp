#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <limits>

#include "math.h"
#include "sphere.h"
#include "shape.h"
#include "ray.h"

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
