#include "math.h"
#include "sphere.h"
#include "plane.h"
#include "ray.h"
#include "scene.h"
#include "primitive.h"
#include "light.h"
#include "material.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <random>
#include <functional>

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

void coordinateSystem(const vector3 &v1, vector3 *v2, vector3 *v3)
{
	if (std::abs(v1.x) > std::abs(v1.y))
	{
		*v2 = vector3(-v1.z, 0.0f, v1.x) / std::sqrt(v1.x * v1.x + v1.z * v1.z);
	}
	else
	{
		*v2 = vector3(0.0f, v1.z, -v1.y) / std::sqrt(v1.y * v1.y + v1.z * v1.z);
	}
	*v3 = v1.cross(*v2);
}

#if 1
vector3 TransformToWorld(float x, float y, float z, vector3 &normal) {
	// Find an axis that is not parallel to normal
	vector3 majorAxis;
	if (std::abs(normal.x) < 0.57735026919f /* 1 / sqrt(3) */) {
		majorAxis = vector3(1, 0, 0);
	}
	else if (std::abs(normal.y) < 0.57735026919f /* 1 / sqrt(3) */) {
		majorAxis = vector3(0, 1, 0);
	}
	else {
		majorAxis = vector3(0, 0, 1);
	}

	// Use majorAxis to create a coordinate system relative to world space
	vector3 u = normal.cross(majorAxis);
	vector3 v = normal.cross(u);
	vector3 w = normal;


	// Transform from local coordinates to world coordinates
	return u * x +
		v * y +
		w * z;
}
#endif

static float SmithGGXMaskingShadowing(vector3 wi, vector3 wo, vector3 normal, float a2)
{
	float dotNL = normal.dot(wi);
	float dotNV = normal.dot(wo);

	float denomA = dotNV * std::sqrt(a2 + (1.0f - a2) * dotNL * dotNL);
	float denomB = dotNL * std::sqrt(a2 + (1.0f - a2) * dotNV * dotNV);

	return 2.0f * dotNL * dotNV / (denomA + denomB);
}

vector3 ImportanceSampleGGX(std::function<float(void)> randomGen, vector3 normal, vector3 wo, Material *m, vector3 &wi)
{
	float a = m->roughness * m->roughness;
	float a2 = a * a;

	float e0 = randomGen();
	float e1 = randomGen();

	float theta = std::acos(std::sqrt((1.0f - e0) / ((a2 - 1.0f) * e0 + 1.0f)));
	float phi = 2.0f * pi * e1;

	vector3 wm = vector3(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)).normalized();

	vector3 tangent;
	vector3 binormal;
	coordinateSystem(normal, &tangent, &binormal);
	wm = tangent * wm.x + normal * wm.y + binormal * wm.z;
	//wm = TransformToWorld(wm.x, wm.y, wm.z, normal);

	wi = 2.0f * wo.dot(wm) * wm - wo;

	float dotWiWm = wi.dot(wm);
	if (normal.dot(wi) > 0.0f && dotWiWm > 0.0f)
	{
		float dotWoWm = wo.dot(wm);
		float dotNWo = normal.dot(wo);
		float dotNWi = normal.dot(wi);
		vector3 f0 = lerp(0.04f, m->color, m->metalness); // 4% reflectivity for dielectrics
		vector3 F = lerp(f0, 1.0f, disneySchlick(dotWiWm));
		//float G = disneySmithG_GGX(dotNWo, a2) * disneySmithG_GGX(dotNWi, a2);
		float G = SmithGGXMaskingShadowing(wi, wo, normal, a2);
		float weight = std::abs(dotWoWm) / (dotNWo * normal.dot(wm));

		return F * G * weight;
	}
	else
	{
		return vector3(0.0f, 0.0f, 0.0f);
	}
}

vector3 ImportanceSample(vector3 normal, std::function<float(void)> randomGen)
{
	float rand = randomGen();
	float r = std::sqrt(rand);
	float theta = randomGen() * 2.0f * pi;

	float x = r * std::cos(theta);
	float y = r * std::sin(theta);

	float z = std::sqrt(1.0f - x * x - y * y);

	return TransformToWorld(x, y, z, normal).normalized();
}

float pdf(vector3 inputDirection, vector3 normal)
{
	return inputDirection.dot(normal) * rcpPi;
}

int main() {
	const int IMAGE_W = 1920;
	const int IMAGE_H = 1080;
	const float imageW = static_cast<float>(IMAGE_W);
	const float imageH = static_cast<float>(IMAGE_H);

	//Material red(vector3(1.0f, 1.0f, 1.0f), 0.1f, 1.0f);
	Material red(vector3(1.0f, 0.0f, 0.0f), 0.1f, 0.0f);
	Material brown(vector3(0.0f, 0.43f, 0.0f), 1.0f, 0.0f);
	//Material brown(vector3(1.0f, 1.0f, 1.0f), 0.01f, 1.0f);
	Material sky(vector3(0.0f, 0.2f, 0.5f), 1.0f, 0.0f);

	std::random_device rd;
	std::minstd_rand gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	std::vector<float> image(IMAGE_W * IMAGE_H * 3);
	std::unique_ptr<GeometricPrimitive> sphere1 = std::make_unique<GeometricPrimitive>(std::make_unique<Sphere>(vector3(0.0f, 0.0f, -3.0f), 0.5f), &red);
	std::unique_ptr<GeometricPrimitive> plane1 = std::make_unique<GeometricPrimitive>(std::make_unique<Plane>(vector3(0.0f, 1.0f, 0.0f), -0.5f), &brown);
	std::vector<std::unique_ptr<Primitive>> primitives;
	primitives.push_back(std::move(sphere1));
	primitives.push_back(std::move(plane1));
	LoosePrimitives prims(std::move(primitives));

	std::vector<std::unique_ptr<Light>> lights;
	lights.push_back(std::make_unique<Light>(vector3(-1.5f, 1.0f, 3.0f), vector3(5.0f, 5.0f, 5.0f)));

	Scene scene(prims, lights);

	vector3 cameraOrigin(0.0f, 0.0f, 0.0f);
	float fov = 37.8f;
	float aspect = imageW / imageH;
	float filmW = tan(fov / 2.0f * pi / 180.0f) * aspect;
	float filmH = tan(fov / 2.0f * pi / 180.0f);

	static const int sampleCount = 64;

	for(int y = 0; y < IMAGE_H; ++y)
	{
		if (y % 10 == 0) {
			std::cout << y * 100 / IMAGE_H << std::endl;
		}
		for (int x = 0; x < IMAGE_W; ++x)
		{
			vector3 L;

			for (int sample = 0; sample < sampleCount; ++sample)
			{
/*				if (x == 57 && y == 162)
				{
					std::cout << x << std::endl;
				}*/

				float filmX = (2.0f * (static_cast<float>(x) + 0.5f) / imageW - 1.0f) * filmW;
				float filmY = (1.0f - 2.0f * (static_cast<float>(y) + 0.5f) / imageH) * filmH;
				float filmZ = -1.0f;
				vector3 filmDir(filmX, filmY, filmZ);
				Ray ray(cameraOrigin, filmDir.normalized());

				bool hit = true;
				int bounces = 10;
				vector3 throughput(1.0f, 1.0f, 1.0f);

				while (bounces > 0 && hit)
				{
					Hit hitData;

					hit = scene.Intersect(ray, &hitData);
					if (hit)
					{
						Material *m = hitData.primitive->GetMaterial();

						for (auto &light : scene.m_lights)
						{
							vector3 lightVec = light->pos - hitData.position;
							vector3 lightDir = lightVec.normalized();
							Ray lightRay(hitData.position + (hitData.normal * 1e-5), lightDir);
							if (!scene.Intersect(lightRay, nullptr))
							{
								float lightDistance = lightVec.length();
								float lightContrib = lightDir.dot(hitData.normal);
								float attenuation = (lightDistance * lightDistance);
								L += DisneyBRDF(hitData.normal, lightDir, -ray.direction, m->color, m->roughness, m->metalness) * light->color * throughput;
							}
						}

						//vector3 reflected = ray.direction - (2.0f * (hitData.normal.dot(ray.direction)) * hitData.normal);
						//auto rnd = [&gen, &dis]() {
						//	return dis(gen());
						//}

/*						vector3 tangent;
						vector3 binormal;
						coordinateSystem(hitData.normal, &tangent, &binormal);*/

						vector3 wo = -ray.direction;
//						wo = vector3(hitData.normal.dot(wo), tangent.dot(wo), binormal.dot(wo));

						vector3 reflected;
						throughput *= ImportanceSampleGGX([&gen, &dis]()->float { return dis(gen); }, hitData.normal, wo, m, reflected);
//						reflected = ImportanceSample(hitData.normal, [&gen, &dis]()->float { return dis(gen); });
//						throughput *= DisneyBRDF(hitData.normal, reflected, -ray.direction, m->color, m->roughness, m->metalness) / pdf(reflected, hitData.normal);

//						reflected = hitData.normal * reflected.x + tangent * reflected.y + binormal * reflected.z;

						ray.direction = reflected;
						ray.tMax = std::numeric_limits<float>::infinity();
						ray.origin = hitData.position;
						ray.origin += ray.direction * 1e-6;

						bounces--;
					}
					else
					{
						L += sky.color * throughput;
					}
				}
			}

			L = Reinhard(L / static_cast<float>(sampleCount));

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
