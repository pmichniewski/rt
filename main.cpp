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

float toSRGB(float in)
{
	if (in <= 0.0031308f)
	{
		return in * 12.92f;
	}
	else
	{
		return pow(in * 1.055f, 0.416f) - 0.055f;
	}
}

int main() {
	const int IMAGE_W = 1920;
	const int IMAGE_H = 1080;

	std::vector<float> image(IMAGE_W * IMAGE_H * 3);
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

			float r = 0;
			float g = 0;
			float b = 0;

			bool hit = false;
			Hit hitData;

			hit = scene.Intersect(camRay, &hitData);
			if (hit)
			{
				float L = 0.0f;
				for (auto &light: scene.m_lights)
				{
					vector3 lightDir = (light->pos - hitData.position).normalized();
					float lightContrib = lightDir.dot(hitData.normal);
					if (lightContrib > 0.0f)
					{
						L += lightContrib;
					}
				}
				r = L;
				g = 0.0f;
				b = 0.0f;
			}
			else
			{
				r = 0.0f;
				g = 0.2f;
				b = 0.5f;
			}

			image[(y * IMAGE_W + x) * 3 + 0] = toSRGB(r);
			image[(y * IMAGE_W + x) * 3 + 1] = toSRGB(g);
			image[(y * IMAGE_W + x) * 3 + 2] = toSRGB(b);
		}
	}

	std::vector<uint8_t> data(IMAGE_W * IMAGE_H * 3);
	for (int y = 0; y < IMAGE_H; ++y)
	{
		for (int x = 0; x < IMAGE_W; ++x)
		{
			data[(y * IMAGE_W + x) * 3 + 0] = static_cast<uint8_t>(image[(y * IMAGE_W + x) * 3 + 0] * 255.0f);
			data[(y * IMAGE_W + x) * 3 + 1] = static_cast<uint8_t>(image[(y * IMAGE_W + x) * 3 + 1] * 255.0f);
			data[(y * IMAGE_W + x) * 3 + 2] = static_cast<uint8_t>(image[(y * IMAGE_W + x) * 3 + 2] * 255.0f);
		}
	}

	std::fstream f("out.ppm", std::fstream::binary | std::fstream::out);

	f << "P6\n";
	f << IMAGE_W << " " << IMAGE_H << "\n";
	f << "255\n";
	f.write(reinterpret_cast<char*>(data.data()), data.size());

	return 0;
}
