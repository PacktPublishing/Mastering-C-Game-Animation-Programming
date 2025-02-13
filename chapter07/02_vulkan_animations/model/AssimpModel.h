/* Assimp model, ready to draw */
#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>

#include <glm/glm.hpp>

#include "Texture.h"
#include "AssimpMesh.h"
#include "AssimpNode.h"
#include "AssimpAnimClip.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ShaderStorageBuffer.h"
#include "ModelSettings.h"

#include "VkRenderData.h"

class AssimpModel {
  public:
    bool loadModel(VkRenderData &renderData, std::string modelFilename, unsigned int extraImportFlags = 0);
    glm::mat4 getRootTranformationMatrix();

    void draw(VkRenderData &renderData, bool selectionModeActive);
    void drawInstanced(VkRenderData &renderData, uint32_t instanceCount, bool selectionModeActive);
    unsigned int getTriangleCount();

    std::string getModelFileName();
    std::string getModelFileNamePath();

    bool hasAnimations();
    const std::vector<std::shared_ptr<AssimpAnimClip>>& getAnimClips();
    float getMaxClipDuration();

    const std::vector<std::shared_ptr<AssimpNode>>& getNodeList();
    const std::unordered_map<std::string, std::shared_ptr<AssimpNode>>& getNodeMap();

    const std::vector<std::shared_ptr<AssimpBone>>& getBoneList();
    const std::vector<std::string> getBoneNameList();

    VkShaderStorageBufferData& getBoneMatrixOffsetBuffer();
    VkShaderStorageBufferData& getBoneParentBuffer();
    VkShaderStorageBufferData& getAnimLookupBuffer();

    void setModelSettings(ModelSettings settings);
    ModelSettings getModelSettings();

    VkDescriptorSet& getTransformDescriptorSet();
    VkDescriptorSet& getMatrixMultDescriptorSet();

    void cleanup(VkRenderData &renderData);

private:
    void processNode(VkRenderData &renderData, std::shared_ptr<AssimpNode> node, aiNode* aNode, const aiScene* scene, std::string assetDirectory);
    void createNodeList(std::shared_ptr<AssimpNode> node, std::shared_ptr<AssimpNode> newNode, std::vector<std::shared_ptr<AssimpNode>> &list);

    bool createDescriptorSet(VkRenderData &renderData);

    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    float mMaxClipDuration = 0.0f;

    /* store the root node for direct access */
    std::shared_ptr<AssimpNode> mRootNode = nullptr;
    /* a map to find the node by name */
    std::unordered_map<std::string, std::shared_ptr<AssimpNode>> mNodeMap{};
    /* and a 'flat' map to keep the order of insertation  */
    std::vector<std::shared_ptr<AssimpNode>> mNodeList{};

    std::vector<std::shared_ptr<AssimpBone>> mBoneList;
    std::vector<std::string> mBoneNameList{};

    std::vector<std::shared_ptr<AssimpAnimClip>> mAnimClips{};

    std::vector<VkMesh> mModelMeshes{};
    std::vector<VkVertexBufferData> mVertexBuffers{};
    std::vector<VkIndexBufferData> mIndexBuffers{};

    VkShaderStorageBufferData mShaderBoneParentBuffer{};
    VkShaderStorageBufferData mShaderBoneMatrixOffsetBuffer{};
    VkShaderStorageBufferData mAnimLookupBuffer{};

    // map textures to external or internal texture names
    std::unordered_map<std::string, VkTextureData> mTextures{};
    VkTextureData mPlaceholderTexture{};

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);

    ModelSettings mModelSettings{};

    VkDescriptorSet mTransformPerModelDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet mMatrixMultPerModelDescriptorSet = VK_NULL_HANDLE;
};
