#include "imageLoader.h"
#include <vector>
#include <stdexcept>


#include <tinyEXR/tinyexr.h>

HDRI loadHDRI(const std::string& path)
{
	HDRI hdri;

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
