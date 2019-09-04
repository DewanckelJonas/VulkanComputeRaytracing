#pragma once
#include <vector>
#include <fstream>

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary); //ate makes it so you start reading at the end of the file handy for knowing filesize.
	if(!file.is_open())
	{
		assert( 0 && "Failed to open file!");
		std::exit(-1);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}