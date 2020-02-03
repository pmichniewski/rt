#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <limits>

#include "math.h"
#include "sphere.h"
#include "plane.h"
#include "ray.h"
#include "scene.h"
#include "primitive.h"
#include "light.h"
#include "material.h"

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

float disneySchlick(float u)
{
	return std::pow(1.0f - u, 5.0f);
}

float disneyGTR2(float NdotH, float alpha) {
	float alpha2 = alpha * alpha;
	float t = 1.0f + (alpha2 - 1.0f) * NdotH * NdotH;
	return alpha2 / (pi * t * t);
}

float disneySmithG_GGX(float NdotV, float alphaG) {
	float a = alphaG * alphaG;
	float b = NdotV * NdotV;
	return 1.0 / (NdotV + sqrt(a + b - a * b));
}

vector3 DisneyBRDF(vector3 N, vector3 L, vector3 V, vector3 baseColor, float roughness, float metalness)
{
	float NdotV = N.dot(V);
	float NdotL = N.dot(L);

	roughness = std::max(0.001f, roughness); // just in case

	vector3 H = (L + V).normalized();
	float LdotH = L.dot(H);
	float NdotH = N.dot(H);

	vector3 f0 = lerp(vector3(0.04f, 0.04f, 0.04f), baseColor, metalness); // 4% reflectivity for dielectrics

	//diffuse
	float FL = disneySchlick(NdotL);
	float FV = disneySchlick(NdotV);
	float Fd90 = 0.5f + 2.0f * NdotH * LdotH * roughness;
	float Fd = lerp(1.0f, Fd90, FL) * lerp(1.0f, Fd90, FV);

	//specular
	float alpha = roughness * roughness;
	float Ds = disneyGTR2(NdotH, alpha);
	float FH = disneySchlick(LdotH);
	vector3 Fs = lerp(f0, vector3(1.0f, 1.0f, 1.0f), FH);
	float roughg = (roughness * 0.5f + 0.5f) * (roughness * 0.5f + 0.5f);
	float Gs = disneySmithG_GGX(NdotL, roughg) * disneySmithG_GGX(NdotV, roughg);

	return (rcpPi * Fd * (1.0f - metalness) * baseColor + Gs * Fs * Ds) * clamp(NdotL, 0.0f, 1.0f);
}

vector3 Reinhard(vector3 color)
{
	vector3 mapped = color / (color + vector3(1.0f, 1.0f, 1.0f));

	return mapped;
}

int main() {
	const int IMAGE_W = 1920;
	const int IMAGE_H = 1080;
	const float imageW = static_cast<float>(IMAGE_W);
	const float imageH = static_cast<float>(IMAGE_H);

	Material red(vector3(1.0f, 0.0f, 0.0f), 0.2f, 0.0f);
	Material brown(vector3(0.5f, 0.3f, 0.0f), 0.8f, 0.0f);

	std::vector<float> image(IMAGE_W * IMAGE_H * 3);
	std::unique_ptr<GeometricPrimitive> sphere1 = std::make_unique<GeometricPrimitive>(std::make_unique<Sphere>(vector3(0.0f, 0.0f, -3.0f), 0.5f), &red);
	std::unique_ptr<GeometricPrimitive> plane1 = std::make_unique<GeometricPrimitive>(std::make_unique<Plane>(vector3(0.0f, 1.0f, 0.0f), -2.0f), &brown);
	std::vector<std::unique_ptr<Primitive>> primitives;
	primitives.push_back(std::move(sphere1));
	primitives.push_back(std::move(plane1));
	LoosePrimitives prims(std::move(primitives));

	std::vector<std::unique_ptr<Light>> lights;
	lights.push_back(std::make_unique<Light>(vector3(-1.5f, 0.0f, -4.0f), vector3(5.0f, 5.0f, 5.0f)));

	Scene scene(prims, lights);

	vector3 cameraOrigin(0.0f, 0.0f, 0.0f);
	float fov = 37.8f;
	float aspect = imageW / imageH;
	float filmW = tan(fov / 2.0f * pi / 180.0f) * aspect;
	float filmH = tan(fov / 2.0f * pi / 180.0f);

	for(int y = 0; y < IMAGE_H; ++y)
	{
		for (int x = 0; x < IMAGE_W; ++x)
		{
			float filmX = (2.0f * (static_cast<float>(x) + 0.5f) / imageW - 1.0f) * filmW;
			float filmY = (1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / imageH) * filmH;
			float filmZ = -1.0f;
			vector3 filmDir(filmX, filmY, filmZ);
			Ray camRay(vector3(0.0f, 0.0f, 0.0f), filmDir.normalized());

			float r = 0;
			float g = 0;
			float b = 0;

			bool hit = false;
			Hit hitData;

			vector3 L;

			hit = scene.Intersect(camRay, &hitData);
			if (hit)
			{
				for (auto &light: scene.m_lights)
				{
					vector3 lightVec = light->pos - hitData.position;
					vector3 lightDir = lightVec.normalized();
					float lightDistance = lightVec.length();
					float lightContrib = lightDir.dot(hitData.normal);
					float attenuation = (lightDistance * lightDistance);
					Material *m = hitData.primitive->GetMaterial();
					L += DisneyBRDF(hitData.normal, lightDir, -camRay.direction, m->color, m->roughness, m->metalness) * light->color; 
				}
			}
			else
			{
				L = vector3(0.0f, 0.2f, 0.5f);
//				L = vector3(0.0f, 0.0f, 0.0f);
			}

			L = Reinhard(L);

			image[(y * IMAGE_W + x) * 3 + 0] = toSRGB(L.x);
			image[(y * IMAGE_W + x) * 3 + 1] = toSRGB(L.y);
			image[(y * IMAGE_W + x) * 3 + 2] = toSRGB(L.z);
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
