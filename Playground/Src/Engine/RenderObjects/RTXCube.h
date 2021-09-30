#pragma once
#include "Engine/Helpers/Utility.h"
#include "SceneObject.h"

class RTXCube : public SceneObject
{
public:
    RTXCube();
    ~RTXCube();

public:
    void                                            Initialize(VulkanDevice* pDevice) override;
    void                                            Update(float dt) override;
    void                                            Render() override;
    void                                            Cleanup(VulkanDevice* pDevice) override;

private:    
    void                                            CreateBottomLevelAS(VulkanDevice* pDevice);

private:
    Vulkan::MeshData*                                m_pMeshData;

    // Actual Data
    std::vector<App::VertexP>                       m_vecVertices;
    std::vector<uint32_t>                           m_vecIndices;
};
