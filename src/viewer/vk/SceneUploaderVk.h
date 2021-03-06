#pragma once

#include <viewer/Scene.h>

#include <BufferPools.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <SamplerManager.Vulkan.h>

namespace apemode {
namespace vk {

// TODO
class SceneUploader {
public:
    struct MeshDeviceAsset : apemode::detail::SceneDeviceAsset {
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer;
        VkDeviceSize                                     VertexCount = 0;
        VkDeviceSize                                     IndexCount = 0;
        uint32_t                                         IndexOffset = 0;
        VkIndexType                                      eIndexType   = VK_INDEX_TYPE_UINT16;
    };

    struct MaterialDeviceAsset : apemode::detail::SceneDeviceAsset {
        const char* pszName = nullptr;

        const apemodevk::UploadedImage* pBaseColorImg         = nullptr;
        const apemodevk::UploadedImage* pNormalImg            = nullptr;
        const apemodevk::UploadedImage* pEmissiveImg          = nullptr;
        const apemodevk::UploadedImage* pMetallicRoughnessOcclusionImg = nullptr;

        VkSampler pBaseColorSampler = VK_NULL_HANDLE;
        VkSampler pNormalSampler    = VK_NULL_HANDLE;
        VkSampler pEmissiveSampler  = VK_NULL_HANDLE;
        VkSampler pMetallicRoughnessOcclusionSampler  = VK_NULL_HANDLE;

        apemodevk::THandle< VkImageView > hBaseColorImgView;
        apemodevk::THandle< VkImageView > hNormalImgView;
        apemodevk::THandle< VkImageView > hEmissiveImgView;
        apemodevk::THandle< VkImageView > hImgView;
        apemodevk::THandle< VkImageView > hMetallicRoughnessOcclusionImgView;
        
        bool bHasMetallicMap  = false;
        bool bHasRoughnessMap = false;
        bool bHasOcclusionMap = false;
    };

    struct DeviceAsset : apemode::detail::SceneDeviceAsset {
        using LoadedImagePtr = apemodevk::unique_ptr< apemodevk::UploadedImage >;

        apemodevk::unique_ptr< apemodevk::UploadedImage > MissingTextureZeros;
        apemodevk::unique_ptr< apemodevk::UploadedImage > MissingTextureOnes;
        apemode::SceneMaterial                            MissingMaterial;
        MaterialDeviceAsset                               MissingMaterialAsset;
        VkSampler                                         pMissingSampler = VK_NULL_HANDLE;
        apemode::vector< LoadedImagePtr >                 LoadedImgs;

        size_t MaxBoneCount = 0;
    };

    /* Updates device resources. */
    struct UploadParameters {
        /** @brief Staging buffer is sufficient to store the entire source image. */
        static constexpr size_t kStagingMemoryLimitHint_Unlimited = ~0ULL;
        /** @brief Tries to use the staging buffer of the least size. */
        static constexpr size_t kStagingMemoryLimitHint_Optimal = 0;
        static constexpr size_t kStagingMemoryLimitHint_16KB    = 16 * 1024;
        static constexpr size_t kStagingMemoryLimitHint_64KB    = 64 * 1024;
        static constexpr size_t kStagingMemoryLimitHint_2MB     = 2 * 1024 * 1024;

        apemodevk::GraphicsDevice* pNode                   = nullptr;                         /* Required */
        apemodevk::ImageUploader*  pImgUploader            = nullptr;                         /* Required */
        apemodevk::SamplerManager* pSamplerManager         = nullptr;                         /* Required */
        const apemodefb::SceneFb*  pSrcScene               = nullptr;                         /* Required */
        uint32_t                   MatUploadQueueFamilyId  = 0;                               /* Required */
        uint32_t                   MeshUploadQueueFamilyId = 0;                               /* Required */
        size_t                     StagingMemoryLimitHint  = kStagingMemoryLimitHint_Optimal; /* Required */
    };

    /* Updates device resources. */
    bool UploadScene( apemode::Scene* pScene, const UploadParameters* pLoadParams );
};

} // namespace vk
} // namespace apemode
