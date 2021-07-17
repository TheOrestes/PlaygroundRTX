#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;
class VulkanTexture2D;
enum class TextureType;

class VulkanMaterial
{
public:
	VulkanMaterial();
	~VulkanMaterial();

	void									LoadTexture(VulkanDevice* pDevice, const std::string& filePath, TextureType type);
	void									Cleanup(VulkanDevice* pDevice);
	void									CleanupOnWindowResize(VulkanDevice* pDevice);

	std::map<TextureType, VulkanTexture2D*>	m_mapTextures;
};

