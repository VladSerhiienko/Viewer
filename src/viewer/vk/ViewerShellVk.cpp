#include "ViewerShellVk.h"
#include <apemode/platform/memory/MemoryManager.h>
#include <apemode/vk/TOneTimeCmdBufferSubmit.Vulkan.h>

using namespace apemode::viewer::vk;

namespace apemode {
using namespace apemodexm;
using namespace apemodevk;

namespace platform {
static void DebugBreak( ) {
    apemodevk::platform::DebugBreak( );
}
} // namespace platform
} // namespace apemode

// clang-format off
class apemode::viewer::vk::GraphicsAllocator : public apemodevk::GraphicsManager::IAllocator {
public:
    void* Malloc( size_t             size,
                  size_t             alignment,
                  const char*        sourceFile,
                  const unsigned int sourceLine,
                  const char*        sourceFunc ) override {
        return apemode::platform::allocate( size, alignment, sourceFile, sourceLine, sourceFunc );
    }

    void* Realloc( void*              pOriginal,
                   size_t             size,
                   size_t             alignment,
                   const char*        sourceFile,
                   const unsigned int sourceLine,
                   const char*        sourceFunc ) override {
        return apemode::platform::reallocate( pOriginal, size, alignment, sourceFile, sourceLine, sourceFunc );
    }

    void Free( void*              pMemory,
               const char*        sourceFile,
               const unsigned int sourceLine,
               const char*        sourceFunc ) override {
        return apemode::platform::deallocate( pMemory, sourceFile, sourceLine, sourceFunc );
    }

    void GetPrevMemoryAllocationScope( const char*&  pszPrevSourceFile,
                                       unsigned int& prevSourceLine,
                                       const char*&  pszPrevSourceFunc ) const override {
        apemode::platform::GetPrevMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
    }

    void StartMemoryAllocationScope( const char*        pszSourceFile,
                                     const unsigned int sourceLine,
                                     const char*        pszSourceFunc ) const override {
        apemode::platform::StartMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
    }

    void EndMemoryAllocationScope( const char*        pszPrevSourceFile,
                                   const unsigned int prevSourceLine,
                                   const char*        pszPrevSourceFunc ) const override {
        apemode::platform::EndMemoryAllocationScope( pszPrevSourceFile, prevSourceLine, pszPrevSourceFunc );
    }
};

class apemode::viewer::vk::GraphicsLogger : public apemodevk::GraphicsManager::ILogger {
public:
    spdlog::logger* pLogger = apemode::AppState::Get( )->GetLogger( );
    void Log( const LogLevel eLevel, const char* pszMsg ) override {
        pLogger->log( static_cast< spdlog::level::level_enum >( eLevel ), pszMsg );
    }
};

// clang-format on

const VkFormat sDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
// const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

const uint16_t kAnimLayerId   = 0;
const uint16_t kAnimStackId   = 0;

ViewerShell::ViewerShell( ) {
    apemode_memory_allocation_scope;
    pCamInput = apemode::unique_ptr< CameraControllerInputBase >( apemode_new MouseKeyboardCameraControllerInput( ) );

#if _WIN32
    pCamController = apemode::unique_ptr< CameraControllerBase >( apemode_new FreeLookCameraController( ) );
    auto pFreeLookCameraController = (FreeLookCameraController*)pCamController.get();
    pFreeLookCameraController->Position.x = 50;
    pFreeLookCameraController->Position.y = 50;
    pFreeLookCameraController->Position.z = 50;
    pFreeLookCameraController->PositionDst.x = 50;
    pFreeLookCameraController->PositionDst.y = 50;
    pFreeLookCameraController->PositionDst.z = 50;
#else
    //*

    pCamController = apemode::unique_ptr< CameraControllerBase >( apemode_new ModelViewCameraController( ) );
    auto pModelViewCameraController = (ModelViewCameraController*)pCamController.get();

    float position = 350;
    float destPosition = 100;

    pModelViewCameraController->Target.x = 0;
    pModelViewCameraController->Target.y = 0;
    pModelViewCameraController->Target.z = 0;
    pModelViewCameraController->TargetDst.x = 0;
    pModelViewCameraController->TargetDst.y = 28;
    pModelViewCameraController->TargetDst.z = 0;

    pModelViewCameraController->Position.x = position;
    pModelViewCameraController->Position.y = position;
    pModelViewCameraController->Position.z = position;
//    pModelViewCameraController->PositionDst.x = destPosition;
//    pModelViewCameraController->PositionDst.y = destPosition;
//    pModelViewCameraController->PositionDst.z = destPosition;
    pModelViewCameraController->PositionDst.x = 4;
    pModelViewCameraController->PositionDst.y = 60;
    pModelViewCameraController->PositionDst.z = -40;

    //*/
#endif
}

ViewerShell::~ViewerShell( ) {
    LogInfo( "ViewerShell: Destroyed." );
}


void ViewerShell::SetAssetManager( apemode::platform::IAssetManager* pInAssetManager ) {
    assert( pInAssetManager );
    pAssetManager = pInAssetManager;
    LogInfo( "ViewerShell: AssetManager={}", (void*)pInAssetManager );
}

bool ViewerShell::Initialize( const apemode::PlatformSurface* pPlatformSurface ) {
    apemode_memory_allocation_scope;
    LogInfo( "ViewerApp: Initializing." );

    Logger    = apemode::make_unique< GraphicsLogger >( );
    Allocator = apemode::make_unique< GraphicsAllocator >( );

    apemode::vector< const char* > ppszLayers;
    apemode::vector< const char* > ppszExtensions;
    size_t                         optionalLayerCount     = 0;
    size_t                         optionalExtentionCount = 0;

#ifdef __APPLE__
    ppszLayers.push_back( "MoltenVK" );
#else

    if ( TGetOption< bool >( "renderdoc", false ) ) {
        ppszLayers.push_back( "VK_LAYER_RENDERDOC_Capture" );
    }

    if ( TGetOption< bool >( "vktrace", false ) ) {
        ppszLayers.push_back( "VK_LAYER_LUNARG_vktrace" );
    }

    if ( TGetOption< bool >( "vkapidump", false ) ) {
        ppszLayers.push_back( "VK_LAYER_LUNARG_api_dump" );
    }

#endif

    if ( Surface.Initialize( pPlatformSurface,
                             Logger.get( ),
                             Allocator.get( ),
                             (const char**) ppszLayers.data( ),
                             ppszLayers.size( ) - optionalExtentionCount,
                             optionalExtentionCount,
                             (const char**) ppszExtensions.data( ),
                             ppszExtensions.size( ) - optionalLayerCount,
                             optionalLayerCount ) ) {
        // apemode::string8 assetsFolder( TGetOption< std::string >( "--assets", "./" ).c_str( ) );
        // pAssetManager->UpdateAssets( assetsFolder.c_str( ), nullptr, 0 );

        TotalSecs = 0.0f;

        FrameId    = 0;
        FrameIndex = Surface.Swapchain.GetBufferCount( );
        Frames.resize( Surface.Swapchain.GetBufferCount( ) );

        OnResized( );

        for ( uint32_t i = 0; i < Frames.size( ); ++i ) {
            VkSemaphoreCreateInfo semaphoreCreateInfo;
            InitializeStruct( semaphoreCreateInfo );
            if ( ! Frames[ i ].hPresentCompleteSemaphore.Recreate( Surface.Node, semaphoreCreateInfo ) ||
                 ! Frames[ i ].hRenderCompleteSemaphore.Recreate( Surface.Node, semaphoreCreateInfo ) ) {
                return false;
            }
        }

        /*
        VK_DESCRIPTOR_TYPE_SAMPLER = 0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
        */

        apemodevk::DescriptorPool::InitializeParameters descPoolInitParameters;
        descPoolInitParameters.pNode                                                               = &Surface.Node;
        descPoolInitParameters.MaxDescriptorSetCount                                               = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_SAMPLER ]                = 64;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ] = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ]          = 512;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ] = 512;

        if ( ! DescriptorPool.Initialize( eastl::move( descPoolInitParameters ) ) ) {
            return false;
        }

        pSamplerManager = apemode::make_unique< apemodevk::SamplerManager >( );
        if ( ! pSamplerManager->Recreate( &Surface.Node ) ) {
            return false;
        }

        apemodevk::ImageUploader imgUploader;
        apemodevk::ImageDecoder  imgDecoder;

        if ( auto pTexAsset = pAssetManager->Acquire( "images/Environment/kyoto_lod.dds" ) ) {
            // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/output_skybox.dds" ) ) {
            // if ( auto pTexAsset = pAssetManager->Acquire( "images/Environment/bolonga_lod.dds" ) ) {
            apemode_memory_allocation_scope;
            {
                const auto texAssetBin = pTexAsset->GetContentAsBinaryBuffer( );
                pAssetManager->Release( pTexAsset );

                apemodevk::ImageDecoder::DecodeOptions decodeOptions;
                decodeOptions.eFileFormat      = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_DDS;
                decodeOptions.bGenerateMipMaps = true;

                apemodevk::ImageUploader::UploadOptions loadOptions;
                loadOptions.bImgView = true;

                auto srcImg = imgDecoder.DecodeSourceImageFromData( texAssetBin.data( ), texAssetBin.size( ), decodeOptions );
                RadianceImg = imgUploader.UploadImage( &Surface.Node, *srcImg, loadOptions );
            }

            VkSamplerCreateInfo samplerCreateInfo;
            apemodevk::InitializeStruct( samplerCreateInfo );

            samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.anisotropyEnable        = true;
            samplerCreateInfo.maxAnisotropy           = 16;
            samplerCreateInfo.compareEnable           = false;
            samplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
            samplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
            samplerCreateInfo.minLod                  = 0;
            samplerCreateInfo.maxLod                  = float( RadianceImg->ImgCreateInfo.mipLevels );
            samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            samplerCreateInfo.unnormalizedCoordinates = false;

            pRadianceCubeMapSampler = pSamplerManager->GetSampler( samplerCreateInfo );
        }

        if ( auto pTexAsset = pAssetManager->Acquire( "images/Environment/kyoto_irr.dds" ) ) {
            // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/output_iem.dds" ) ) {
            // if ( auto pTexAsset = pAssetManager->Acquire( "images/Environment/bolonga_irr.dds" ) ) {
            apemode_memory_allocation_scope;
            {
                const auto texAssetBin = pTexAsset->GetContentAsBinaryBuffer( );
                pAssetManager->Release( pTexAsset );

                apemodevk::ImageDecoder::DecodeOptions decodeOptions;
                decodeOptions.eFileFormat      = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_DDS;
                decodeOptions.bGenerateMipMaps = false;

                apemodevk::ImageUploader::UploadOptions loadOptions;
                loadOptions.bImgView = true;

                auto srcImg   = imgDecoder.DecodeSourceImageFromData( texAssetBin.data( ), texAssetBin.size( ), decodeOptions );
                IrradianceImg = imgUploader.UploadImage( &Surface.Node, *srcImg, loadOptions );
            }

            VkSamplerCreateInfo samplerCreateInfo;
            apemodevk::InitializeStruct( samplerCreateInfo );

            samplerCreateInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.anisotropyEnable        = true;
            samplerCreateInfo.maxAnisotropy           = 16;
            samplerCreateInfo.compareEnable           = false;
            samplerCreateInfo.compareOp               = VK_COMPARE_OP_NEVER;
            samplerCreateInfo.magFilter               = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter               = VK_FILTER_LINEAR;
            samplerCreateInfo.minLod                  = 0;
            samplerCreateInfo.maxLod                  = float( IrradianceImg->ImgCreateInfo.mipLevels );
            samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            samplerCreateInfo.unnormalizedCoordinates = false;

            pIrradianceCubeMapSampler = pSamplerManager->GetSampler( samplerCreateInfo );
        }

        pNkRenderer = apemode::unique_ptr< apemode::vk::NuklearRenderer >( apemode_new apemode::vk::NuklearRenderer( ) );

        auto pFontAsset = pAssetManager->Acquire( "fonts/Mecha.ttf" );
        // auto pFontAsset = pAssetManager->Acquire( "fonts/iosevka-ss07-medium.ttf" );

        apemode::vk::NuklearRenderer::InitParameters initParamsNk;
        initParamsNk.pNode           = &Surface.Node;
        initParamsNk.pAssetManager   = pAssetManager;
        initParamsNk.pFontAsset      = pFontAsset;
        initParamsNk.pSamplerManager = pSamplerManager.get( );
        initParamsNk.pDescPool       = DescriptorPool;
        initParamsNk.pRenderPass     = hDbgRenderPass;
        initParamsNk.FrameCount      = uint32_t( Frames.size( ) );
        // initParamsNk.pRenderPass     = hNkRenderPass;

        pNkRenderer->Init( &initParamsNk );
        pAssetManager->Release( pFontAsset );

        apemode::vk::DebugRenderer::InitParameters initParamsDbg;
        initParamsDbg.pNode         = &Surface.Node;
        initParamsDbg.pAssetManager = pAssetManager;
        initParamsDbg.pRenderPass   = hDbgRenderPass;
        initParamsDbg.pDescPool     = DescriptorPool;
        initParamsDbg.FrameCount    = uint32_t( Frames.size( ) );

        pDebugRenderer = apemode::unique_ptr< apemode::vk::DebugRenderer >( apemode_new apemode::vk::DebugRenderer( ) );
        pDebugRenderer->RecreateResources( &initParamsDbg );

        pSceneRenderer = apemode::unique_ptr< apemode::vk::SceneRenderer >( apemode_new apemode::vk::SceneRenderer( ) );

        apemode::vk::SceneRenderer::RecreateParameters recreateParams;
        recreateParams.pNode         = &Surface.Node;
        recreateParams.pAssetManager = pAssetManager;
        recreateParams.pRenderPass   = hDbgRenderPass;
        recreateParams.pDescPool     = DescriptorPool;
        recreateParams.FrameCount    = uint32_t( Frames.size( ) );

        /*
        --assets "/Users/vlad.serhiienko/Projects/Home/Viewer/assets" --scene
        "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/sphere-bot.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/bristleback-dota-fan-art.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/wild-west-motorcycle.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/wild-west-sniper-rifle.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/rank-3-police-unit.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/wasteland-hunters-vehicule.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/snow-road-raw-scan-freebie.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/skull_salazar.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/graograman.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/warcraft-draenei-fanart.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/1972-datsun-240k-gt.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/dreadroamer-free.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/knight-artorias.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/rainier-ak-3ds.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/leather-shoes.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/terrarium-bot.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/car-lego-technic-42010.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/9-mm.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/colt.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/cerberus.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/gunslinger.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/pickup-truck.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/rock-jacket-mid-poly.fbxp"
        --assets "..\..\assets\**" --scene "C:/Sources/Models/FbxPipeline/mech-m-6k.fbxp"
        */

        if ( ! pSceneRenderer->Recreate( &recreateParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        // const std::string sceneFile = "shared/horned_infernal_duke.fbxp";
        // const std::string sceneFile = "shared/alien.fbxp";
        // const std::string sceneFile = "shared/opel-gt-retopo.fbxp";
        // const std::string sceneFile = "shared/flare-gun.fbxp";
        // const std::string sceneFile = "shared/stesla_8.fbxp";
        // const std::string sceneFile = "shared/horned_infernal_duke_8.fbxp";
        // const std::string sceneFile = "shared/0004_16.fbxp";
        // const std::string sceneFile = "shared/0004.fbxp";
        // const std::string sceneFile = "shared/0005.fbxp";
        const std::string sceneFile = "shared/horned_infernal_duke.fbxp";
        // TGetOption< std::string >( "scene", "" );
        auto pSceneAsset = pAssetManager->Acquire( sceneFile.c_str() );
        mLoadedScene = LoadSceneFromBin( pSceneAsset->GetContentAsBinaryBuffer() );

        apemode::vk::SceneUploader::UploadParameters uploadParams;
        uploadParams.pSamplerManager = pSamplerManager.get( );
        uploadParams.pSrcScene       = mLoadedScene.pSrcScene;
        uploadParams.pImgUploader    = &imgUploader;
        uploadParams.pNode           = &Surface.Node;

        apemode::vk::SceneUploader sceneUploader;
        if ( ! sceneUploader.UploadScene( mLoadedScene.pScene.get( ), &uploadParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        apemode::vk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode         = &Surface.Node;
        skyboxRendererRecreateParams.pAssetManager = pAssetManager;
        skyboxRendererRecreateParams.pRenderPass   = hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool     = DescriptorPool;
        skyboxRendererRecreateParams.FrameCount    = uint32_t( Frames.size( ) );

        pSkyboxRenderer = apemode::make_unique< apemode::vk::SkyboxRenderer >( );
        if ( ! pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        pSkybox                = apemode::make_unique< apemode::vk::Skybox >( );
        pSkybox->pSampler      = pRadianceCubeMapSampler;
        pSkybox->pImgView      = RadianceImg->hImgView;
        pSkybox->Dimension     = RadianceImg->ImgCreateInfo.extent.width;
        pSkybox->eImgLayout    = RadianceImg->eImgLayout;
        pSkybox->Exposure      = 3;
        pSkybox->LevelOfDetail = 0;

        LightDirection = XMFLOAT4( 0, 1, 0, 1 );
        LightColor     = XMFLOAT4( 1, 1, 1, 1 );

        Stopwatch.Start( );
        return true;
    }

    return false;
}

bool apemode::viewer::vk::ViewerShell::OnResized( ) {
    apemode_memory_allocation_scope;

    VkImageCreateInfo depthImgCreateInfo;
    InitializeStruct( depthImgCreateInfo );
    depthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
    depthImgCreateInfo.format        = sDepthFormat;
    depthImgCreateInfo.extent.width  = Surface.Swapchain.ImgExtent.width;
    depthImgCreateInfo.extent.height = Surface.Swapchain.ImgExtent.height;
    depthImgCreateInfo.extent.depth  = 1;
    depthImgCreateInfo.mipLevels     = 1;
    depthImgCreateInfo.arrayLayers   = 1;
    depthImgCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    depthImgCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    depthImgCreateInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageViewCreateInfo depthImgViewCreateInfo;
    InitializeStruct( depthImgViewCreateInfo );
    depthImgViewCreateInfo.format                          = sDepthFormat;
    depthImgViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    depthImgViewCreateInfo.subresourceRange.levelCount     = 1;
    depthImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthImgViewCreateInfo.subresourceRange.layerCount     = 1;
    depthImgViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationCreateInfo.flags                   = 0;

    for ( uint32_t i = 0; i < Frames.size( ); ++i ) {
        if ( ! Frames[ i ].hDepthImg.Recreate( Surface.Node.hAllocator, depthImgCreateInfo, allocationCreateInfo ) ) {
            return false;
        }
    }

    for ( uint32_t i = 0; i < Frames.size( ); ++i ) {
        depthImgViewCreateInfo.image = Frames[ i ].hDepthImg;
        if ( ! Frames[ i ].hDepthImgView.Recreate( Surface.Node, depthImgViewCreateInfo ) ) {
            return false;
        }
    }

    VkAttachmentDescription colorDepthAttachments[ 2 ];
    InitializeStruct( colorDepthAttachments );

    colorDepthAttachments[ 0 ].format         = Surface.Surface.eColorFormat;
    colorDepthAttachments[ 0 ].samples        = VK_SAMPLE_COUNT_1_BIT;
    colorDepthAttachments[ 0 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorDepthAttachments[ 0 ].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorDepthAttachments[ 0 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorDepthAttachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorDepthAttachments[ 0 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorDepthAttachments[ 0 ].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    colorDepthAttachments[ 1 ].format         = sDepthFormat;
    colorDepthAttachments[ 1 ].samples        = VK_SAMPLE_COUNT_1_BIT;
    colorDepthAttachments[ 1 ].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorDepthAttachments[ 1 ].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE; // ?
    colorDepthAttachments[ 1 ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorDepthAttachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorDepthAttachments[ 1 ].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorDepthAttachments[ 1 ].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef;
    InitializeStruct( colorAttachmentRef );
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef;
    InitializeStruct( depthAttachmentRef );
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassNk;
    InitializeStruct( subpassNk );
    subpassNk.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassNk.colorAttachmentCount = 1;
    subpassNk.pColorAttachments    = &colorAttachmentRef;

    VkSubpassDescription subpassDbg;
    InitializeStruct( subpassDbg );
    subpassDbg.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDbg.colorAttachmentCount    = 1;
    subpassDbg.pColorAttachments       = &colorAttachmentRef;
    subpassDbg.pDepthStencilAttachment = &depthAttachmentRef;

    VkRenderPassCreateInfo renderPassCreateInfoNk;
    InitializeStruct( renderPassCreateInfoNk );
    renderPassCreateInfoNk.attachmentCount = 1;
    renderPassCreateInfoNk.pAttachments    = &colorDepthAttachments[ 0 ];
    renderPassCreateInfoNk.subpassCount    = 1;
    renderPassCreateInfoNk.pSubpasses      = &subpassNk;

    VkRenderPassCreateInfo renderPassCreateInfoDbg;
    InitializeStruct( renderPassCreateInfoDbg );
    renderPassCreateInfoDbg.attachmentCount = 2;
    renderPassCreateInfoDbg.pAttachments    = &colorDepthAttachments[ 0 ];
    renderPassCreateInfoDbg.subpassCount    = 1;
    renderPassCreateInfoDbg.pSubpasses      = &subpassDbg;

    if ( ! hNkRenderPass.Recreate( Surface.Node, renderPassCreateInfoNk ) ) {
        apemode::platform::DebugBreak( );
        return false;
    }

    if ( ! hDbgRenderPass.Recreate( Surface.Node, renderPassCreateInfoDbg ) ) {
        apemode::platform::DebugBreak( );
        return false;
    }

    VkFramebufferCreateInfo framebufferCreateInfoNk;
    InitializeStruct( framebufferCreateInfoNk );
    framebufferCreateInfoNk.renderPass      = hNkRenderPass;
    framebufferCreateInfoNk.attachmentCount = 1;
    framebufferCreateInfoNk.width           = Surface.Swapchain.ImgExtent.width;
    framebufferCreateInfoNk.height          = Surface.Swapchain.ImgExtent.height;
    framebufferCreateInfoNk.layers          = 1;

    VkFramebufferCreateInfo framebufferCreateInfoDbg;
    InitializeStruct( framebufferCreateInfoDbg );
    framebufferCreateInfoDbg.renderPass      = hDbgRenderPass;
    framebufferCreateInfoDbg.attachmentCount = 2;
    framebufferCreateInfoDbg.width           = Surface.Swapchain.ImgExtent.width;
    framebufferCreateInfoDbg.height          = Surface.Swapchain.ImgExtent.height;
    framebufferCreateInfoDbg.layers          = 1;

    for ( uint32_t i = 0; i < Frames.size( ); ++i ) {
        VkImageView attachments[ 1 ]         = {Surface.Swapchain.Buffers[ i ].hImgView};
        framebufferCreateInfoNk.pAttachments = attachments;

        if ( ! Frames[ i ].hNkFramebuffer.Recreate( Surface.Node, framebufferCreateInfoNk ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }
    }

    for ( uint32_t i = 0; i < Frames.size( ); ++i ) {
        VkImageView attachments[ 2 ]          = {Surface.Swapchain.Buffers[ i ].hImgView, Frames[ i ].hDepthImgView};
        framebufferCreateInfoDbg.pAttachments = attachments;

        if ( ! Frames[ i ].hDbgFramebuffer.Recreate( Surface.Node, framebufferCreateInfoDbg ) ) {
            return false;
        }
    }
    
    for ( uint32_t i = 0; i < false && Frames.size( ); ++i ) {
        
        VkImageCreateInfo sceneImgCreateInfo;
        InitializeStruct( sceneImgCreateInfo );
        
        sceneImgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        sceneImgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        sceneImgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        sceneImgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        sceneImgCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        sceneImgCreateInfo.format = Surface.Surface.eColorFormat;
        sceneImgCreateInfo.extent.width = Surface.Swapchain.ImgExtent.width;
        sceneImgCreateInfo.extent.height = Surface.Swapchain.ImgExtent.height;
        sceneImgCreateInfo.extent.depth = 1;
        sceneImgCreateInfo.arrayLayers = 1;
        sceneImgCreateInfo.mipLevels = 1;

        for ( uint32_t ii = 0; ii < apemodevk::utils::GetArraySizeU( Frames[i].hSceneImgs ); ++ii ) {
            if ( ! Frames[ i ].hSceneImgs[ii].Recreate( Surface.Node.hAllocator, sceneImgCreateInfo, allocationCreateInfo ) ) {
                return false;
            }
        }

        VkImageViewCreateInfo sceneImgViewCreateInfo;
        InitializeStruct( sceneImgViewCreateInfo );
        sceneImgViewCreateInfo.format                          = sceneImgCreateInfo.format;
        sceneImgViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        sceneImgViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        sceneImgViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        sceneImgViewCreateInfo.subresourceRange.levelCount     = 1;
        sceneImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        sceneImgViewCreateInfo.subresourceRange.layerCount     = 1;
        
        for ( uint32_t ii = 0; ii < apemodevk::utils::GetArraySizeU( Frames[i].hSceneImgs ); ++ii ) {
            depthImgViewCreateInfo.image = Frames[ i ].hSceneImgs[ii].Handle.pImg;
            
            if ( ! Frames[ i ].hSceneImgViews[ii].Recreate( Surface.Node, sceneImgViewCreateInfo ) ) {
                return false;
            }
        }
        
//        VkFramebufferCreateInfo framebufferCreateInfoDbg;
//        InitializeStruct( framebufferCreateInfoDbg );
//        framebufferCreateInfoDbg.renderPass      = hDbgRenderPass;
//        framebufferCreateInfoDbg.attachmentCount = 2;
//        framebufferCreateInfoDbg.width           = Surface.Swapchain.ImgExtent.width;
//        framebufferCreateInfoDbg.height          = Surface.Swapchain.ImgExtent.height;
//        framebufferCreateInfoDbg.layers          = 1;
//
//        for ( uint32_t ii = 0; ii < apemodevk::utils::GetArraySizeU( Frames[i].hSceneImgs ); ++ii ) {
//            depthImgViewCreateInfo.image = Frames[ i ].hSceneImgs[ii].Handle.pImg;
//            if ( ! Frames[ i ].hSceneImgViews[ii].Recreate( Surface.Node, sceneImgViewCreateInfo ) ) {
//                return false;
//            }
//        }
    }

    return true;
}

void ViewerShell::UpdateUI( const VkExtent2D currentExtent, const apemode::platform::AppInput* pAppInput ) {

    auto pNkContext = &pNkRenderer->Context;

    constexpr uint32_t windowFlags = NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE;
    // apemodevk::platform::LogFmt(apemodevk::platform::Info, "nk_begin");
    if ( nk_begin( pNkContext, "Viewer", nk_rect( 10, 10, 180, 500 ), windowFlags ) ) {
        auto invViewMatrix = GetMatrix( XMMatrixInverse( nullptr, pCamController->ViewMatrix( ) ) );

        if ( nk_tree_push( pNkContext, NK_TREE_NODE, "View Matrix", NK_MINIMIZED ) ) {
            nk_layout_row_dynamic( pNkContext, 30, 1 );
            nk_labelf( pNkContext,
                       NK_TEXT_LEFT,
                       "View: %2.2f %2.2f %2.2f %2.2f ",
                       invViewMatrix._11,
                       invViewMatrix._12,
                       invViewMatrix._13,
                       invViewMatrix._14 );
            nk_labelf( pNkContext,
                       NK_TEXT_LEFT,
                       "    : %2.2f %2.2f %2.2f %2.2f ",
                       invViewMatrix._21,
                       invViewMatrix._22,
                       invViewMatrix._23,
                       invViewMatrix._24 );
            nk_labelf( pNkContext,
                       NK_TEXT_LEFT,
                       "    : %2.2f %2.2f %2.2f %2.2f ",
                       invViewMatrix._31,
                       invViewMatrix._32,
                       invViewMatrix._33,
                       invViewMatrix._34 );
            nk_labelf( pNkContext,
                       NK_TEXT_LEFT,
                       "    : %2.2f %2.2f %2.2f %2.2f ",
                       invViewMatrix._41,
                       invViewMatrix._42,
                       invViewMatrix._43,
                       invViewMatrix._44 );

            // nk_labelf( pNkContext, NK_TEXT_LEFT, "bEnableAnimations" );
            // nk_checkbox_label( pNkContext, "", &bEnableAnimations );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "WorldRotationY" );
            nk_slider_float( pNkContext, 0, &WorldRotationY, apemodexm::XM_2PI, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightColorR" );
            nk_slider_float( pNkContext, 0, &LightColor.x, 1, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightColorG" );
            nk_slider_float( pNkContext, 0, &LightColor.y, 1, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightColorB" );
            nk_slider_float( pNkContext, 0, &LightColor.z, 1, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightDirectionX" );
            nk_slider_float( pNkContext, -1, &LightDirection.x, 1, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightDirectionY" );
            nk_slider_float( pNkContext, -1, &LightDirection.y, 1, 0.1f );

            nk_labelf( pNkContext, NK_TEXT_LEFT, "LightDirectionZ" );
            nk_slider_float( pNkContext, -1, &LightDirection.z, 1, 0.1f );

            nk_tree_pop( pNkContext );
        }
    }
    nk_end( pNkContext );

    bIsUsingUI = 1 == pNkRenderer->HandleInput( pAppInput );
}

void ViewerShell::UpdateTime( ) {
    DeltaSecs = (float) Stopwatch.GetElapsedSeconds( );
    Stopwatch.Start( );

    FrameIndex = uint32_t( FrameId % Frames.size( ) );
    TotalSecs += DeltaSecs;
}

void ViewerShell::UpdateScene( ) {
    if ( mLoadedScene.pScene ) {
        if (mLoadedScene.pScene->HasAnimStackLayer(kAnimStackId, kAnimLayerId) ) {
            mLoadedScene.pScene->UpdateTransformProperties( TotalSecs, true, kAnimStackId, kAnimLayerId, &SceneTransformFrame );
            mLoadedScene.pScene->UpdateTransformMatrices( SceneTransformFrame );
        }
    }
}

void ViewerShell::UpdateCamera( const apemode::platform::AppInput* pAppInput ) {
    XMFLOAT2 extentF{float( Surface.Swapchain.ImgExtent.width ), float( Surface.Swapchain.ImgExtent.height )};
    pCamInput->Update( DeltaSecs, *pAppInput, extentF );

    if ( !bIsUsingUI ) {
        pCamController->Orbit( pCamInput->OrbitDelta );
        pCamController->Dolly( pCamInput->DollyDelta );
    }

    if ( bLookAnimation )
        pCamController->Orbit( {0.01f * DeltaSecs, 0} );

    pCamController->Update( DeltaSecs );
}

void ViewerShell::Populate( const apemode::SceneNodeTransformFrame* pTransformFrame,
                            ViewerShell::Frame*                     pCurrentFrame,
                            ViewerShell::Frame*                     pSwapchainFrame,
                            VkCommandBuffer                         pCmdBuffer ) {
    float clearColor[ 4 ] = { 0.1f, 0.1f, 0.1f, 1.0f };
    // float clearColor[ 4 ] = { 1.0f, 1.0f, 1.0f, 1.0f };
    // float clearColor[ 4 ] = {0.5f, 0.5f, 1.0f, 1.0f};

    VkClearValue clearValue[ 2 ];
    clearValue[ 0 ].color.float32[ 0 ]   = clearColor[ 0 ];
    clearValue[ 0 ].color.float32[ 1 ]   = clearColor[ 1 ];
    clearValue[ 0 ].color.float32[ 2 ]   = clearColor[ 2 ];
    clearValue[ 0 ].color.float32[ 3 ]   = clearColor[ 3 ];
    clearValue[ 1 ].depthStencil.depth   = 1;
    clearValue[ 1 ].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassBeginInfo;
    InitializeStruct( renderPassBeginInfo );
    renderPassBeginInfo.renderPass               = hDbgRenderPass;
    renderPassBeginInfo.framebuffer              = pSwapchainFrame->hDbgFramebuffer;
    renderPassBeginInfo.renderArea.extent.width  = Surface.Swapchain.ImgExtent.width;
    renderPassBeginInfo.renderArea.extent.height = Surface.Swapchain.ImgExtent.height;
    renderPassBeginInfo.clearValueCount          = 2;
    renderPassBeginInfo.pClearValues             = clearValue;

    vkCmdBeginRenderPass( pCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    const XMFLOAT2 extentF{float( Surface.Swapchain.ImgExtent.width ), float( Surface.Swapchain.ImgExtent.height )};

    auto viewMatrix     = pCamController->ViewMatrix( );
    auto invViewMatrix  = XMMatrixInverse( nullptr, viewMatrix );
    auto projMatrix     = CamProjController.ProjMatrix( 55, extentF.x, extentF.y, 0.1f, 1000.0f );
    auto projBiasMatrix = CamProjController.ProjBiasMatrix( );
    auto invProjMatrix  = XMMatrixInverse( nullptr, projMatrix );

    pSkyboxRenderer->Reset( FrameIndex );
    pDebugRenderer->Reset( FrameIndex );
    pSceneRenderer->Reset( mLoadedScene.pScene.get( ), FrameIndex );

    apemode::vk::SkyboxRenderer::RenderParameters skyboxRenderParams;
    XMStoreFloat4x4( &skyboxRenderParams.InvViewMatrix, invViewMatrix );
    XMStoreFloat4x4( &skyboxRenderParams.InvProjMatrix, invProjMatrix );
    XMStoreFloat4x4( &skyboxRenderParams.ProjBiasMatrix, projBiasMatrix );
    skyboxRenderParams.Dims.x      = extentF.x;
    skyboxRenderParams.Dims.y      = extentF.y;
    skyboxRenderParams.Scale.x     = 1;
    skyboxRenderParams.Scale.y     = 1;
    skyboxRenderParams.FrameIndex  = FrameIndex;
    skyboxRenderParams.pCmdBuffer  = pCmdBuffer;
    skyboxRenderParams.pNode       = &Surface.Node;
    skyboxRenderParams.FieldOfView = apemodexm::DegreesToRadians( 67 );

    // pSkyboxRenderer->Render( pSkybox.get( ), &skyboxRenderParams );

    XMMATRIX rootMatrix = XMMatrixRotationY( WorldRotationY );

    apemode::vk::SceneRenderer::RenderParameters sceneRenderParameters;
    sceneRenderParameters.Dims.x                    = extentF.x;
    sceneRenderParameters.Dims.y                    = extentF.y;
    sceneRenderParameters.Scale.x                   = 1;
    sceneRenderParameters.Scale.y                   = 1;
    sceneRenderParameters.FrameIndex                = FrameIndex;
    sceneRenderParameters.pCmdBuffer                = pCmdBuffer;
    sceneRenderParameters.pNode                     = &Surface.Node;
    sceneRenderParameters.RadianceMap.eImgLayout    = RadianceImg->eImgLayout;
    sceneRenderParameters.RadianceMap.pImgView      = RadianceImg->hImgView;
    sceneRenderParameters.RadianceMap.pSampler      = pRadianceCubeMapSampler;
    sceneRenderParameters.RadianceMap.MipLevels     = RadianceImg->ImgCreateInfo.mipLevels;
    sceneRenderParameters.IrradianceMap.eImgLayout  = IrradianceImg->eImgLayout;
    sceneRenderParameters.IrradianceMap.pImgView    = IrradianceImg->hImgView;
    sceneRenderParameters.IrradianceMap.pSampler    = pIrradianceCubeMapSampler;
    sceneRenderParameters.IrradianceMap.MipLevels   = IrradianceImg->ImgCreateInfo.mipLevels;
    sceneRenderParameters.LightColor                = LightColor;
    sceneRenderParameters.LightDirection            = LightDirection;
    sceneRenderParameters.pTransformFrame           = pTransformFrame;
    XMStoreFloat4x4( &sceneRenderParameters.ProjMatrix, projMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.ViewMatrix, viewMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.InvViewMatrix, invViewMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.InvProjMatrix, invProjMatrix );
    pSceneRenderer->RenderScene( mLoadedScene.pScene.get( ), &sceneRenderParameters );

    {
        apemode::vk::DebugRenderer::DebugUBO frameData;
        XMStoreFloat4x4( &frameData.ProjMatrix, projMatrix );
        XMStoreFloat4x4( &frameData.ViewMatrix, viewMatrix );
        frameData.Color = {1, 0, 0, 1};

        apemode::vk::DebugRenderer::RenderCubeParameters debugRenderCubeParameters;
        debugRenderCubeParameters.Dims[ 0 ]  = extentF.x;
        debugRenderCubeParameters.Dims[ 1 ]  = extentF.y;
        debugRenderCubeParameters.Scale[ 0 ] = 1;
        debugRenderCubeParameters.Scale[ 1 ] = 1;
        debugRenderCubeParameters.FrameIndex = FrameIndex;
        debugRenderCubeParameters.pCmdBuffer = pCmdBuffer;
        debugRenderCubeParameters.pFrameData = &frameData;

        const float scale = 0.5f;
        XMMATRIX    worldMatrix;

        worldMatrix = XMMatrixScaling( scale, scale * 2, scale ) * XMMatrixTranslation( 0, scale * 3, 0 );
        XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
        frameData.Color = {0, 1, 0, 1};
        // pDebugRenderer->Render( &debugRenderCubeParameters );

        worldMatrix = XMMatrixScaling( scale, scale, scale * 2 ) * XMMatrixTranslation( 0, 0, scale * 3 );
        XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
        frameData.Color = {0, 0, 1, 1};
        // pDebugRenderer->Render( &debugRenderCubeParameters );

        worldMatrix = XMMatrixScaling( scale * 2, scale, scale ) * XMMatrixTranslation( scale * 3, 0, 0 );
        XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
        frameData.Color = {1, 0, 0, 1};
        // pDebugRenderer->Render( &debugRenderCubeParameters );

        apemode::vk::DebugRenderer::RenderSceneParameters debugRenderSceneParameters;
        debugRenderSceneParameters.pNode      = &Surface.Node;
        debugRenderSceneParameters.pCmdBuffer = pCmdBuffer;
        debugRenderSceneParameters.FrameIndex = FrameIndex;
        debugRenderSceneParameters.Dims.x     = extentF.x;
        debugRenderSceneParameters.Dims.y     = extentF.y;
        debugRenderSceneParameters.Scale.x    = 1;
        debugRenderSceneParameters.Scale.y    = 1;
        XMStoreFloat4x4( &debugRenderSceneParameters.RootMatrix, rootMatrix );
        XMStoreFloat4x4( &debugRenderSceneParameters.ProjMatrix, projMatrix );
        XMStoreFloat4x4( &debugRenderSceneParameters.ViewMatrix, viewMatrix );
        XMStoreFloat4x4( &debugRenderSceneParameters.InvViewMatrix, invViewMatrix );
        XMStoreFloat4x4( &debugRenderSceneParameters.InvProjMatrix, invProjMatrix );

        debugRenderSceneParameters.pTransformFrame    = &mLoadedScene.pScene->BindPoseFrame;
        debugRenderSceneParameters.SceneColorOverride = XMFLOAT4{1, 1, 0, 1};
        debugRenderSceneParameters.LineWidth          = 4;
        // pDebugRenderer->Render( mLoadedScene.pScene.get( ), &debugRenderSceneParameters );

        debugRenderSceneParameters.pTransformFrame    = pTransformFrame;
        debugRenderSceneParameters.SceneColorOverride = XMFLOAT4{0, 1, 1, 1};
        debugRenderSceneParameters.LineWidth          = 2;
        // pDebugRenderer->Render( mLoadedScene.pScene.get( ), &debugRenderSceneParameters );
    }

    apemode::vk::NuklearRenderer::RenderParameters renderParamsNk;
    renderParamsNk.Dims[ 0 ]            = extentF.x;
    renderParamsNk.Dims[ 1 ]            = extentF.y;
    renderParamsNk.Scale[ 0 ]           = 1;
    renderParamsNk.Scale[ 1 ]           = 1;
    renderParamsNk.eAntiAliasing        = NK_ANTI_ALIASING_ON;
    renderParamsNk.MaxVertexBufferSize  = 64 * 1024;
    renderParamsNk.MaxElementBufferSize = 64 * 1024;
    renderParamsNk.FrameIndex           = FrameIndex;
    renderParamsNk.pCmdBuffer           = pCmdBuffer;

    // pNkRenderer->Render( &renderParamsNk );
    nk_clear( &pNkRenderer->Context );
    // apemodevk::platform::LogFmt(apemodevk::platform::Info, "nk_clear");

    vkCmdEndRenderPass( pCmdBuffer );

    pDebugRenderer->Flush( FrameIndex );
    pSceneRenderer->Flush( mLoadedScene.pScene.get( ), FrameIndex );
    pSkyboxRenderer->Flush( FrameIndex );
}

bool ViewerShell::Update( const VkExtent2D currentExtent, const apemode::platform::AppInput* pAppInput ) {
    apemode_memory_allocation_scope;

    if ( pAppInput->bIsQuitRequested || pAppInput->IsFirstPressed( apemode::platform::kDigitalInput_BackButton ) ||
         pAppInput->IsFirstPressed( apemode::platform::kDigitalInput_KeyEscape ) ) {
        Surface.Node.Await( );
        return false;
    }

    if ( currentExtent.width && currentExtent.height && Surface.Resize( currentExtent ) ) {
        OnResized( );
    }

    UpdateTime( );
    UpdateUI( currentExtent, pAppInput );
    UpdateCamera( pAppInput );
    UpdateScene( );

    Frame& currentFrame = Frames[ FrameIndex ];

    CheckedResult( vkAcquireNextImageKHR( Surface.Node.hLogicalDevice,
                                          Surface.Swapchain.hSwapchain,
                                          UINT64_MAX,
                                          currentFrame.hPresentCompleteSemaphore,
                                          VK_NULL_HANDLE,
                                          &currentFrame.BackbufferIndex ) );

    Frame& swapchainFrame = Frames[ currentFrame.BackbufferIndex ];

    const SceneNodeTransformFrame* pTrasformFrame =
        bEnableAnimations && mLoadedScene.pScene->HasAnimStackLayer( kAnimStackId, kAnimLayerId ) ? &SceneTransformFrame  : 0;

    const uint32_t       queueFamilyId               = 0;
    VkPipelineStageFlags eColorAttachmentOutputStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    apemodevk::OneTimeCmdBufferSubmitResult submitResult =
        apemodevk::TOneTimeCmdBufferSubmit( &Surface.Node,
                                            queueFamilyId,
                                            false,
                                            [&]( VkCommandBuffer pCmdBuffer ) {
                                                Populate( pTrasformFrame, &currentFrame, &swapchainFrame, pCmdBuffer );
                                                return true;
                                            },
                                            apemodevk::kDefaultQueueAwaitTimeoutNanos,
                                            apemodevk::kDefaultQueueAwaitTimeoutNanos,
                                            currentFrame.hRenderCompleteSemaphore.GetAddressOf( ),
                                            1,
                                            &eColorAttachmentOutputStage,
                                            currentFrame.hPresentCompleteSemaphore.GetAddressOf( ),
                                            1 );

    if ( submitResult.eResult != VK_SUCCESS ) {
        return false;
    }

    const uint32_t queueId = submitResult.QueueId;

    /*
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "Submitted to Queue: %u %u", queueId, queueFamilyId);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Backbuffer index: %u", currentFrame.BackbufferIndex);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Signal semaphore: %p",
    currentFrame.hRenderCompleteSemaphore.Handle); apemodevk::platform::LogFmt(apemodevk::platform::Info, ">   Wait semaphore:
    %p", currentFrame.hPresentCompleteSemaphore.Handle); apemodevk::platform::LogFmt(apemodevk::platform::Info,
    "------------------------------");
    */

    if ( FrameId ) {
        // TODO: Acquire correctly.
        apemodevk::QueueInPool& currentQueue = Surface.Node.GetQueuePool( )->Pools[ queueFamilyId ].Queues[ queueId ];
        currentQueue.bInUse                  = true;

        const uint32_t frameCount = uint32_t( Frames.size( ) );

        uint32_t presentIndex    = ( FrameIndex + frameCount - 1 ) % frameCount;
        Frame&   presentingFrame = Frames[ presentIndex ];

        VkPresentInfoKHR presentInfoKHR;
        InitializeStruct( presentInfoKHR );
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores    = presentingFrame.hRenderCompleteSemaphore.GetAddressOf( );
        presentInfoKHR.swapchainCount     = 1;
        presentInfoKHR.pSwapchains        = Surface.Swapchain.hSwapchain.GetAddressOf( );
        presentInfoKHR.pImageIndices      = &presentingFrame.BackbufferIndex;

        CheckedResult( vkQueuePresentKHR( currentQueue.hQueue, &presentInfoKHR ) );

        /*
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "Present to Queue: %u %u", queueId, queueFamilyId);
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Backbuffer index: %u", presentingFrame.BackbufferIndex);
        apemodevk::platform::LogFmt(apemodevk::platform::Info, ">   Wait semaphore: %p",
        presentingFrame.hRenderCompleteSemaphore.Handle); apemodevk::platform::LogFmt(apemodevk::platform::Info,
        "------------------------------");
        */

        currentQueue.bInUse = false;
    }

    ++FrameId;
    return true;
}
