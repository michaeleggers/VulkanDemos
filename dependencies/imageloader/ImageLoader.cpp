#include <stdexcept>
#include "ImageLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace img
{
	ImageLoader::ImageLoader()
	{

	}
	ImageLoader::~ImageLoader()
	{

	}
	unsigned char* ImageLoader::LoadTextureFromFile(const std::string& fileName, int * widthOut, int* heightOut, bool topDown)
	{
		if (topDown) {
			stbi_set_flip_vertically_on_load(1);
		}
		int channels;
		unsigned char * data = stbi_load(fileName.c_str(), widthOut, heightOut, &channels, 4);
		return data;
	}
	
}
