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
#include <thread>
#include <cstdint>
#include <mutex>
#include <optional>

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

vector3 ImportanceSampleGGX(float e0, float e1, vector3 normal, vector3 wo, Material *m, vector3 &wi)
{
	float a = m->roughness * m->roughness;
	float a2 = a * a;

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

float radicalInverse_VdC(uint32_t bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

void hammersley(uint32_t i, uint32_t N, uint32_t offset, float *a, float *b)
{
	*a = static_cast<float>(i) / static_cast<float>(N);
	*b = radicalInverse_VdC(i + offset);
}

struct tileData
{
	int x1;
	int y1;
	int x2;
	int y2;
	float *tileOutput;
};

void renderWorker(std::vector<tileData> *tileInfos, std::mutex *tileLock, int tileW, int tileH, int imageW, int imageH, float filmW, float filmH, vector3 cameraOrigin, const Scene *scene)
{
	auto getTile = [tileInfos, tileLock]()->std::optional<tileData>
	{
		std::scoped_lock lock(*tileLock);
		if (tileInfos->size() > 0)
		{
			tileData data = tileInfos->back();
			tileInfos->pop_back();
			return data;
		}
		return std::nullopt;
	};

	std::random_device rd;
	std::minstd_rand gen(rd());
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);

	static const int sampleCount = 256; // TODO: move this someplace better
	static const int bounceCount = 10; // TODO: move this someplace better
	auto tile = getTile();
	while (tile.has_value())
	{
		tileData data = tile.value();
//		std::cout << "Rendering tile x: " << data.x1 << " y: " << data.y1 << std::endl;

		for (int y = 0; y < data.y2 - data.y1; ++y)
		{
			for (int x = 0; x < data.x2 - data.x1; ++x)
			{
				vector3 L;

				for (int sample = 0; sample < sampleCount; ++sample)
				{
	/*				if (x == 57 && y == 162)
					{
						std::cout << x << std::endl;
					}*/
					float subSampleX;
					float subSampleY;
					hammersley(sample, sampleCount, 0, &subSampleX, &subSampleY);

					float filmX = (2.0f * (static_cast<float>(data.x1 + x) + (subSampleX - 0.5f)) / imageW - 1.0f) * filmW;
					float filmY = (1.0f - 2.0f * (static_cast<float>(data.y1 + y) + (subSampleY - 0.5f)) / imageH) * filmH;
					float filmZ = -1.0f;
					vector3 filmDir(filmX, filmY, filmZ);
					Ray ray(cameraOrigin, filmDir.normalized());

					bool hit = true;
					int bounce = 0;
					vector3 throughput(1.0f, 1.0f, 1.0f);

					while (bounce < bounceCount && hit)
					{
						Hit hitData;

						hit = scene->Intersect(ray, &hitData);
						if (hit)
						{
							Material *m = hitData.primitive->GetMaterial();

							for (auto &light : scene->m_lights)
							{
								vector3 lightVec = light->pos - hitData.position;
								vector3 lightDir = lightVec.normalized();
								Ray lightRay(hitData.position + (hitData.normal * 1e-5), lightDir);
								if (!scene->Intersect(lightRay, nullptr))
								{
									float lightDistance = lightVec.length();
									float lightContrib = lightDir.dot(hitData.normal);
									float attenuation = (lightDistance * lightDistance);
									L += DisneyBRDF(hitData.normal, lightDir, -ray.direction, m->color, m->roughness, m->metalness) * light->color * light->strength * throughput / attenuation;
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
							float e0 = dis(gen);
							float e1 = dis(gen);
							//hammersley(sample, sampleCount, 0, &e0, &e1);
							throughput *= ImportanceSampleGGX(e0, e1, hitData.normal, wo, m, reflected);
//							throughput *= ImportanceSampleGGX([&gen, &dis]()->float { return dis(gen); }, hitData.normal, wo, m, reflected);
	//						reflected = ImportanceSample(hitData.normal, [&gen, &dis]()->float { return dis(gen); });
	//						throughput *= DisneyBRDF(hitData.normal, reflected, -ray.direction, m->color, m->roughness, m->metalness) / pdf(reflected, hitData.normal);

	//						reflected = hitData.normal * reflected.x + tangent * reflected.y + binormal * reflected.z;

							ray.direction = reflected;
							ray.tMax = std::numeric_limits<float>::infinity();
							ray.origin = hitData.position;
							ray.origin += ray.direction * 1e-6;

							bounce++;
						}
						else
						{
							if (scene->GetSkyMaterial())
							{
								L += scene->GetSkyMaterial()->color * throughput;
							}
						}
					}
				}

				L = Reinhard(L / static_cast<float>(sampleCount));

				data.tileOutput[(y * tileW + x) * 3 + 0] = toSRGB(L.x);
				data.tileOutput[(y * tileW + x) * 3 + 1] = toSRGB(L.y);
				data.tileOutput[(y * tileW + x) * 3 + 2] = toSRGB(L.z);
			}
		}
		tile = getTile();
	}
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

	std::unique_ptr<GeometricPrimitive> sphere1 = std::make_unique<GeometricPrimitive>(std::make_unique<Sphere>(vector3(0.0f, 0.0f, -3.0f), 0.5f), &red);
	std::unique_ptr<GeometricPrimitive> plane1 = std::make_unique<GeometricPrimitive>(std::make_unique<Plane>(vector3(0.0f, 1.0f, 0.0f), -0.5f), &brown);
	std::vector<std::unique_ptr<Primitive>> primitives;
	primitives.push_back(std::move(sphere1));
	primitives.push_back(std::move(plane1));
	LoosePrimitives prims(std::move(primitives));

	std::vector<std::unique_ptr<Light>> lights;
	lights.push_back(std::make_unique<Light>(vector3(-1.5f, 1.0f, 3.0f), vector3(5.0f, 5.0f, 5.0f), 25.0f));

	Scene scene(prims, lights);
	scene.m_skyMaterial = &sky;

	vector3 cameraOrigin(0.0f, 0.0f, 0.0f);
	float fov = 37.8f;
	float aspect = imageW / imageH;
	float filmW = tan(fov / 2.0f * pi / 180.0f) * aspect;
	float filmH = tan(fov / 2.0f * pi / 180.0f);

	static const int tileWidth = 32;
	static const int tileHeight = 32;
	static const int tileStride = tileWidth * tileHeight * 3;
	static const int tileCountX = divideRoundingUp(IMAGE_W, tileWidth);
	static const int tileCountY = divideRoundingUp(IMAGE_H, tileHeight);

	std::vector<float> image(tileCountX * tileCountY * tileStride);

	std::vector<tileData> tiles;

	for (int y = 0; y < tileCountY; ++y)
	{
		for (int x = 0; x < tileCountX; ++x)
		{
			tileData tile;
			tile.x1 = x * tileWidth;
			tile.y1 = y * tileHeight;
			tile.x2 = std::min((x + 1) * tileWidth, IMAGE_W);
			tile.y2 = std::min((y + 1) * tileHeight, IMAGE_H);
			tile.tileOutput = &image[(y * tileCountX + x) * tileStride];
			tiles.push_back(tile);
		}
	}

	std::vector<std::thread> workers;
	std::mutex tileLock;
	int maxThreads = std::thread::hardware_concurrency();
	for (int i = 0; i < maxThreads; ++i) {
		std::thread t(renderWorker, &tiles, &tileLock, tileWidth, tileHeight, IMAGE_W, IMAGE_H, filmW, filmH, cameraOrigin, &scene);
		workers.push_back(std::move(t));
	}

	for (int i = 0; i < maxThreads; ++i)
	{
		workers[i].join();
	}

	std::vector<uint8_t> data(IMAGE_W * IMAGE_H * 3);
	for (int y = 0; y < tileCountY; ++y)
	{
		for (int x = 0; x < tileCountX; ++x)
		{
			for (int tileY = 0; tileY < tileHeight; ++tileY)
			{
				for (int tileX = 0; tileX < tileWidth; ++tileX)
				{
					int targetX = x * tileWidth + tileX;
					int targetY = y * tileHeight + tileY;
					if (targetX < IMAGE_W && targetY < IMAGE_H)
					{
						data[(targetY * IMAGE_W + targetX) * 3 + 0] =
							static_cast<uint8_t>(image[(y * tileCountX + x) * tileStride + (tileY * tileWidth + tileX) * 3 + 0] * 255.0f);
						data[(targetY * IMAGE_W + targetX) * 3 + 1] =
							static_cast<uint8_t>(image[(y * tileCountX + x) * tileStride + (tileY * tileWidth + tileX) * 3 + 1] * 255.0f);
						data[(targetY * IMAGE_W + targetX) * 3 + 2] =
							static_cast<uint8_t>(image[(y * tileCountX + x) * tileStride + (tileY * tileWidth + tileX) * 3 + 2] * 255.0f);
					}
				}
			}
		}
	}

	std::fstream f("out.ppm", std::fstream::binary | std::fstream::out);

	f << "P6\n";
	f << IMAGE_W << " " << IMAGE_H << "\n";
	f << "255\n";
	f.write(reinterpret_cast<char*>(data.data()), data.size());

	return 0;
}
