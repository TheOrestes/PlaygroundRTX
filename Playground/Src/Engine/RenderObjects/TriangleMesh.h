#pragma once

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "Engine/Helpers/Utility.h"
#include "SceneObject.h"

class TriangleMesh : public SceneObject
{
public:
    TriangleMesh(const std::string& filepath);
    ~TriangleMesh();

    void                                            Initialize(VulkanDevice* pDevice) override;
    void                                            Update(float dt) override;
    void                                            Render() override;
    void                                            Cleanup(VulkanDevice* pDevice) override;

private:
    void                                            LoadModel(const std::string& path);
    void                                            ProcessNode(aiNode* node, const aiScene* scene);
    void                                            ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void                                            CreateBottomLevelAS(VulkanDevice* pDevice);

private:
    std::vector<App::VertexP>                       m_vecVertices;
    std::vector<uint32_t>                           m_vecIndices;

    Vulkan::MeshData*                               m_pMeshData;
    std::string                                     m_FilePath;
};

