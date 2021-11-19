#define NOMINMAX // prevents overlap of Windows.h with the std

#include <Windows.h>
#include <tchar.h>
#include <iostream>
#include <memory>
#include <random>
#include <time.h>
#include <chrono>
#include <SimpleImage.h>
#include <TextureGenerator.h>

// Main Functions
bool interpret_arguments(int argc, _TCHAR* argv[], int64_t& resolution, _TCHAR*& heightmap_path, _TCHAR*& color_path, _TCHAR*& normalmap_path);
// Generators
std::vector<float> generate_heightfield(int64_t resolution);
std::vector<GEDUtils::Vec3f> generate_normals(std::vector<float>& height, int64_t resolution);
std::vector<GEDUtils::Vec3f> generate_colors(std::vector<float>& height, std::vector<GEDUtils::Vec3f>& normal, int64_t resolution);
std::vector<float> resize_heightfield(std::vector<float>& height, int64_t resolution);
// Other
void smooth_heightfield(std::vector<float>& height, int64_t resolution, int64_t iterations, int64_t kernel_size);
void make_pretty(std::vector<float>& height, int64_t resolution);
bool save_image(std::vector<float>& data, int64_t resolution, _TCHAR* path);
bool save_image(std::vector<GEDUtils::Vec3f>& data, int64_t resolution, _TCHAR* path);
// Helpers
int64_t idx(int64_t x, int64_t y, int64_t size);
float smoothstep(float x);
float clamp(float x, float min = 0.0f, float max = 1.0f);
float map_range(float x, float from_low, float from_high, float to_low = 0.0f, float to_high = 1.0f);
GEDUtils::Vec3f blend(GEDUtils::Vec3f& a, GEDUtils::Vec3f& b, float alpha);
GEDUtils::Vec3f texture(GEDUtils::SimpleImage& tex, UINT u, UINT v);

int _tmain(int argc, _TCHAR* argv[])
{
	// Command line parameters
	int64_t resolution = 0;
	_TCHAR* heightmap_path = nullptr;
	_TCHAR* color_path = nullptr;
	_TCHAR* normalmap_path = nullptr;

	if (!interpret_arguments(argc, argv, resolution, heightmap_path, color_path, normalmap_path))
		return EXIT_FAILURE;

	// auto lets the compiler determine the type from context
	auto start_time = std::chrono::high_resolution_clock::now();

	std::cout << "Generating heightfield" << std::endl;
	auto height = generate_heightfield(resolution);
	make_pretty(height, resolution);
	std::cout << "Generating normalmap" << std::endl;
	auto normal = generate_normals(height, resolution);
	std::cout << "Generating colormap" << std::endl;
	auto color = generate_colors(height, normal, resolution);

	auto mid_time = std::chrono::high_resolution_clock::now();

	std::cout << "Saving Images" << std::endl;
	auto height_small = resize_heightfield(height, resolution);
	if (!save_image(height_small, resolution / 4, heightmap_path))
		std::wcout << "ERROR: Heightmap could not be saved to: " << heightmap_path << std::endl;
	if (!save_image(color, resolution, color_path))
		std::wcout << "ERROR: Colormap could not be saved to: " << color_path << std::endl;
	if (!save_image(normal, resolution, normalmap_path))
		std::wcout << "ERROR: Normalmap could not be saved to: " << normalmap_path << std::endl;

	auto end_time = std::chrono::high_resolution_clock::now();

	std::cout << "Generated in " << std::chrono::duration_cast<std::chrono::milliseconds>(mid_time - start_time).count() << " milliseconds." << std::endl;
	std::cout << "Saved in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - mid_time).count() << " milliseconds." << std::endl;

	return EXIT_SUCCESS;
}

bool interpret_arguments(int argc, _TCHAR* argv[], int64_t& resolution, _TCHAR*& heightmap_path, _TCHAR*& color_path, _TCHAR*& normalmap_path)
{
	// Interpret the command line arguments, similiar to the config parser
	// Start with 1 since the first argument is the current path
	for (int i = 1; i < argc; i++)
	{
		if (_tcscmp(TEXT("-r"), argv[i]) == 0)
		{
			i++;
			if (i < argc)
				resolution = _tstoi64(argv[i]);
			else
				std::cout << "ERROR: Terrain resolution parameter missing." << std::endl;
		}
		else if (_tcscmp(TEXT("-o_height"), argv[i]) == 0)
		{
			i++;
			if (i < argc)
				heightmap_path = argv[i];
			else
				std::cout << "ERROR: Terrain heightmap path missing." << std::endl;
		}
		else if (_tcscmp(TEXT("-o_color"), argv[i]) == 0)
		{
			i++;
			if (i < argc)
				color_path = argv[i];
			else
				std::cout << "ERROR: Terrain colormap path parameter missing." << std::endl;
		}
		else if (_tcscmp(TEXT("-o_normal"), argv[i]) == 0)
		{
			i++;
			if (i < argc)
				normalmap_path = argv[i];
			else
				std::cout << "ERROR: Terrain normalmap path missing." << std::endl;
		}
		else
		{
			std::cout << "WARNING: Unknown parameter (will be ignored): " << argv[i] << std::endl;
		}
	}
	// Check if all necessary parameters are set
	// We cannot check here if the paths are valid
	if (resolution <= 0)
	{
		std::cout << "ERROR: Resolution must be greater than 0" << std::endl;
		return false;
	}
	else if ((resolution & (resolution - 1)) != 0) // using https://iq.opengenus.org/detect-if-a-number-is-power-of-2-using-bitwise-operators/
	{
		std::cout << "ERROR: Resolution must be a power of two" << std::endl;
		return false;
	}
	if (heightmap_path == nullptr)
	{
		std::cout << "ERROR: Please provide a path for the heightmap using -o_height" << std::endl;
		return false;
	}
	if (color_path == nullptr)
	{
		std::cout << "ERROR: Please provide a path for the colormap using -o_color" << std::endl;
		return false;
	}
	if (normalmap_path == nullptr)
	{
		std::cout << "ERROR: Please provide a path for the normalmap using -o_normal" << std::endl;
		return false;
	}

	return true;
}

std::vector<float> generate_heightfield(int64_t resolution)
{
	std::default_random_engine rand(4u);
	std::normal_distribution<float> dist(0.0f, 1.f);

	int64_t ds_res = resolution + 1;
	std::vector<float> ds_field(ds_res * ds_res);

	// Initialize corners
	ds_field[0] = dist(rand);
	ds_field[ds_res - 1] = dist(rand);
	ds_field[(ds_res - 1) * ds_res] = dist(rand);
	ds_field[ds_res - 1 + (ds_res - 1) * ds_res] = dist(rand);

	float step = 1;
	for (int64_t distance = ds_res - 1; distance >= 1; distance = distance / 2)
	{
		dist = std::normal_distribution<float>(0, pow(0.56f, step));

		// Diamond
		for (int64_t y = 0; y < ds_res - 1; y += distance)
			for (int64_t x = 0; x < ds_res - 1; x += distance)
			{
				//  1   2
				//    # 
				//  3   4
				float sum = 0;
				sum += ds_field[idx(x, y, ds_res)]; // 1
				sum += ds_field[idx(x + distance, y, ds_res)]; // 2
				sum += ds_field[idx(x, y + distance, ds_res)]; // 3
				sum += ds_field[idx(x + distance, y + distance, ds_res)]; // 4
				ds_field[idx(x + distance / 2, y + distance / 2, ds_res)] = sum / 4.0f + dist(rand);
			}

		step++;

		// Square
		for (int64_t y = 0; y < ds_res - 1; y += distance)
			for (int64_t x = 0; x < ds_res - 1; x += distance)
			{
				float sum;
				float count;

				//    4
				//  1 # 2
				//    3
				//  X   X
				//  
				sum = 0.0f;
				sum += ds_field[idx(x, y, ds_res)]; // 1
				sum += ds_field[idx(x + distance, y, ds_res)]; // 2
				sum += ds_field[idx(x + distance / 2, y + distance / 2, ds_res)]; // 3
				if (y >= distance)
				{
					sum += ds_field[idx(x + distance / 2, y - distance / 2, ds_res)]; // 4
					count = 4.0f;
				}
				else
					count = 3.0f;
				ds_field[idx(x + distance / 2, y, ds_res)] = sum / count + dist(rand);

				//     
				//  1 X X
				//4 # 3
				//  2   X
				// 
				sum = 0.0f;
				sum += ds_field[idx(x, y, ds_res)]; // 1
				sum += ds_field[idx(x, y + distance, ds_res)]; // 2
				sum += ds_field[idx(x + distance / 2, y + distance / 2, ds_res)]; // 3
				if (x >= distance)
				{
					sum += ds_field[idx(x - distance / 2, y + distance / 2, ds_res)]; // 4
					count = 4.0f;
				}
				else
					count = 3.0f;
				ds_field[idx(x, y + distance / 2, ds_res)] = sum / count + dist(rand);

				//     
				//  X X 1
				//  X 3 # 4
				//  X   2
				// 
				sum = 0.0f;
				sum += ds_field[idx(x + distance, y, ds_res)]; // 1
				sum += ds_field[idx(x + distance, y + distance, ds_res)]; // 2
				sum += ds_field[idx(x + distance / 2, y + distance / 2, ds_res)]; // 3
				if (x + distance > ds_res)
				{
					sum += ds_field[idx(x + distance + distance / 2, y + distance / 2, ds_res)]; // 4
					count = 4.0f;
				}
				else
					count = 3.0f;
				ds_field[idx(x + distance, y + distance / 2, ds_res)] = sum / count + dist(rand);

				//    
				//  X X X
				//  X 3 X
				//  1 # 2
				//    4
				sum = 0.0f;
				sum += ds_field[idx(x, y + distance, ds_res)]; // 1
				sum += ds_field[idx(x + distance, y + distance, ds_res)]; // 2
				sum += ds_field[idx(x + distance / 2, y + distance / 2, ds_res)]; // 3
				if (y + distance > ds_res)
				{
					sum += ds_field[idx(x + distance / 2, y + distance + distance / 2, ds_res)]; // 4
					count = 4.0f;
				}
				else
					count = 3.0f;
				ds_field[idx(x + distance / 2, y + distance, ds_res)] = sum / count + dist(rand);
			}
	}

	// Copy the temporary array to the output row by row due to the different row lengths
	std::vector<float> heightfield(resolution * resolution);
	for (int64_t y = 0; y < resolution; y++)
		std::copy(&ds_field[idx(0, y, ds_res)], &ds_field[idx(resolution, y, ds_res)], &heightfield[idx(0, y, resolution)]);

	// Compress heights to [0;1]
	float max = *std::max_element(heightfield.begin(), heightfield.end());
	float min = *std::min_element(heightfield.begin(), heightfield.end());
	for (auto& value : heightfield)
		value = clamp(map_range(value, min, max));

	return heightfield;
}

std::vector<GEDUtils::Vec3f> generate_normals(std::vector<float>& height, int64_t resolution)
{
	std::vector<GEDUtils::Vec3f> normal(resolution * resolution);

	for (int64_t y = 0; y < resolution; y++)
		for (int64_t x = 0; x < resolution; x++)
		{
			float n_x, n_y, n_z;

			// Compute X
			if (x == 0)
				n_x = height[idx(x + 1, y, resolution)] - height[idx(x, y, resolution)];
			else if (x == resolution - 1)
				n_x = height[idx(x, y, resolution)] - height[idx(x - 1, y, resolution)];
			else
				n_x = (height[idx(x + 1, y, resolution)] - height[idx(x - 1, y, resolution)]) / 2;

			// Compute Y
			if (y == 0)
				n_y = height[idx(x, y + 1, resolution)] - height[idx(x, y, resolution)];
			else if (y == resolution - 1)
				n_y = height[idx(x, y, resolution)] - height[idx(x, y - 1, resolution)];
			else
				n_y = (height[idx(x, y + 1, resolution)] - height[idx(x, y - 1, resolution)]) / 2;

			// Set Z
			n_z = 1.0f / resolution;

			// Normalize
			float length = sqrt(n_x * n_x + n_y * n_y + n_z * n_z);
			n_x /= -length;
			n_y /= -length;
			n_z /= length;

			normal[idx(x, y, resolution)] = GEDUtils::Vec3f(n_x * 0.5f + 0.5f, n_y * 0.5f + 0.5f, n_z * 0.5f + 0.5f);
		}

	return normal;
}

std::vector<GEDUtils::Vec3f> generate_colors(std::vector<float>& height, std::vector<GEDUtils::Vec3f>& normal, int64_t resolution)
{
	GEDUtils::SimpleImage tex_low_flat(L"../../../../external/textures/mud02.jpg");
	GEDUtils::SimpleImage tex_low_steep(L"../../../../external/textures/rock3.jpg");
	GEDUtils::SimpleImage tex_high_flat(L"../../../../external/textures/gras15.jpg");
	GEDUtils::SimpleImage tex_high_steep(L"../../../../external/textures/rock3.jpg");

	std::vector<GEDUtils::Vec3f> color(resolution * resolution);

	for (int64_t y = 0; y < resolution; y++)
		for (int64_t x = 0; x < resolution; x++)
		{
			// Compute texture coordinates
			UINT u = static_cast<UINT>(x);
			UINT v = static_cast<UINT>(y);

			// Compute alpha
			float alpha_slope = smoothstep(clamp(map_range(1.0f - normal[idx(x, y, resolution)].z, 0.1f, 0.2f)));
			float alpha_height = smoothstep(clamp(map_range(height[idx(x, y, resolution)], 0.3f, 0.32f)));

			// Sample textures
			GEDUtils::Vec3f low_flat = texture(tex_low_flat, u, v);
			GEDUtils::Vec3f low_steep = texture(tex_low_steep, u, v);
			GEDUtils::Vec3f high_flat = texture(tex_high_flat, u, v);
			GEDUtils::Vec3f high_steep = texture(tex_high_steep, u, v);

			// Blend
			GEDUtils::Vec3f low = blend(low_flat, low_steep, alpha_slope);
			GEDUtils::Vec3f high = blend(high_flat, high_steep, alpha_slope);

			color[idx(x, y, resolution)] = blend(low, high, alpha_height);
		}

	return color;
}

void smooth_heightfield(std::vector<float>& height, int64_t resolution, int64_t iterations, int64_t kernel_size)
{
	// Nothing to do
	if (iterations <= 0)
		return;

	std::vector<float> tmp_field(resolution * resolution);

	// Two pass smoothing
	for (int64_t i = 0; i < iterations; i++)
	{
		// Horizontal
		for (int64_t y = 0; y < resolution; y++)
			for (int64_t x = 0; x < resolution; x++)
			{
				float smoothed = 0.0f;
				for (int64_t k = -kernel_size; k <= kernel_size; k++)
					smoothed += height[idx(std::min(std::max(x + k, 0ll), resolution - 1), y, resolution)];
				tmp_field[idx(x, y, resolution)] = smoothed / (kernel_size * 2 + 1);
			}
		// Vertical
		for (int64_t y = 0; y < resolution; y++)
			for (int64_t x = 0; x < resolution; x++)
			{
				float smoothed = 0.0f;
				for (int64_t k = -kernel_size; k <= kernel_size; k++)
					smoothed += tmp_field[idx( x, std::min(std::max(y + k, 0ll), resolution - 1), resolution)];
				height[idx(x, y, resolution)] = smoothed / (kernel_size * 2 + 1);
			}
	}
}

void make_pretty(std::vector<float>& height, int64_t resolution)
{
	// Makes a deep copy of the heightfield
	auto smoothed = height;

	smooth_heightfield(smoothed, resolution, 1, std::max(resolution / 40ll, 1ll));

	for (int64_t y = 0; y < resolution; y++)
		for (int64_t x = 0; x < resolution; x++)
		{
			// Compute mix factor from terrain slope
			float mix = 0;
			if (x == 0)
				mix += abs(smoothed[idx(x + 2, y, resolution)] - smoothed[idx(x, y, resolution)]);
			else if (x == resolution - 1)
				mix += abs(smoothed[idx(x, y, resolution)] - smoothed[idx(x - 2, y, resolution)]);
			else
				mix += abs(smoothed[idx(x + 1, y, resolution)] - smoothed[idx(x - 1, y, resolution)]);
			if (y == 0)
				mix += abs(smoothed[idx(x, y + 2, resolution)] - smoothed[idx(x, y, resolution)]);
			else if (y == resolution - 1)
				mix += abs(smoothed[idx(x, y, resolution)] - smoothed[idx(x, y - 2, resolution)]);
			else
				mix += abs(smoothed[idx(x, y + 1, resolution)] - smoothed[idx(x, y - 1, resolution)]);

			mix = smoothstep(smoothstep(clamp(mix * (resolution / 8))));

			// Mix original and smoothed heightfield
			float value = height[idx(x, y, resolution)] * mix + smoothed[idx(x, y, resolution)] * (1.0f - mix);
			height[idx(x, y, resolution)] = value * value;
		}
}

bool save_image(std::vector<float>& data, int64_t resolution, _TCHAR* path)
{
	GEDUtils::SimpleImage image(static_cast<UINT>(resolution), static_cast<UINT>(resolution));

	for (int64_t y = 0; y < resolution; y++)
		for (int64_t x = 0; x < resolution; x++)
			image.setPixel(static_cast<UINT>(x), static_cast<UINT>(y), data[idx(x, y, resolution)]);

	return image.save(path);
}

bool save_image(std::vector<GEDUtils::Vec3f>& data, int64_t resolution, _TCHAR* path)
{
	GEDUtils::SimpleImage image(static_cast<UINT>(resolution), static_cast<UINT>(resolution));

	for (int64_t y = 0; y < resolution; y++)
		for (int64_t x = 0; x < resolution; x++)
			image.setPixel(static_cast<UINT>(x), static_cast<UINT>(y),
				data[idx(x, y, resolution)].x,
				data[idx(x, y, resolution)].y,
				data[idx(x, y, resolution)].z);

	return image.save(path);
}

std::vector<float> resize_heightfield(std::vector<float>& height, int64_t resolution)
{
	std::vector<float> output(resolution * resolution / 16);

	for (int64_t y = 0; y < resolution / 4; y++)
		for (int64_t x = 0; x < resolution / 4; x++)
		{
			float sum = 0;

			for (int64_t y_loc = y * 4; y_loc < (y * 4) + 4; y_loc++)
				for (int64_t x_loc = x * 4; x_loc < (x * 4) + 4; x_loc++)
					sum += height[idx(x_loc, y_loc, resolution)];

			output[idx(x, y, resolution / 4)] = sum / 16;
		}

	return output;
}

// Grants access to a flattened array
// inline tells the compiler to replace the method call with the method body
inline int64_t idx(int64_t x, int64_t y, int64_t size)
{
	return x + y * size;
}

// Smoothstep from https://en.wikipedia.org/wiki/Smoothstep
inline float smoothstep(float x)
{
	return x * x * (3.0f - 2.0f * x);
}

// std::clamp is only available in C++ 17+
inline float clamp(float x, float min, float max)
{
	return std::min(std::max(min, x), max);
}

inline float map_range(float x, float from_low, float from_high, float to_low, float to_high)
{
	return ((x - from_low) / (from_high - from_low)) * (to_high - to_low) + to_low;
}

inline GEDUtils::Vec3f blend(GEDUtils::Vec3f& a, GEDUtils::Vec3f& b, float alpha)
{
	GEDUtils::Vec3f result;
	result.x = a.x * (1.0f - alpha) + b.x * alpha;
	result.y = a.y * (1.0f - alpha) + b.y * alpha;
	result.z = a.z * (1.0f - alpha) + b.z * alpha;
	return result;
}

// Texture lookup with repeat
// u and v still have to be positive
inline GEDUtils::Vec3f texture(GEDUtils::SimpleImage& tex, UINT u, UINT v)
{
	GEDUtils::Vec3f result;
	tex.getPixel(u % tex.getWidth(), v % tex.getHeight(), result.x, result.y, result.z);
	return result;
}