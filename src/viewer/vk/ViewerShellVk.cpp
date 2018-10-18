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
    }
}

// clang-format off
struct apemode::viewer::vk::GraphicsAllocator : apemodevk::GraphicsManager::IAllocator {
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

struct apemode::viewer::vk::GraphicsLogger : apemodevk::GraphicsManager::ILogger {
    spdlog::logger* pLogger = apemode::AppState::Get( )->GetLogger( );
    void Log( const LogLevel eLevel, const char* pszMsg ) override {
        pLogger->log( static_cast< spdlog::level::level_enum >( eLevel ), pszMsg );
    }
};

// clang-format on

const VkFormat sDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
//const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

ViewerShell::ViewerShell( ) : mAssetManager() {
    apemode_memory_allocation_scope;
    pCamInput      = apemode::unique_ptr< CameraControllerInputBase >( apemode_new MouseKeyboardCameraControllerInput( ) );
    pCamController = apemode::unique_ptr< CameraControllerBase >( apemode_new FreeLookCameraController( ) );
    // pCamController = apemode::unique_ptr< CameraControllerBase >( apemode_new ModelViewCameraController( ) );
}

ViewerShell::~ViewerShell( ) {
    LogInfo( "ViewerApp: Destroyed." );
}

bool ViewerShell::Initialize( const apemode::PlatformSurface* pPlatformSurface  ) {
    apemode_memory_allocation_scope;
    LogInfo( "ViewerApp: Initializing." );

    Logger    = apemode::make_unique< GraphicsLogger >( );
    Allocator = apemode::make_unique< GraphicsAllocator >( );

    apemode::vector< const char* > ppszLayers;
    apemode::vector< const char* > ppszExtensions;
    size_t                   optionalLayerCount = 0;
    size_t                   optionalExtentionCount = 0;

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
        apemode::string8 assetsFolder( TGetOption< std::string >( "--assets", "./" ).c_str( ) );
        mAssetManager.UpdateAssets( assetsFolder.c_str( ), nullptr, 0 );

        TotalSecs = 0.0f;

        FrameId    = 0;
        FrameIndex = Surface.Swapchain.GetBufferCount( );
        Frames.resize(Surface.Swapchain.GetBufferCount( ));

        OnResized( );

        for ( uint32_t i = 0; i < Frames.size(); ++i ) {

            VkCommandPoolCreateInfo cmdPoolCreateInfo;
            InitializeStruct( cmdPoolCreateInfo );
            cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolCreateInfo.queueFamilyIndex = Surface.PresentQueueFamilyIds[0];

            if ( false == Frames[i].hCmdPool.Recreate( Surface.Node, cmdPoolCreateInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            VkCommandBufferAllocateInfo cmdBufferAllocInfo;
            InitializeStruct( cmdBufferAllocInfo );
            cmdBufferAllocInfo.commandPool = Frames[ i ].hCmdPool;
            cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufferAllocInfo.commandBufferCount = 1;

            if ( false == Frames[ i ].hCmdBuffer.Recreate( Surface.Node, cmdBufferAllocInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            VkSemaphoreCreateInfo semaphoreCreateInfo;
            InitializeStruct( semaphoreCreateInfo );
            if ( false == Frames[ i ].hPresentCompleteSemaphore.Recreate( Surface.Node, semaphoreCreateInfo ) ||
                 false == Frames[ i ].hRenderCompleteSemaphore.Recreate( Surface.Node, semaphoreCreateInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }
        }

        apemodevk::DescriptorPool::InitializeParameters descPoolInitParameters;
        descPoolInitParameters.pNode                                                               = &Surface.Node;
        descPoolInitParameters.MaxDescriptorSetCount                                               = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_SAMPLER ]                = 64;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ] = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ] = 512;

        if ( false == DescriptorPool.Initialize( descPoolInitParameters ) ) {
            return false;
        }

        pSamplerManager = apemode::make_unique< apemodevk::SamplerManager >( );
        if ( false == pSamplerManager->Recreate( &Surface.Node ) ) {
            return false;
        }

        apemodevk::ImageUploader imgUploader;
        apemodevk::ImageDecoder  imgDecoder;

        // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/kyoto_lod.dds" ) ) {
        // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/output_skybox.dds" ) ) {
        if ( auto pTexAsset = mAssetManager.Acquire( "images/Environment/bolonga_lod.dds" ) ) {
            apemode_memory_allocation_scope;
            {
                const auto texAssetBin = pTexAsset->GetContentAsBinaryBuffer( );
                mAssetManager.Release( pTexAsset );

                apemodevk::ImageDecoder::DecodeOptions decodeOptions;
                decodeOptions.eFileFormat = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_DDS;
                decodeOptions.bGenerateMipMaps = true;

                apemodevk::ImageUploader::UploadOptions loadOptions;
                loadOptions.bImgView = true;

                auto srcImg = imgDecoder.DecodeSourceImageFromData( texAssetBin.data(), texAssetBin.size(), decodeOptions );
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

        // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/kyoto_irr.dds" ) ) {
        // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/output_iem.dds" ) ) {
        if ( auto pTexAsset = mAssetManager.Acquire( "images/Environment/bolonga_irr.dds" ) ) {
            apemode_memory_allocation_scope;
            {
                const auto texAssetBin = pTexAsset->GetContentAsBinaryBuffer( );
                mAssetManager.Release( pTexAsset );

                apemodevk::ImageDecoder::DecodeOptions decodeOptions;
                decodeOptions.eFileFormat      = apemodevk::ImageDecoder::DecodeOptions::eImageFileFormat_DDS;
                decodeOptions.bGenerateMipMaps = false;

                apemodevk::ImageUploader::UploadOptions loadOptions;
                loadOptions.bImgView = true;

                auto srcImg = imgDecoder.DecodeSourceImageFromData( texAssetBin.data(), texAssetBin.size(), decodeOptions );
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

        pNkRenderer = apemode::unique_ptr< NuklearRendererBase >( apemode_new apemode::vk::NuklearRenderer( ) );

        auto pFontAsset = mAssetManager.Acquire( "fonts/iosevka-ss07-medium.ttf" );

        apemode::vk::NuklearRenderer::InitParameters initParamsNk;
        initParamsNk.pNode           = &Surface.Node;
        initParamsNk.pAssetManager   = &mAssetManager;
        initParamsNk.pFontAsset      = pFontAsset;
        initParamsNk.pSamplerManager = pSamplerManager.get( );
        initParamsNk.pDescPool       = DescriptorPool;
        initParamsNk.pRenderPass     = hDbgRenderPass;
        // initParamsNk.pRenderPass     = hNkRenderPass;

        pNkRenderer->Init( &initParamsNk );
        mAssetManager.Release( pFontAsset );

        apemode::vk::DebugRenderer::InitParameters initParamsDbg;
        initParamsDbg.pNode           = &Surface.Node;
        initParamsDbg.pAssetManager   = &mAssetManager;
        initParamsDbg.pRenderPass     = hDbgRenderPass;
        initParamsDbg.pDescPool       = DescriptorPool;
        initParamsDbg.FrameCount      = Frames.size();

        pDebugRenderer = apemode::unique_ptr< apemode::vk::DebugRenderer >( apemode_new apemode::vk::DebugRenderer() );
        pDebugRenderer->RecreateResources( &initParamsDbg );

        pSceneRendererBase = apemode::unique_ptr< apemode::SceneRendererBase >( apemode_new apemode::vk::SceneRenderer( ) );

        apemode::vk::SceneRenderer::RecreateParameters recreateParams;
        recreateParams.pNode           = &Surface.Node;
        recreateParams.pAssetManager   = &mAssetManager;
        recreateParams.pRenderPass     = hDbgRenderPass;
        recreateParams.pDescPool       = DescriptorPool;
        recreateParams.FrameCount      = Frames.size();

        /*
        --assets "/Users/vlad.serhiienko/Projects/Home/Viewer/assets" --scene "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp"
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

        if ( false == pSceneRendererBase->Recreate( &recreateParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        auto sceneFile = TGetOption< std::string >( "scene", "" );
        mLoadedScene = LoadSceneFromBin( apemode::platform::shared::FileReader( ).ReadBinFile( sceneFile.c_str( ) ) );

        apemode::vk::SceneUploader::UploadParameters uploadParams;
        uploadParams.pSamplerManager = pSamplerManager.get( );
        uploadParams.pSrcScene       = mLoadedScene.pSrcScene;
        uploadParams.pImgUploader    = &imgUploader;
        uploadParams.pNode           = &Surface.Node;

        apemode::vk::SceneUploader sceneUploader;
        if ( false == sceneUploader.UploadScene( mLoadedScene.pScene.get( ), &uploadParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        apemode::vk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode           = &Surface.Node;
        skyboxRendererRecreateParams.pAssetManager   = &mAssetManager;
        skyboxRendererRecreateParams.pRenderPass     = hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool       = DescriptorPool;
        skyboxRendererRecreateParams.FrameCount      = Frames.size();

        pSkyboxRenderer = apemode::make_unique< apemode::vk::SkyboxRenderer >( );
        if ( false == pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
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

        Stopwatch.Start();
        return true;
    }

    return false;
}

bool apemode::viewer::vk::ViewerShell::OnResized( ) {
    apemode_memory_allocation_scope;

    Width  = Surface.Swapchain.ImgExtent.width;
    Height = Surface.Swapchain.ImgExtent.height;

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
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocationCreateInfo.flags = 0;

    for ( uint32_t i = 0; i < Frames.size(); ++i ) {
        if ( false == Frames[ i ].hDepthImg.Recreate( Surface.Node.hAllocator, depthImgCreateInfo, allocationCreateInfo ) ) {
            return false;
        }
    }

    for ( uint32_t i = 0; i < Frames.size(); ++i ) {
        depthImgViewCreateInfo.image = Frames[ i ].hDepthImg;
        if ( false == Frames[ i ].hDepthImgView.Recreate( Surface.Node, depthImgViewCreateInfo ) ) {
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

    if ( false == hNkRenderPass.Recreate( Surface.Node, renderPassCreateInfoNk ) ) {
        apemode::platform::DebugBreak( );
        return false;
    }

    if ( false == hDbgRenderPass.Recreate( Surface.Node, renderPassCreateInfoDbg ) ) {
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

    for ( uint32_t i = 0; i < Frames.size(); ++i ) {
        VkImageView attachments[ 1 ] = {Surface.Swapchain.Buffers[ i ].hImgView};
        framebufferCreateInfoNk.pAttachments = attachments;

        if ( false == Frames[ i ].hNkFramebuffer.Recreate( Surface.Node, framebufferCreateInfoNk ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }
    }

    for ( uint32_t i = 0; i < Frames.size(); ++i ) {
        VkImageView attachments[ 2 ] = {Surface.Swapchain.Buffers[ i ].hImgView, Frames[ i ].hDepthImgView};
        framebufferCreateInfoDbg.pAttachments = attachments;

        if ( false == Frames[ i ].hDbgFramebuffer.Recreate( Surface.Node, framebufferCreateInfoDbg ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }
    }

    return true;
}

void ViewerShell::UpdateUI(const VkExtent2D currentExtent, const apemode::platform::AppInput* pAppInput) {
    auto pNkContext = &pNkRenderer->Context;
    
    pNkRenderer->HandleInput(pAppInput);
    if ( nk_begin( pNkContext, "Viewer", nk_rect( 10, 10, 180, 500 ), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE ) ) {

        auto invViewMatrix = GetMatrix( XMMatrixInverse( nullptr, pCamController->ViewMatrix( ) ) );

        nk_layout_row_dynamic(pNkContext, 10, 1);
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
    }
    nk_end(pNkContext);

}

void ViewerShell::IncFrame() {
    DeltaSecs = (float) Stopwatch.GetElapsedSeconds( );
    Stopwatch.Start( );

    FrameIndex = FrameId % (uint64_t) Frames.size();
    TotalSecs += DeltaSecs;
}

void ViewerShell::UpdateCamera(const apemode::platform::AppInput *pAppInput) {
    pCamInput->Update( DeltaSecs, *pAppInput, {float( Width ), float( Height )} );
    pCamController->Orbit( pCamInput->OrbitDelta );
    pCamController->Dolly( pCamInput->DollyDelta );
    pCamController->Update( DeltaSecs );
}

void ViewerShell::Populate(Frame * pCurrentFrame, Frame * pSwapchainFrame, VkCommandBuffer pCmdBuffer) {

    float clearColor[ 4 ] = {0.5f, 0.5f, 1.0f, 1.0f};

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

    XMFLOAT2 extentF{float( Surface.Swapchain.ImgExtent.width ), float( Surface.Swapchain.ImgExtent.height )};

    auto viewMatrix     = pCamController->ViewMatrix( );
    auto invViewMatrix  = XMMatrixInverse( nullptr, viewMatrix );
    auto projMatrix     = CamProjController.ProjMatrix( 55, extentF.x, extentF.y, 0.1f, 1000.0f );
    auto projBiasMatrix = CamProjController.ProjBiasMatrix( );
    auto invProjMatrix  = XMMatrixInverse( nullptr, projMatrix );

    apemode::vk::DebugRenderer::SkyboxUBO frameData;
    XMStoreFloat4x4( &frameData.ProjMatrix, projMatrix );
    XMStoreFloat4x4( &frameData.ViewMatrix, viewMatrix );
    frameData.Color = {1, 0, 0, 1};

    pSkyboxRenderer->Reset( FrameIndex );
    pDebugRenderer->Reset( FrameIndex );
    pSceneRendererBase->Reset( mLoadedScene.pScene.get( ), FrameIndex );

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

    pSkyboxRenderer->Render( pSkybox.get( ), &skyboxRenderParams );

    apemode::vk::DebugRenderer::RenderParameters renderParamsDbg;
    renderParamsDbg.Dims[ 0 ]  = extentF.x;
    renderParamsDbg.Dims[ 1 ]  = extentF.y;
    renderParamsDbg.Scale[ 0 ] = 1;
    renderParamsDbg.Scale[ 1 ] = 1;
    renderParamsDbg.FrameIndex = FrameIndex;
    renderParamsDbg.pCmdBuffer = pCmdBuffer;
    renderParamsDbg.pFrameData = &frameData;

    const float scale = 0.5f;

    XMMATRIX worldMatrix;

    worldMatrix = XMMatrixScaling( scale, scale * 2, scale ) * XMMatrixTranslation( 0, scale * 3, 0 );
    XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
    frameData.Color = {0, 1, 0, 1};
    // pDebugRenderer->Render( &renderParamsDbg );

    worldMatrix = XMMatrixScaling( scale, scale, scale * 2 ) * XMMatrixTranslation( 0, 0, scale * 3 );
    XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
    frameData.Color = {0, 0, 1, 1};
    // pDebugRenderer->Render( &renderParamsDbg );


    worldMatrix = XMMatrixScaling( scale * 2, scale, scale ) * XMMatrixTranslation( scale * 3, 0, 0 );
    XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
    frameData.Color = { 1, 0, 0, 1 };
    // pDebugRenderer->Render( &renderParamsDbg );

    XMMATRIX rootMatrix = XMMatrixRotationY( WorldRotationY );

    apemode::vk::SceneRenderer::RenderParameters sceneRenderParameters;
    sceneRenderParameters.Dims.x                   = extentF.x;
    sceneRenderParameters.Dims.y                   = extentF.y;
    sceneRenderParameters.Scale.x                  = 1;
    sceneRenderParameters.Scale.y                  = 1;
    sceneRenderParameters.FrameIndex               = FrameIndex;
    sceneRenderParameters.pCmdBuffer               = pCmdBuffer;
    sceneRenderParameters.pNode                    = &Surface.Node;
    sceneRenderParameters.RadianceMap.eImgLayout   = RadianceImg->eImgLayout;
    sceneRenderParameters.RadianceMap.pImgView     = RadianceImg->hImgView;
    sceneRenderParameters.RadianceMap.pSampler     = pRadianceCubeMapSampler;
    sceneRenderParameters.RadianceMap.MipLevels    = RadianceImg->ImgCreateInfo.mipLevels;
    sceneRenderParameters.IrradianceMap.eImgLayout = IrradianceImg->eImgLayout;
    sceneRenderParameters.IrradianceMap.pImgView   = IrradianceImg->hImgView;
    sceneRenderParameters.IrradianceMap.pSampler   = pIrradianceCubeMapSampler;
    sceneRenderParameters.IrradianceMap.MipLevels  = IrradianceImg->ImgCreateInfo.mipLevels;
    sceneRenderParameters.LightColor               = LightColor;
    sceneRenderParameters.LightDirection           = LightDirection;
    XMStoreFloat4x4( &sceneRenderParameters.ProjMatrix, projMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.ViewMatrix, viewMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.InvViewMatrix, invViewMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.InvProjMatrix, invProjMatrix );
    XMStoreFloat4x4( &sceneRenderParameters.RootMatrix, rootMatrix );
//    pSceneRendererBase->RenderScene( mLoadedScene.pScene.get( ), &sceneRenderParameters );

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

    pNkRenderer->Render( &renderParamsNk );
    nk_clear( &pNkRenderer->Context );

    vkCmdEndRenderPass( pCmdBuffer );

    pDebugRenderer->Flush( FrameIndex );
    pSceneRendererBase->Flush( mLoadedScene.pScene.get( ), FrameIndex );
    pSkyboxRenderer->Flush( FrameIndex );

}

bool ViewerShell::Update( const VkExtent2D currentExtent, const apemode::platform::AppInput* pAppInput ) {
    apemode_memory_allocation_scope;

    if ( pAppInput->bIsQuitRequested ||
         pAppInput->IsFirstPressed( apemode::platform::kDigitalInput_BackButton ) ||
         pAppInput->IsFirstPressed( apemode::platform::kDigitalInput_KeyEscape ) ) {
        return false;
    }

    if ( currentExtent.width && currentExtent.height && Surface.Resize( currentExtent ) ) {
        OnResized( );
    }

    IncFrame( );
    // UpdateUI( currentExtent, pAppInput );
    UpdateCamera( pAppInput );

    Frame & currentFrame = Frames[ FrameIndex ];

    CheckedResult( vkAcquireNextImageKHR( Surface.Node.hLogicalDevice,
                                          Surface.Swapchain.hSwapchain,
                                          UINT64_MAX,
                                          currentFrame.hPresentCompleteSemaphore,
                                          VK_NULL_HANDLE,
                                          &currentFrame.BackbufferIndex ) );
    
    
    
    Frame & swapchainFrame = Frames[ currentFrame.BackbufferIndex ];
    
    const uint32_t queueFamilyId = 0;
    VkPipelineStageFlags eColorAttachmentOutputStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    apemodevk::OneTimeCmdBufferSubmitResult submitResult =
        apemodevk::TOneTimeCmdBufferSubmit( &Surface.Node,
                                            queueFamilyId,
                                            false,
                                            [&]( VkCommandBuffer pCmdBuffer ) {
                                                Populate( &currentFrame, &swapchainFrame, pCmdBuffer );
                                                return true;
                                            },
                                            apemodevk::kDefaultQueueAwaitTimeoutNanos,
                                            apemodevk::kDefaultQueueAwaitTimeoutNanos,
                                            currentFrame.hRenderCompleteSemaphore.GetAddressOf( ),
                                            1,
                                            &eColorAttachmentOutputStage,
                                            currentFrame.hPresentCompleteSemaphore.GetAddressOf( ),
                                            1 );

    if (submitResult.eResult != VK_SUCCESS) {
        return false;
    }
    
    
    const uint32_t queueId = submitResult.QueueId;
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "Submitted to Queue: %u %u", queueId, queueFamilyId);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Backbuffer index: %u", currentFrame.BackbufferIndex);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Signal semaphore: %p", currentFrame.hRenderCompleteSemaphore.Handle);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, ">   Wait semaphore: %p", currentFrame.hPresentCompleteSemaphore.Handle);
    apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");

    if ( FrameId ) {
        // TODO: Acquire correctly.
        apemodevk::QueueInPool& currentQueue = Surface.Node.GetQueuePool( )->Pools[ queueFamilyId ].Queues[ queueId ];
        currentQueue.bInUse                  = true;

        uint32_t presentIndex                = ( FrameIndex + Frames.size( ) - 1 ) % Frames.size( );
        Frame&   presentingFrame             = Frames[ presentIndex ];

        VkPresentInfoKHR presentInfoKHR;
        InitializeStruct( presentInfoKHR );
        presentInfoKHR.waitSemaphoreCount = 1;
        presentInfoKHR.pWaitSemaphores    = presentingFrame.hRenderCompleteSemaphore.GetAddressOf( );
        presentInfoKHR.swapchainCount     = 1;
        presentInfoKHR.pSwapchains        = Surface.Swapchain.hSwapchain.GetAddressOf( );
        presentInfoKHR.pImageIndices      = &presentingFrame.BackbufferIndex;

        CheckedResult( vkQueuePresentKHR( currentQueue.hQueue, &presentInfoKHR ) );
        
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "Present to Queue: %u %u", queueId, queueFamilyId);
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "> Backbuffer index: %u", presentingFrame.BackbufferIndex);
        apemodevk::platform::LogFmt(apemodevk::platform::Info, ">   Wait semaphore: %p", presentingFrame.hRenderCompleteSemaphore.Handle);
        apemodevk::platform::LogFmt(apemodevk::platform::Info, "------------------------------");
        currentQueue.bInUse = false;
    }

    ++FrameId;
    return true;
}

