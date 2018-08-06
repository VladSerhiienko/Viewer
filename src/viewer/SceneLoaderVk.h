#pragma once

#include <MathInc.h>
#include <Scene.h>

#include <BufferPools.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <SamplerManager.Vulkan.h>
#include <SceneRendererBase.h>

namespace apemode {
struct Scene;

// TODO
class SceneUpdaterVk {
public:
    struct MeshDeviceAsset : apemode::SceneDeviceAsset {
        apemodevk::THandle< apemodevk::BufferComposite > hVertexBuffer;
        apemodevk::THandle< apemodevk::BufferComposite > hIndexBuffer;
        uint32_t                                         VertexCount = 0;
        uint32_t                                         IndexOffset = 0;
        VkIndexType                                      IndexType   = VK_INDEX_TYPE_UINT16;
    };

    struct MaterialDeviceAsset : apemode::SceneDeviceAsset {
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

    struct DeviceAsset : apemode::SceneDeviceAsset {
        apemodevk::unique_ptr< LoadedImage > MissingTextureZeros;
        apemodevk::unique_ptr< LoadedImage > MissingTextureOnes;
        apemode::SceneMaterial               MissingMaterial;
        apemodevk::MaterialDeviceAsset       MissingMaterialAsset;
        VkSampler                            pMissingSampler = VK_NULL_HANDLE;
    };

    struct UpdateParameters {
        apemodevk::GraphicsDevice* pNode           = nullptr; /* Required */
        apemodevk::ImageLoader*    pImgLoader      = nullptr; /* Required */
        apemodevk::SamplerManager* pSamplerManager = nullptr; /* Required */
    };

    bool UpdateScene( apemode::Scene* pScene, const UpdateParameters* pParams ) {
        return false;
    }
};
} // namespace apemode
