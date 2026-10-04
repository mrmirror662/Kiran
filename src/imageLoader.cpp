#include "imageLoader.h"
#include <vector>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include<iostream>

#include <tinyEXR/tinyexr.h>
#define STB_IMAGE_IMPLEMENTATION    

#include <STB/stb_image.h>
namespace imgutl {
	HDRI loadHDRI(const std::string& path)
	{
		HDRI hdri;

		// LDR images (jpg/png): 8-bit sRGB decoded to linear radiance.
		std::string ext = path.substr(path.find_last_of('.') + 1);
		for (auto& c : ext) c = static_cast<char>(std::tolower(c));
		if (ext != "exr")
		{
			int w, h, ch;
			unsigned char* px = stbi_load(path.c_str(), &w, &h, &ch, 3);
			if (!px)
				throw std::runtime_error("Failed to load environment image: " + path);
			auto lin = [](unsigned char v) {
				float c = v / 255.0f;
				return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
			};
			hdri.width = w;
			hdri.height = h;
			hdri.data.reserve(size_t(w) * h);
			for (size_t i = 0; i < size_t(w) * h; ++i)
				hdri.data.emplace_back(lin(px[3 * i]), lin(px[3 * i + 1]), lin(px[3 * i + 2]), 1.0f);
			stbi_image_free(px);
			return hdri;
		}

		// Variables to store the EXR image information
		float* out;  // Output array of floats for image data
		int width;
		int height;
		const char* err = nullptr;

		// Load the EXR image using TinyEXR
		int ret = LoadEXR(&out, &width, &height, path.c_str(), &err);

		if (ret != TINYEXR_SUCCESS)
		{
			if (err)
			{
				std::string error_message = "Failed to load EXR image: " + path + " Error: " + std::string(err);
				FreeEXRErrorMessage(err);  // Free the error message memory
				throw std::runtime_error(error_message);
			}
			else
			{
				throw std::runtime_error("Failed to load EXR image: " + path);
			}
		}

		hdri.width = width;
		hdri.height = height;
		hdri.data.reserve(width * height);

		// Each pixel has 4 channels (RGBA), even if the image doesn't use all 4 channels
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				glm::vec4 pixel;
				int idx = (y * width + x) * 4;  // 4 channels per pixel

				// Fill channels based on the loaded channels count
				pixel.r = out[idx];
				pixel.g = out[idx + 1];
				pixel.b = out[idx + 2];
				pixel.a = 1.0;

				hdri.data.push_back(pixel);

			}
		}

		free(out);  // Free the loaded EXR data

		return hdri;
	}
	Image LoadImageFromPath(const std::string& path)
	{
		int width, height, channels;
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

		Image image;
		if (data) {
			// Copy image data to vector buffer
			image.buffer.assign(data, data + (width * height * channels));
			image.width = width;
			image.height = height;
			image.channel = channels;

			// Free STB image memory
			stbi_image_free(data);
		}
		else {
			std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
		}

		return image;
	}
}