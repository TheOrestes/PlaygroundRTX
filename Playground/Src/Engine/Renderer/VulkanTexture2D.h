#pragma once

#include "vulkan/vulkan.h"

class VulkanDevice;

enum class TextureType
{
	TEXTURE_ALBEDO,
	TEXTURE_METALNESS,
	TEXTURE_NORMAL,
	TEXTURE_ROUGHNESS,
	TEXTURE_AO,
	TEXTURE_EMISSIVE,
	TEXTURE_HDRI,
	TEXTURE_ERROR
};

class VulkanTexture2D
{
public:
	VulkanTexture2D();
	~VulkanTexture2D();

	void								CreateTexture(VulkanDevice* pDevice, std::string fileName, TextureType eType);
	void								CreateTextureHDRI(VulkanDevice* pDevice, std::string fileName);
	void								Cleanup(VulkanDevice* pDevice);
	void								CleanupOnWindowResize(VulkanDevice* pDevice);

public:
	VkImage								m_vkTextureImage;
	VkImageView							m_vkTextureImageView;
	VkImageLayout						m_vkTextureImageLayout;
	VkDeviceMemory						m_vkTextureImageMemory;
	VkSampler							m_vkTextureSampler;

private:
	unsigned char*						LoadTextureFile(VulkanDevice* pDevice, std::string fileName);
	float*								LoadHDRI(VulkanDevice* pDevice, std::string fileName);
	void								CreateTextureImage(VulkanDevice* pDevice, std::string fileName);
	void								CreateTextureSampler(VulkanDevice* pDevice);

	int									m_iTextureWidth;
	int									m_iTextureHeight;
	int									m_iTextureChannels;
	VkDeviceSize						m_vkTextureDeviceSize;

	TextureType							m_eTextureType;
};

