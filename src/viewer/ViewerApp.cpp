#include <ViewerApp.h>
#include <MemoryManager.h>

using namespace apemode;

namespace apemode {
    using namespace apemodexm;
    using namespace apemodevk;

    namespace platform {
        static void DebugBreak( ) {
            apemodevk::platform::DebugBreak( );
        }
    }
}

const VkFormat sDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
//const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

ViewerApp::ViewerApp( ) {
    apemode_memory_allocation_scope;
    pCamInput      = apemode::unique_ptr< CameraControllerInputBase >( apemode_new MouseKeyboardCameraControllerInput( ) );
    pCamController = apemode::unique_ptr< CameraControllerBase >( apemode_new FreeLookCameraController( ) );
    // pCamController = apemode::make_unique< ModelViewCameraController >( );
}

ViewerApp::~ViewerApp( ) {
    LogInfo( "ViewerApp: Destroyed." );
}

apemode::unique_ptr< apemode::AppSurfaceBase > ViewerApp::CreateAppSurface( ) {
    LogInfo( "ViewerApp: Creating surface." );
    return apemode::unique_ptr< apemode::AppSurfaceBase >( apemode_new AppSurfaceSdlVk( ) );
}

bool ViewerApp::Initialize(  ) {
    apemode_memory_allocation_scope;
    LogInfo( "ViewerApp: Initializing." );

    if ( AppBase::Initialize( ) ) {
        apemode::string8 assetsFolder( TGetOption< std::string >( "--assets", "./" ).c_str( ) );
        // apemode::string8 interestingFilePattern = ".*\\.(vert|frag|comp|geom|tesc|tese|h|inl|inc|fx)$";
        mAssetManager.UpdateAssets( assetsFolder.c_str( ), nullptr, 0 );

        TotalSecs = 0.0f;

        auto appSurfaceBase = GetSurface( );
        if ( appSurfaceBase->GetImpl( ) != kAppSurfaceImpl_SdlVk )
            return false;

        auto pAppSurface = (AppSurfaceSdlVk*) appSurfaceBase;

        FrameId    = 0;
        FrameIndex = 0;
        FrameCount = pAppSurface->Swapchain.GetBufferCount( );

        OnResized();

        for ( uint32_t i = 0; i < FrameCount; ++i ) {

            VkCommandPoolCreateInfo cmdPoolCreateInfo;
            InitializeStruct( cmdPoolCreateInfo );
            cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolCreateInfo.queueFamilyIndex = pAppSurface->PresentQueueFamilyIds[0];

            if ( false == hCmdPool[ i ].Recreate( pAppSurface->Node, cmdPoolCreateInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            VkCommandBufferAllocateInfo cmdBufferAllocInfo;
            InitializeStruct( cmdBufferAllocInfo );
            cmdBufferAllocInfo.commandPool = hCmdPool[ i ];
            cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufferAllocInfo.commandBufferCount = 1;

            if ( false == hCmdBuffers[ i ].Recreate( pAppSurface->Node, cmdBufferAllocInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            VkSemaphoreCreateInfo semaphoreCreateInfo;
            InitializeStruct( semaphoreCreateInfo );
            if ( false == hPresentCompleteSemaphores[ i ].Recreate( pAppSurface->Node, semaphoreCreateInfo ) ||
                 false == hRenderCompleteSemaphores[ i ].Recreate( pAppSurface->Node, semaphoreCreateInfo ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }
        }

        apemodevk::DescriptorPool::InitializeParameters descPoolInitParameters;
        descPoolInitParameters.pNode                                                               = &pAppSurface->Node;
        descPoolInitParameters.MaxDescriptorSetCount                                               = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_SAMPLER ]                = 64;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ] = 1024;
        descPoolInitParameters.MaxDescriptorPoolSizes[ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ] = 512;

        if ( false == DescriptorPool.Initialize( descPoolInitParameters ) ) {
            return false;
        }

        pSamplerManager = apemode::make_unique< apemodevk::SamplerManager >( );
        if ( false == pSamplerManager->Recreate( &pAppSurface->Node ) ) {
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
                RadianceImg = imgUploader.UploadImage( &pAppSurface->Node, *srcImg, loadOptions );
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
                IrradianceImg = imgUploader.UploadImage( &pAppSurface->Node, *srcImg, loadOptions );
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

        pNkRenderer = apemode::unique_ptr< NuklearRendererSdlBase >( apemode_new vk::NuklearRenderer( ) );

        auto pFontAsset = mAssetManager.Acquire( "fonts/iosevka-ss07-medium.ttf" );

        vk::NuklearRenderer::InitParameters initParamsNk;
        initParamsNk.pNode           = &pAppSurface->Node;
        initParamsNk.pAssetManager   = &mAssetManager;
        initParamsNk.pFontAsset      = pFontAsset;
        initParamsNk.pSamplerManager = pSamplerManager.get( );
        initParamsNk.pDescPool       = DescriptorPool;
        initParamsNk.pRenderPass     = hDbgRenderPass;
        // initParamsNk.pRenderPass     = hNkRenderPass;

        pNkRenderer->Init( &initParamsNk );
        mAssetManager.Release( pFontAsset );

        vk::DebugRenderer::InitParameters initParamsDbg;
        initParamsDbg.pNode           = &pAppSurface->Node;
        initParamsDbg.pAssetManager   = &mAssetManager;
        initParamsDbg.pRenderPass     = hDbgRenderPass;
        initParamsDbg.pDescPool       = DescriptorPool;
        initParamsDbg.FrameCount      = FrameCount;

        pDebugRenderer = apemode::unique_ptr< vk::DebugRenderer >( apemode_new vk::DebugRenderer() );
        pDebugRenderer->RecreateResources( &initParamsDbg );

        pSceneRendererBase = apemode::unique_ptr< apemode::SceneRendererBase >( pAppSurface->CreateSceneRenderer( ) );

        vk::SceneRenderer::RecreateParameters recreateParams;
        recreateParams.pNode           = &pAppSurface->Node;
        recreateParams.pAssetManager   = &mAssetManager;
        recreateParams.pRenderPass     = hDbgRenderPass;
        recreateParams.pDescPool       = DescriptorPool;
        recreateParams.FrameCount      = FrameCount;

        /*
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
        mLoadedScene = LoadSceneFromBin( apemodeos::FileReader( ).ReadBinFile( sceneFile.c_str( ) ) );

        vk::SceneUploader::UploadParameters uploadParams;
        uploadParams.pSamplerManager = pSamplerManager.get( );
        uploadParams.pSrcScene       = mLoadedScene.pSrcScene;
        uploadParams.pImgUploader    = &imgUploader;
        uploadParams.pNode           = &pAppSurface->Node;

        vk::SceneUploader sceneUploader;
        if ( false == sceneUploader.UploadScene( mLoadedScene.pScene.get( ), &uploadParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        vk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode           = &pAppSurface->Node;
        skyboxRendererRecreateParams.pAssetManager   = &mAssetManager;
        skyboxRendererRecreateParams.pRenderPass     = hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool       = DescriptorPool;
        skyboxRendererRecreateParams.FrameCount      = FrameCount;

        pSkyboxRenderer = apemode::make_unique< vk::SkyboxRenderer >( );
        if ( false == pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
            apemode::platform::DebugBreak( );
            return false;
        }

        pSkybox                = apemode::make_unique< vk::Skybox >( );
        pSkybox->pSampler      = pRadianceCubeMapSampler;
        pSkybox->pImgView      = RadianceImg->hImgView;
        pSkybox->Dimension     = RadianceImg->ImgCreateInfo.extent.width;
        pSkybox->eImgLayout    = RadianceImg->eImgLayout;
        pSkybox->Exposure      = 3;
        pSkybox->LevelOfDetail = 0;

        LightDirection = XMFLOAT4( 0, 1, 0, 1 );
        LightColor     = XMFLOAT4( 1, 1, 1, 1 );

        return true;
    }

    return false;
}

bool apemode::ViewerApp::OnResized( ) {
    apemode_memory_allocation_scope;

    if ( auto appSurface = static_cast< AppSurfaceSdlVk* >( GetSurface( ) ) ) {

            Width  = appSurface->GetWidth( );
            Height = appSurface->GetHeight( );

            VkImageCreateInfo depthImgCreateInfo;
            InitializeStruct( depthImgCreateInfo );
            depthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
            depthImgCreateInfo.format        = sDepthFormat;
            depthImgCreateInfo.extent.width  = appSurface->Swapchain.ImgExtent.width;
            depthImgCreateInfo.extent.height = appSurface->Swapchain.ImgExtent.height;
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

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                if ( false == hDepthImgs[ i ].Recreate( appSurface->Node.hAllocator, depthImgCreateInfo, allocationCreateInfo ) ) {
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                depthImgViewCreateInfo.image = hDepthImgs[ i ];
                if ( false == hDepthImgViews[ i ].Recreate( appSurface->Node, depthImgViewCreateInfo ) ) {
                    return false;
                }
            }

            VkAttachmentDescription colorDepthAttachments[ 2 ];
            InitializeStruct( colorDepthAttachments );

            colorDepthAttachments[ 0 ].format         = appSurface->Surface.eColorFormat;
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

            if ( false == hNkRenderPass.Recreate( appSurface->Node, renderPassCreateInfoNk ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            if ( false == hDbgRenderPass.Recreate( appSurface->Node, renderPassCreateInfoDbg ) ) {
                apemode::platform::DebugBreak( );
                return false;
            }

            VkFramebufferCreateInfo framebufferCreateInfoNk;
            InitializeStruct( framebufferCreateInfoNk );
            framebufferCreateInfoNk.renderPass      = hNkRenderPass;
            framebufferCreateInfoNk.attachmentCount = 1;
            framebufferCreateInfoNk.width           = appSurface->Swapchain.ImgExtent.width;
            framebufferCreateInfoNk.height          = appSurface->Swapchain.ImgExtent.height;
            framebufferCreateInfoNk.layers          = 1;

            VkFramebufferCreateInfo framebufferCreateInfoDbg;
            InitializeStruct( framebufferCreateInfoDbg );
            framebufferCreateInfoDbg.renderPass      = hDbgRenderPass;
            framebufferCreateInfoDbg.attachmentCount = 2;
            framebufferCreateInfoDbg.width           = appSurface->Swapchain.ImgExtent.width;
            framebufferCreateInfoDbg.height          = appSurface->Swapchain.ImgExtent.height;
            framebufferCreateInfoDbg.layers          = 1;

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                VkImageView attachments[ 1 ] = {appSurface->Swapchain.Buffers[ i ].hImgView};
                framebufferCreateInfoNk.pAttachments = attachments;

                if ( false == hNkFramebuffers[ i ].Recreate( appSurface->Node, framebufferCreateInfoNk ) ) {
                    apemode::platform::DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                VkImageView attachments[ 2 ] = {appSurface->Swapchain.Buffers[ i ].hImgView, hDepthImgViews[ i ]};
                framebufferCreateInfoDbg.pAttachments = attachments;

                if ( false == hDbgFramebuffers[ i ].Recreate( appSurface->Node, framebufferCreateInfoDbg ) ) {
                    apemode::platform::DebugBreak( );
                    return false;
                }
            }
    }

    return true;
}

void ViewerApp::OnFrameMove( ) {
    apemode_memory_allocation_scope;
    nk_input_begin( &pNkRenderer->Context ); {
        SDL_Event evt;
        while ( SDL_PollEvent( &evt ) )
            pNkRenderer->HandleEvent( &evt );
        nk_input_end( &pNkRenderer->Context );
    }

    AppBase::OnFrameMove( );

    ++FrameId;
    FrameIndex = FrameId % (uint64_t) FrameCount;
}

void ViewerApp::Update( float deltaSecs, Input const& inputState ) {
    apemode_memory_allocation_scope;
    TotalSecs += deltaSecs;

    auto ctx = &pNkRenderer->Context;
    float clearColor[ 4 ] = {0.5f, 0.5f, 1.0f, 1.0f};

    if ( nk_begin( ctx, "Viewer", nk_rect( 10, 10, 180, 500 ), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_MOVABLE ) ) {

        auto viewMatrix = GetMatrix( pCamController->ViewMatrix( ) );
        auto invViewMatrix = GetMatrix( XMMatrixInverse( nullptr, pCamController->ViewMatrix( ) ) );

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_labelf( ctx,
                   NK_TEXT_LEFT,
                   "View: %2.2f %2.2f %2.2f %2.2f ",
                   invViewMatrix._11,
                   invViewMatrix._12,
                   invViewMatrix._13,
                   invViewMatrix._14 );
        nk_labelf( ctx,
                   NK_TEXT_LEFT,
                   "    : %2.2f %2.2f %2.2f %2.2f ",
                   invViewMatrix._21,
                   invViewMatrix._22,
                   invViewMatrix._23,
                   invViewMatrix._24 );
        nk_labelf( ctx,
                   NK_TEXT_LEFT,
                   "    : %2.2f %2.2f %2.2f %2.2f ",
                   invViewMatrix._31,
                   invViewMatrix._32,
                   invViewMatrix._33,
                   invViewMatrix._34 );
        nk_labelf( ctx,
                   NK_TEXT_LEFT,
                   "    : %2.2f %2.2f %2.2f %2.2f ",
                   invViewMatrix._41,
                   invViewMatrix._42,
                   invViewMatrix._43,
                   invViewMatrix._44 );

        nk_labelf( ctx, NK_TEXT_LEFT, "WorldRotationY" );
        nk_slider_float( ctx, 0, &WorldRotationY, apemodexm::XM_2PI, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightColorR" );
        nk_slider_float( ctx, 0, &LightColor.x, 1, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightColorG" );
        nk_slider_float( ctx, 0, &LightColor.y, 1, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightColorB" );
        nk_slider_float( ctx, 0, &LightColor.z, 1, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightDirectionX" );
        nk_slider_float( ctx, -1, &LightDirection.x, 1, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightDirectionY" );
        nk_slider_float( ctx, -1, &LightDirection.y, 1, 0.1f );

        nk_labelf( ctx, NK_TEXT_LEFT, "LightDirectionZ" );
        nk_slider_float( ctx, -1, &LightDirection.z, 1, 0.1f );
    }
    nk_end(ctx);

    pCamInput->Update( deltaSecs, inputState, {float( Width ), float( Height )} );
    pCamController->Orbit( pCamInput->OrbitDelta );
    pCamController->Dolly( pCamInput->DollyDelta );
    pCamController->Update( deltaSecs );

    if ( auto pAppSurface = static_cast< AppSurfaceSdlVk* >( GetSurface( ) ) ) {
        apemode_memory_allocation_scope;

        auto queueFamilyPool = pAppSurface->Node.GetQueuePool( )->GetPool( pAppSurface->PresentQueueFamilyIds[ 0 ] );
        auto acquiredQueue   = queueFamilyPool->Acquire( true );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = queueFamilyPool->Acquire( true );
        }

        VkDevice        device                   = pAppSurface->Node.hLogicalDevice;
        VkQueue         queue                    = acquiredQueue.pQueue;
        VkSwapchainKHR  swapchain                = pAppSurface->Swapchain.hSwapchain;
        VkFence         fence                    = acquiredQueue.pFence;
        VkSemaphore     presentCompleteSemaphore = hPresentCompleteSemaphores[ FrameIndex ];
        VkSemaphore     renderCompleteSemaphore  = hRenderCompleteSemaphores[ FrameIndex ];
        VkCommandPool   pCmdPool                 = hCmdPool[ FrameIndex ];
        VkCommandBuffer pCmdBuffer               = hCmdBuffers[ FrameIndex ];

        const uint32_t width  = pAppSurface->GetWidth( );
        const uint32_t height = pAppSurface->GetHeight( );

        if ( Width != width || Height != height ) {
            CheckedResult( vkDeviceWaitIdle( device ) );
            OnResized( );
        }

        VkFramebuffer framebufferNk = hNkFramebuffers[ FrameIndex ];
        VkFramebuffer framebufferDbg = hDbgFramebuffers[ FrameIndex ];

        CheckedResult( WaitForFence( device, fence ) );

        CheckedResult( vkAcquireNextImageKHR( device,
            swapchain,
            UINT64_MAX,
            presentCompleteSemaphore,
            VK_NULL_HANDLE,
            &BackbufferIndices[ FrameIndex ] ) );

        CheckedResult( vkResetCommandPool( device, pCmdPool, 0 ) );

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CheckedResult( vkBeginCommandBuffer( pCmdBuffer, &commandBufferBeginInfo ) );

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
        renderPassBeginInfo.framebuffer              = hDbgFramebuffers[ FrameIndex ];
        renderPassBeginInfo.renderArea.extent.width  = pAppSurface->GetWidth( );
        renderPassBeginInfo.renderArea.extent.height = pAppSurface->GetHeight( );
        renderPassBeginInfo.clearValueCount          = 2;
        renderPassBeginInfo.pClearValues             = clearValue;

        vkCmdBeginRenderPass( pCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        auto viewMatrix     = pCamController->ViewMatrix( );
        auto invViewMatrix  = XMMatrixInverse( nullptr, viewMatrix );
        auto projMatrix     = CamProjController.ProjMatrix( 55, float( width ), float( height ), 0.1f, 1000.0f );
        auto projBiasMatrix = CamProjController.ProjBiasMatrix( );
        auto invProjMatrix  = XMMatrixInverse( nullptr, projMatrix );

        vk::DebugRenderer::SkyboxUBO frameData;
        XMStoreFloat4x4( &frameData.ProjMatrix, projMatrix );
        XMStoreFloat4x4( &frameData.ViewMatrix, viewMatrix );
        frameData.Color = {1, 0, 0, 1};

        pSkyboxRenderer->Reset( FrameIndex );
        pDebugRenderer->Reset( FrameIndex );
        pSceneRendererBase->Reset( mLoadedScene.pScene.get( ), FrameIndex );

        vk::SkyboxRenderer::RenderParameters skyboxRenderParams;
        XMStoreFloat4x4( &skyboxRenderParams.InvViewMatrix, invViewMatrix );
        XMStoreFloat4x4( &skyboxRenderParams.InvProjMatrix, invProjMatrix );
        XMStoreFloat4x4( &skyboxRenderParams.ProjBiasMatrix, projBiasMatrix );
        skyboxRenderParams.Dims.x      = float( width );
        skyboxRenderParams.Dims.y      = float( height );
        skyboxRenderParams.Scale.x     = 1;
        skyboxRenderParams.Scale.y     = 1;
        skyboxRenderParams.FrameIndex  = FrameIndex;
        skyboxRenderParams.pCmdBuffer  = pCmdBuffer;
        skyboxRenderParams.pNode       = &pAppSurface->Node;
        skyboxRenderParams.FieldOfView = apemodexm::DegreesToRadians( 67 );

        pSkyboxRenderer->Render( pSkybox.get( ), &skyboxRenderParams );

        vk::DebugRenderer::RenderParameters renderParamsDbg;
        renderParamsDbg.Dims[ 0 ]  = float( width );
        renderParamsDbg.Dims[ 1 ]  = float( height );
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
        pDebugRenderer->Render( &renderParamsDbg );

        worldMatrix = XMMatrixScaling( scale, scale, scale * 2 ) * XMMatrixTranslation( 0, 0, scale * 3 );
        XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
        frameData.Color = {0, 0, 1, 1};
        pDebugRenderer->Render( &renderParamsDbg );


        worldMatrix = XMMatrixScaling( scale * 2, scale, scale ) * XMMatrixTranslation( scale * 3, 0, 0 );
        XMStoreFloat4x4( &frameData.WorldMatrix, worldMatrix );
        frameData.Color = { 1, 0, 0, 1 };
        pDebugRenderer->Render( &renderParamsDbg );

        XMMATRIX rootMatrix = XMMatrixRotationY( WorldRotationY );

        vk::SceneRenderer::RenderParameters sceneRenderParameters;
        sceneRenderParameters.Dims.x                   = float( width );
        sceneRenderParameters.Dims.y                   = float( height );
        sceneRenderParameters.Scale.x                  = 1;
        sceneRenderParameters.Scale.y                  = 1;
        sceneRenderParameters.FrameIndex               = FrameIndex;
        sceneRenderParameters.pCmdBuffer               = pCmdBuffer;
        sceneRenderParameters.pNode                    = &pAppSurface->Node;
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
        pSceneRendererBase->RenderScene( mLoadedScene.pScene.get( ), &sceneRenderParameters );

        vk::NuklearRenderer::RenderParameters renderParamsNk;
        renderParamsNk.Dims[ 0 ]            = float( width );
        renderParamsNk.Dims[ 1 ]            = float( height );
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

        VkPipelineStageFlags waitPipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submitInfo;
        InitializeStruct( submitInfo );
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &renderCompleteSemaphore;
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &presentCompleteSemaphore;
        submitInfo.pWaitDstStageMask    = &waitPipelineStage;
        submitInfo.commandBufferCount   = 1;
        submitInfo.pCommandBuffers      = &pCmdBuffer;

        CheckedResult( vkEndCommandBuffer( pCmdBuffer ) );
        CheckedResult( vkResetFences( device, 1, &fence ) );
        CheckedResult( vkQueueSubmit( queue, 1, &submitInfo, fence ) );

        if ( FrameId ) {
            uint32_t    presentIndex    = ( FrameIndex + FrameCount - 1 ) % FrameCount;
            VkSemaphore renderSemaphore = hRenderCompleteSemaphores[ presentIndex ];

            VkPresentInfoKHR presentInfoKHR;
            InitializeStruct( presentInfoKHR );
            presentInfoKHR.waitSemaphoreCount = 1;
            presentInfoKHR.pWaitSemaphores    = &renderSemaphore;
            presentInfoKHR.swapchainCount     = 1;
            presentInfoKHR.pSwapchains        = &swapchain;
            presentInfoKHR.pImageIndices      = &BackbufferIndices[ presentIndex ];

            CheckedResult( vkQueuePresentKHR( queue, &presentInfoKHR ) );
        }

        queueFamilyPool->Release(acquiredQueue);

    }
}

bool ViewerApp::IsRunning( ) {
    apemode_memory_allocation_scope;

    if ( AppBase::IsRunning( ) )
        return true;

    AppSurfaceBase*  pSurfaceBase = GetSurface( );
    AppSurfaceSdlVk* pSurface = static_cast< AppSurfaceSdlVk* >( pSurfaceBase );

    CheckedResult( vkDeviceWaitIdle( pSurface->Node ) );
    return false;
}

extern "C" AppBase* CreateApp( ) {
    apemode_memory_allocation_scope;
    return apemode_new ViewerApp( );
}
