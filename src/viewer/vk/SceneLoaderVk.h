#pragma once

#include <BufferPools.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <SamplerManager.Vulkan.h>
#include <SceneRendererBase.h>

#include <MathInc.h>
#include <Scene.h>

namespace apemode {
namespace vk {

// TODO
class SceneLoader {
public:
    struct MeshDeviceAsset : apemode::detail::SceneDeviceAsset {
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer;
        uint32_t                                         VertexCount = 0;
        uint32_t                                         IndexOffset = 0;
        VkIndexType                                      IndexType   = VK_INDEX_TYPE_UINT16;
    };

    struct MaterialDeviceAsset : apemode::detail::SceneDeviceAsset {
        const char* pszName = nullptr;

        const apemodevk::LoadedImage* pBaseColorLoadedImg         = nullptr;
        const apemodevk::LoadedImage* pNormalLoadedImg            = nullptr;
        const apemodevk::LoadedImage* pOcclusionLoadedImg         = nullptr;
        const apemodevk::LoadedImage* pEmissiveLoadedImg          = nullptr;
        const apemodevk::LoadedImage* pMetallicRoughnessLoadedImg = nullptr;

        VkSampler pBaseColorSampler = VK_NULL_HANDLE;
        VkSampler pNormalSampler    = VK_NULL_HANDLE;
        VkSampler pOcclusionSampler = VK_NULL_HANDLE;
        VkSampler pEmissiveSampler  = VK_NULL_HANDLE;
        VkSampler pMetallicSampler  = VK_NULL_HANDLE;
        VkSampler pRoughnessSampler = VK_NULL_HANDLE;

        apemodevk::THandle< VkImageView > hBaseColorImgView;
        apemodevk::THandle< VkImageView > hNormalImgView;
        apemodevk::THandle< VkImageView > hOcclusionImgView;
        apemodevk::THandle< VkImageView > hEmissiveImgView;
        apemodevk::THandle< VkImageView > hMetallicImgView;
        apemodevk::THandle< VkImageView > hRoughnessImgView;
    };

    struct DeviceAsset : apemode::detail::SceneDeviceAsset {
        apemodevk::unique_ptr< apemodevk::LoadedImage > MissingTextureZeros;
        apemodevk::unique_ptr< apemodevk::LoadedImage > MissingTextureOnes;
        apemode::SceneMaterial                          MissingMaterial;
        MaterialDeviceAsset                             MissingMaterialAsset;
        VkSampler                                       pMissingSampler = VK_NULL_HANDLE;
    };

    /* Updates device resources. */
    struct LoadParameters {
        /** @brief Staging buffer is sufficient to store the entire source image. */
        static constexpr size_t kStagingMemoryLimitHint_Unlimited = ~0ULL;
        /** @brief Tries to use the staging buffer of the least size. */
        static constexpr size_t kStagingMemoryLimitHint_Optimal = 0;
        static constexpr size_t kStagingMemoryLimitHint_16KB    = 16 * 1024;
        static constexpr size_t kStagingMemoryLimitHint_64KB    = 64 * 1024;
        static constexpr size_t kStagingMemoryLimitHint_2MB     = 2 * 1024 * 1024;

        apemodevk::GraphicsDevice* pNode                  = nullptr; /* Required */
        apemodevk::ImageLoader*    pImgLoader             = nullptr; /* Required */
        apemodevk::SamplerManager* pSamplerManager        = nullptr; /* Required */
        const apemodefb::SceneFb*  pSrcScene              = nullptr; /* Required */
        size_t                     StagingMemoryLimitHint = kStagingMemoryLimitHint_Optimal;
    };

    /* Updates device resources. */
    bool LoadScene( apemode::Scene* pScene, const LoadParameters* pLoadParams );
};

} // namespace vk
} // namespace apemode
