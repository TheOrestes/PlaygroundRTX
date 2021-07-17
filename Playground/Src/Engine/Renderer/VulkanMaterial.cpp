#include "PlaygroundPCH.h"
#include "VulkanMaterial.h"

#include "VulkanDevice.h"
#include "VulkanTexture2D.h"

#include "PlaygroundHeaders.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanMaterial::VulkanMaterial()
{
	m_mapTextures.clear();
}

//---------------------------------------------------------------------------------------------------------------------
VulkanMaterial::~VulkanMaterial()
{
	m_mapTextures.clear();
}


//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::LoadTexture(VulkanDevice* pDevice, const std::string& filePath, TextureType type)
{
	VulkanTexture2D* pTexture = new VulkanTexture2D();
	pTexture->CreateTexture(pDevice, filePath, type);

	m_mapTextures.emplace(type, pTexture);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::Cleanup(VulkanDevice* pDevice)
{
	std::map<TextureType, VulkanTexture2D*>::iterator iter = m_mapTextures.begin();
	for (; iter != m_mapTextures.end(); ++iter)
	{
		iter->second->Cleanup(pDevice);
	}
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanMaterial::CleanupOnWindowResize(VulkanDevice* pDevice)
{

}
