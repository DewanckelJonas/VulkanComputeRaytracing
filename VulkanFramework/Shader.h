#pragma once
#include "Platform.h"
#include <vector>

VkShaderModule CreateShaderModule(const std::vector<char>& code, VkDevice device)
{
	VkShaderModule shaderModule{};

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	ErrorCheck(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule));
	return shaderModule;
}