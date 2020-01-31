#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <limits>

#include "math.h"
#include "sphere.h"
#include "shape.h"
#include "ray.h"
#include "scene.h"
#include "primitive.h"
#include "light.h"

int main() {
	const int IMAGE_W = 1920;
	const int IMAGE_H = 1080;

	std::vector<uint8_t> data(IMAGE_W * IMAGE_H * 3);
	std::unique_ptr<Sphere> s = std::make_unique<Sphere>(vector3(0.0f, 0.0f, -3.0f), 1.0f);
	std::unique_ptr<GeometricPrimitive> gp = std::make_unique<GeometricPrimitive>(std::move(s));
	std::vector<std::unique_ptr<Primitive>> primitives;
	primitives.push_back(std::move(gp));
	LoosePrimitives prims(std::move(primitives));

	std::vector<std::unique_ptr<Light>> lights;
	lights.push_back(std::make_unique<Light>(vector3(0.0f, 1.0f, 2.0f), vector3(1.0f, 1.0f, 1.0f)));

	Scene scene(prims, lights);

	vector3 cameraOrigin(0.0f, 0.0f, 0.0f);
	float near = 0.01f;
	float fov = 90.0f;
	float aspect = 16.0f / 9.0f;
	float filmW = aspect * tan(fov) * near;
	float filmH = tan(fov) * near;

	for(int y = 0; y < IMAGE_H; ++y)
	{
		for (int x = 0; x < IMAGE_W; ++x)
		{
			float filmX = ((float)x / static_cast<float>(IMAGE_W - 1) - 0.5f) * filmW;
			float filmY = ((float)y / static_cast<float>(IMAGE_H - 1) - 0.5f) * filmH;
			float filmZ = -near;
			vector3 filmDir(filmX, filmY, filmZ);
			Ray camRay(vector3(0.0f, 0.0f, 0.0f), filmDir.normalized());

			uint8_t r = 0;
			uint8_t g = 0;
			uint8_t b = 0;

			bool hit = false;
			Hit hitData;

			hit = scene.Intersect(camRay, &hitData);
			if (hit)
			{
/*				r = 255;
				g = 0;
				b = 0;*/
				float L = 0.0f;
				for (auto &light: scene.m_lights)
				{
					vector3 lightDir = (light->pos - hitData.position).normalized();
					L += lightDir.dot(hitData.normal);
				}
				r = (255.0f * L);
				g = 0;
				b = 0;
			}
			else
			{
				r = 0;
				g = 50;
				b = 127;
			}

			data[(y * IMAGE_W + x) * 3 + 0] = r;
			data[(y * IMAGE_W + x) * 3 + 1] = g;
			data[(y * IMAGE_W + x) * 3 + 2] = b;
		}
	}

	std::fstream f("out.ppm", std::fstream::binary | std::fstream::out);

	f << "P6\n";
	f << IMAGE_W << " " << IMAGE_H << "\n";
	f << "255\n";
	f.write(reinterpret_cast<char*>(data.data()), data.size());

	return 0;
}
