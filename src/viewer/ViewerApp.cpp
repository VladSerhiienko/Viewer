#include <ViewerApp.h>

using namespace apemode;

#ifndef _WIN32

inline void DebugBreak( ) {
    apemodevk::platform::DebugBreak( );
}

inline void OutputDebugStringA( const char* pDebugStringA ) {
    SDL_LogInfo( SDL_LOG_CATEGORY_RENDER, pDebugStringA );
}

#endif

namespace apemode {
    using namespace apemodexm;
    using namespace apemodevk;
}

struct EFeedbackTypeWithOStream {
    apemodevk::ShaderCompiler::IShaderFeedbackWriter::EFeedbackType e;

    EFeedbackTypeWithOStream(  apemodevk::ShaderCompiler::IShaderFeedbackWriter::EFeedbackType e) : e(e) {
    }

    // clang-format off
    template < typename OStream >
    friend OStream& operator<<( OStream& os, const EFeedbackTypeWithOStream& feedbackType ) {
        switch ( feedbackType.e ) {
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly:                 return os << "Assembly";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed:             return os << "Preprocessed";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_PreprocessedOptimized:    return os << "PreprocessedOptimized";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv:                      return os << "Spv";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_CompilationError:        return os << "CompilationError";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InternalError:           return os << "InternalError";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InvalidAssembly:         return os << "InvalidAssembly";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InvalidStage:            return os << "InvalidStage";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_NullResultObject:        return os << "NullResultObject";
        case apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success:                 return os << "Success";
        default:                                                                                                        return os;
        }
    }
    // clang-format on
};

bool ShaderFileReader::ReadShaderTxtFile( const std::string& InFilePath,
                                          std::string&       OutFileFullPath,
                                          std::string&       OutFileContent ) {
    if ( auto pAsset = mAssetManager->GetAsset( InFilePath ) ) {
        OutFileContent  = pAsset->AsTxt( );
        OutFileFullPath = pAsset->GetId( );
        return true;
    }

    apemodevk::platform::DebugBreak( );
    return false;
}

void ShaderFeedbackWriter::WriteFeedback( EFeedbackType                                     eType,
                                          const std::string&                                FullFilePath,
                                          const ShaderCompiler::IMacroDefinitionCollection* pMacros,
                                          const void*                                       pContent,
                                          const void*                                       pContentEnd ) {

    const auto feedbackStage            = eType & eFeedbackType_CompilationStageMask;
    const auto feedbackCompilationError = eType & eFeedbackType_CompilationStatusMask;

    if ( eFeedbackType_CompilationStatus_Success != feedbackCompilationError ) {
        AppState::Get( )->Logger->error( "ShaderCompiler: {} / {} / {}",
                                         EFeedbackTypeWithOStream( feedbackStage ),
                                         EFeedbackTypeWithOStream( feedbackCompilationError ),
                                         FullFilePath );
        AppState::Get( )->Logger->error( "           Msg: {}", (const char*) pContent );
        DebugBreak( );
    } else {
        AppState::Get( )->Logger->info( "ShaderCompiler: {} / {} / {}",
                                        EFeedbackTypeWithOStream( feedbackStage ),
                                        EFeedbackTypeWithOStream( feedbackCompilationError ),
                                        FullFilePath );
        // TODO: Store compiled shader to file system
    }
}

const VkFormat sDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
//const VkFormat sDepthFormat = VK_FORMAT_D16_UNORM;

ViewerApp::ViewerApp( ) {
    pCamController = apemodevk::make_unique< ModelViewCameraController >();
    pCamInput      = apemodevk::make_unique< MouseKeyboardCameraControllerInput >( );
    //pCamController = apemodevk::make_unique< FreeLookCameraController >( );
}

ViewerApp::~ViewerApp( ) {
}

AppSurfaceBase* ViewerApp::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool ViewerApp::Initialize(  ) {

    if ( AppBase::Initialize( ) ) {

        std::string assetsFolder = TGetOption< std::string >( "--assets", "./" );
        mAssetManager.AddFilesFromDirectory( assetsFolder, {} );

        // Shaders and possible headers ...
        std::string interestingFilePattern = ".*\\.(vert|frag|comp|geom|tesc|tese|h|inl|inc|fx)$";
        mFileTracker.FilePatterns.push_back( interestingFilePattern );
        mFileTracker.ScanDirectory( assetsFolder, true );

        pShaderFileReader = apemodevk::make_unique< apemode::ShaderFileReader >( );
        pShaderFileReader->mAssetManager = &mAssetManager;

        pShaderFeedbackWriter = apemodevk::make_unique< apemode::ShaderFeedbackWriter >( );

        pShaderCompiler = apemodevk::make_unique< apemodevk::ShaderCompiler >( );
        pShaderCompiler->SetShaderFileReader( pShaderFileReader.get( ) );
        pShaderCompiler->SetShaderFeedbackWriter( pShaderFeedbackWriter.get( ) );

        totalSecs = 0.0f;

        auto appSurfaceBase = GetSurface( );
        if ( appSurfaceBase->GetImpl( ) != kAppSurfaceImpl_SdlVk )
            return false;

        auto appSurface = (AppSurfaceSdlVk*) appSurfaceBase;

        FrameId    = 0;
        FrameIndex = 0;
        FrameCount = appSurface->Swapchain.ImgCount;

        OnResized();

        for (uint32_t i = 0; i < FrameCount; ++i) {
            VkCommandPoolCreateInfo cmdPoolCreateInfo;
            InitializeStruct( cmdPoolCreateInfo );
            cmdPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            cmdPoolCreateInfo.queueFamilyIndex = appSurface->PresentQueueFamilyIds[0];

            if ( false == hCmdPool[ i ].Recreate( *appSurface->pNode, cmdPoolCreateInfo ) ) {
                DebugBreak( );
                return false;
            }

            VkCommandBufferAllocateInfo cmdBufferAllocInfo;
            InitializeStruct( cmdBufferAllocInfo );
            cmdBufferAllocInfo.commandPool = hCmdPool[ i ];
            cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdBufferAllocInfo.commandBufferCount = 1;

            if ( false == hCmdBuffers[ i ].Recreate( *appSurface->pNode, cmdBufferAllocInfo ) ) {
                DebugBreak( );
                return false;
            }

            VkSemaphoreCreateInfo semaphoreCreateInfo;
            InitializeStruct( semaphoreCreateInfo );
            if ( false == hPresentCompleteSemaphores[ i ].Recreate( *appSurface->pNode, semaphoreCreateInfo ) ||
                 false == hRenderCompleteSemaphores[ i ].Recreate( *appSurface->pNode, semaphoreCreateInfo ) ) {
                DebugBreak( );
                return false;
            }
        }

        if ( false == DescriptorPool.RecreateResourcesFor( *appSurface->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 ) ) {
            DebugBreak( );
            return false;
        }

        pNkRenderer = new NuklearRendererSdlVk();

        auto queueFamilyPool = appSurface->pNode->GetQueuePool( )->GetPool( appSurface->PresentQueueFamilyIds[ 0 ] );
        apemodevk::AcquiredQueue acquiredQueue;

        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = queueFamilyPool->Acquire( false );
        }

        NuklearRendererSdlVk::InitParametersVk initParamsNk;
        initParamsNk.pNode           = appSurface->pNode;
        initParamsNk.pShaderCompiler = pShaderCompiler.get( );
        initParamsNk.pFontAsset      = mAssetManager.GetAsset( "fonts/iosevka-ss07-medium.ttf" );
        initParamsNk.pRenderPass     = hDbgRenderPass;
        initParamsNk.pDescPool       = DescriptorPool;
        initParamsNk.pQueue          = acquiredQueue.pQueue;
        initParamsNk.QueueFamilyId   = acquiredQueue.queueFamilyId;
        // initParamsNk.pRenderPass     = hNkRenderPass;

        pNkRenderer->Init( &initParamsNk );

        queueFamilyPool->Release( acquiredQueue );

        DebugRendererVk::InitParametersVk initParamsDbg;
        initParamsDbg.pNode           = appSurface->pNode;
        initParamsDbg.pShaderCompiler = pShaderCompiler.get( );
        initParamsDbg.pRenderPass     = hDbgRenderPass;
        initParamsDbg.pDescPool       = DescriptorPool;
        initParamsDbg.FrameCount      = FrameCount;

        pDebugRenderer = new DebugRendererVk();
        pDebugRenderer->RecreateResources( &initParamsDbg );

        pSceneRendererBase = appSurface->CreateSceneRenderer( );

        SceneRendererVk::RecreateParametersVk recreateParams;
        recreateParams.pNode           = appSurface->pNode;
        recreateParams.pShaderCompiler = pShaderCompiler.get( );
        recreateParams.pRenderPass     = hDbgRenderPass;
        recreateParams.pDescPool       = DescriptorPool;
        recreateParams.FrameCount      = FrameCount;

        SceneRendererVk::SceneUpdateParametersVk updateParams;
        updateParams.pNode           = appSurface->pNode;
        updateParams.pShaderCompiler = pShaderCompiler.get( );
        updateParams.pRenderPass     = hDbgRenderPass;
        updateParams.pDescPool       = DescriptorPool;
        updateParams.FrameCount      = FrameCount;

        // --assets "..\..\assets\**" --scene "C:/Sources/Models/bristleback-dota-fan-art.fbxp"
        // --assets "..\..\assets\**" --scene "C:/Sources/Models/skull_salazar.fbxp"
        // --assets "..\..\assets\**" --scene "C:/Sources/Models/1972-datsun-240k-gt.fbxp"
        // --assets "..\..\assets\**" --scene "C:\Sources\Models\graograman.fbxp"
        // --assets "..\..\assets\**" --scene "C:/Sources/Models/dreadroamer-free.fbxp"
        // --assets "..\..\assets\**" --scene "C:/Sources/Models/warcraft-draenei-fanart.fbxp"
        if ( false == pSceneRendererBase->Recreate( &recreateParams ) ) {
            DebugBreak( );
            return false;
        }

        auto sceneFile = TGetOption< std::string >( "scene", "" ); {
            auto sceneFileContent = apemodeos::FileReader( ).ReadBinFile( sceneFile );
            auto loadedScene = LoadSceneFromBin( sceneFileContent.data( ), sceneFileContent.size( ) );

            pScene = std::move( loadedScene.first );
            pSceneSource = SceneSourceData( loadedScene.second, std::move( sceneFileContent ) );
        }

        updateParams.pSceneSrc = pSceneSource.first;
        if ( false == pSceneRendererBase->UpdateScene( pScene.get( ), &updateParams ) ) {
            DebugBreak( );
            return false;
        }

        apemodevk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode           = appSurface->pNode;
        skyboxRendererRecreateParams.pShaderCompiler = pShaderCompiler.get();
        skyboxRendererRecreateParams.pRenderPass     = hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool       = DescriptorPool;
        skyboxRendererRecreateParams.FrameCount      = FrameCount;

        pSkyboxRenderer = apemodevk::make_unique< apemodevk::SkyboxRenderer >( );
        if ( false == pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
            DebugBreak( );
            return false;
        }

        pSamplerManager = apemodevk::make_unique< apemodevk::SamplerManager >( );
        if ( false == pSamplerManager->Recreate( appSurface->pNode ) ) {
            DebugBreak( );
            return false;
        }

        apemodevk::ImageLoader imgLoader;
        if ( false == imgLoader.Recreate( appSurface->pNode, nullptr ) ) {
            DebugBreak( );
            return false;
        }

        // if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/PaperMill/Specular_HDR.dds" ) ) {
        if ( auto pTexAsset = mAssetManager.GetAsset( "images/Environment/Canyon/Unfiltered_HDR.dds" ) ) {
            pLoadedDDS = imgLoader.LoadImageFromData( pTexAsset->AsBin( ), apemodevk::ImageLoader::eImageFileFormat_DDS, true, true );
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
        samplerCreateInfo.maxLod                  = (float) pLoadedDDS->ImageCreateInfo.mipLevels;
        samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = false;

        auto samplerIndex = pSamplerManager->GetSamplerIndex( samplerCreateInfo );
        assert( SamplerManager::IsSamplerIndexValid( samplerIndex ) );

        pSkybox                = apemodevk::make_unique< apemodevk::Skybox >( );
        pSkybox->pSampler      = pSamplerManager->StoredSamplers[ samplerIndex ].pSampler;
        pSkybox->pImgView      = pLoadedDDS->hImgView;
        pSkybox->Dimension     = pLoadedDDS->ImageCreateInfo.extent.width;
        pSkybox->eImgLayout    = pLoadedDDS->eImgLayout;
        pSkybox->Exposure      = 3;
        pSkybox->LevelOfDetail = 0;

        return true;
    }

    return false;
}

bool apemode::ViewerApp::OnResized( ) {
    if ( auto appSurface = (AppSurfaceSdlVk*) GetSurface( ) ) {

            width  = appSurface->GetWidth( );
            height = appSurface->GetHeight( );

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
                if ( false == hDepthImgs[ i ].Recreate( appSurface->pNode->hAllocator, depthImgCreateInfo, allocationCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                depthImgViewCreateInfo.image = hDepthImgs[ i ];
                if ( false == hDepthImgViews[ i ].Recreate( *appSurface->pNode, depthImgViewCreateInfo ) ) {
                    DebugBreak( );
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

            if ( false == hNkRenderPass.Recreate( *appSurface->pNode, renderPassCreateInfoNk ) ) {
                DebugBreak( );
                return false;
            }

            if ( false == hDbgRenderPass.Recreate( *appSurface->pNode, renderPassCreateInfoDbg ) ) {
                DebugBreak( );
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
                VkImageView attachments[ 1 ] = {appSurface->Swapchain.hImgViews[ i ]};
                framebufferCreateInfoNk.pAttachments = attachments;

                if ( false == hNkFramebuffers[ i ].Recreate( *appSurface->pNode, framebufferCreateInfoNk ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                VkImageView attachments[ 2 ] = {appSurface->Swapchain.hImgViews[ i ], hDepthImgViews[ i ]};
                framebufferCreateInfoDbg.pAttachments = attachments;

                if ( false == hDbgFramebuffers[ i ].Recreate( *appSurface->pNode, framebufferCreateInfoDbg ) ) {
                    DebugBreak( );
                    return false;
                }
            }
    }

    return true;
}

void ViewerApp::OnFrameMove( ) {
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
    totalSecs += deltaSecs;

    bool hovered = false;
    bool reset   = false;

    const nk_flags windowFlags
        = NK_WINDOW_BORDER
        | NK_WINDOW_MOVABLE
        | NK_WINDOW_SCALABLE
        | NK_WINDOW_MINIMIZABLE;

    auto ctx = &pNkRenderer->Context;
    float clearColor[ 4 ] = {0.5f, 0.5f, 1.0f, 1.0f};

    if (nk_begin(ctx, "Calculator", nk_rect(10, 10, 180, 250),
        NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE))
    {
        static int set = 0, prev = 0, op = 0;
        static const char numbers[] = "789456123";
        static const char ops[] = "+-*/";
        static double a = 0, b = 0;
        static double *current = &a;

        size_t i = 0;
        int solve = 0;
        {int len; char buffer[256];
        nk_layout_row_dynamic(ctx, 35, 1);
        len = snprintf(buffer, 256, "%.2f", *current);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, buffer, &len, 255, nk_filter_float);
        buffer[len] = 0;
        *current = atof(buffer);}

        nk_layout_row_dynamic(ctx, 35, 4);
        for (i = 0; i < 16; ++i) {
            if (i >= 12 && i < 15) {
                if (i > 12) continue;
                if (nk_button_label(ctx, "C")) {
                    a = b = op = 0; current = &a; set = 0;
                } if (nk_button_label(ctx, "0")) {
                    *current = *current*10.0f; set = 0;
                } if (nk_button_label(ctx, "=")) {
                    solve = 1; prev = op; op = 0;
                }
            } else if (((i+1) % 4)) {
                if (nk_button_text(ctx, &numbers[(i/4)*3+i%4], 1)) {
                    *current = *current * 10.0f + numbers[(i/4)*3+i%4] - '0';
                    set = 0;
                }
            } else if (nk_button_text(ctx, &ops[i/4], 1)) {
                if (!set) {
                    if (current != &b) {
                        current = &b;
                    } else {
                        prev = op;
                        solve = 1;
                    }
                }
                op = ops[i/4];
                set = 1;
            }
        }
        if (solve) {
            if (prev == '+') a = a + b;
            if (prev == '-') a = a - b;
            if (prev == '*') a = a * b;
            if (prev == '/') a = a / b;
            current = &a;
            if (set) current = &b;
            b = 0; set = 0;
        }
    }
    nk_end(ctx);

    pCamInput->Update( deltaSecs, inputState, {(float) width, (float) height} );
    pCamController->Orbit( pCamInput->OrbitDelta );
    pCamController->Dolly( pCamInput->DollyDelta );
    pCamController->Update( deltaSecs );

    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        auto queueFamilyPool = appSurfaceVk->pNode->GetQueuePool( )->GetPool( appSurfaceVk->PresentQueueFamilyIds[ 0 ] );
        auto acquiredQueue   = queueFamilyPool->Acquire( true );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = queueFamilyPool->Acquire( true );
        }

        VkDevice        device                   = *appSurfaceVk->pNode;
        VkQueue         queue                    = acquiredQueue.pQueue;
        VkSwapchainKHR  swapchain                = appSurfaceVk->Swapchain.hSwapchain;
        VkFence         fence                    = acquiredQueue.pFence;
        VkSemaphore     presentCompleteSemaphore = hPresentCompleteSemaphores[ FrameIndex ];
        VkSemaphore     renderCompleteSemaphore  = hRenderCompleteSemaphores[ FrameIndex ];
        VkCommandPool   cmdPool                  = hCmdPool[ FrameIndex ];
        VkCommandBuffer cmdBuffer                = hCmdBuffers[ FrameIndex ];

        const uint32_t width  = appSurfaceVk->GetWidth( );
        const uint32_t height = appSurfaceVk->GetHeight( );

        if ( width != width || height != height ) {
            CheckedCall( vkDeviceWaitIdle( device ) );
            OnResized( );
        }

        VkFramebuffer framebufferNk = hNkFramebuffers[ FrameIndex ];
        VkFramebuffer framebufferDbg = hDbgFramebuffers[ FrameIndex ];

        while (true) {
            const auto waitForFencesErrorHandle = vkWaitForFences( device, 1, &fence, VK_TRUE, 100 );
            if ( VK_SUCCESS == waitForFencesErrorHandle ) {
                break;
            } else if ( VK_TIMEOUT == waitForFencesErrorHandle ) {
                continue;
            } else {
                assert( false );
                return;
            }
        }

        CheckedCall( vkAcquireNextImageKHR( device,
            swapchain,
            UINT64_MAX,
            presentCompleteSemaphore,
            VK_NULL_HANDLE,
            &BackbufferIndices[ FrameIndex ] ) );

        CheckedCall( vkResetCommandPool( device, cmdPool, 0 ) );

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        CheckedCall( vkBeginCommandBuffer( cmdBuffer, &commandBufferBeginInfo ) );

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
        renderPassBeginInfo.renderArea.extent.width  = appSurfaceVk->GetWidth( );
        renderPassBeginInfo.renderArea.extent.height = appSurfaceVk->GetHeight( );
        renderPassBeginInfo.clearValueCount          = 2;
        renderPassBeginInfo.pClearValues             = clearValue;

        vkCmdBeginRenderPass( cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

        auto View    = pCamController->ViewMatrix( );
        auto InvView = XMMatrixInverse( nullptr, View );
        auto Proj    = CamProjController.ProjMatrix( 55, (float) width, (float) height, 0.1f, 1000.0f );
        auto InvProj = XMMatrixInverse( nullptr, Proj );

        DebugRendererVk::FrameUniformBuffer frameData;
        XMStoreFloat4x4( &frameData.projectionMatrix, Proj );
        XMStoreFloat4x4( &frameData.viewMatrix, pCamController->ViewMatrix( ) );
        frameData.color = {1, 0, 0, 1};

        pSkyboxRenderer->Reset( FrameIndex );
        pDebugRenderer->Reset( FrameIndex );
        pSceneRendererBase->Reset( pScene.get( ), FrameIndex );

        apemodevk::SkyboxRenderer::RenderParameters skyboxRenderParams;
        XMStoreFloat4x4( &skyboxRenderParams.InvViewMatrix, InvView );
        XMStoreFloat4x4( &skyboxRenderParams.InvProjMatrix, InvProj );
        XMStoreFloat4x4( &skyboxRenderParams.ProjBiasMatrix, CamProjController.ProjBiasMatrix( ) );
        skyboxRenderParams.Dims.x      = (float) width;
        skyboxRenderParams.Dims.y      = (float) height;
        skyboxRenderParams.Scale.x     = 1;
        skyboxRenderParams.Scale.y     = 1;
        skyboxRenderParams.FrameIndex  = FrameIndex;
        skyboxRenderParams.pCmdBuffer  = cmdBuffer;
        skyboxRenderParams.pNode       = appSurfaceVk->pNode;
        skyboxRenderParams.FieldOfView = apemodexm::DegreesToRadians( 67 );
        skyboxRenderParams.FieldOfView = apemodexm::DegreesToRadians( 67 );

        pSkyboxRenderer->Render( pSkybox.get( ), &skyboxRenderParams );

        DebugRendererVk::RenderParametersVk renderParamsDbg;
        renderParamsDbg.Dims[ 0 ]  = (float) width;
        renderParamsDbg.Dims[ 1 ]  = (float) height;
        renderParamsDbg.Scale[ 0 ] = 1;
        renderParamsDbg.Scale[ 1 ] = 1;
        renderParamsDbg.FrameIndex = FrameIndex;
        renderParamsDbg.pCmdBuffer = cmdBuffer;
        renderParamsDbg.pFrameData = &frameData;

        const float scale = 0.5f;

        XMMATRIX worldMatrix;

        worldMatrix = XMMatrixScaling( scale, scale * 2, scale ) * XMMatrixTranslation( 0, scale * 3, 0 );
        XMStoreFloat4x4( &frameData.worldMatrix, worldMatrix );
        frameData.color = {0, 1, 0, 1};
        pDebugRenderer->Render( &renderParamsDbg );

        worldMatrix = XMMatrixScaling( scale, scale, scale * 2 ) * XMMatrixTranslation( 0, 0, scale * 3 );
        XMStoreFloat4x4( &frameData.worldMatrix, worldMatrix );
        frameData.color = {0, 0, 1, 1};
        pDebugRenderer->Render( &renderParamsDbg );


        worldMatrix = XMMatrixScaling( scale * 2, scale, scale ) * XMMatrixTranslation( scale * 3, 0, 0 );
        XMStoreFloat4x4( &frameData.worldMatrix, worldMatrix );
        frameData.color = { 1, 0, 0, 1 };
        pDebugRenderer->Render( &renderParamsDbg );

        frameData.color = { 1, 0, 0, 1 };
        pDebugRenderer->Render(&renderParamsDbg);

        apemode::SceneRendererVk::SceneRenderParametersVk sceneRenderParameters;
        sceneRenderParameters.Dims.x     = (float) width;
        sceneRenderParameters.Dims.y     = (float) height;
        sceneRenderParameters.Scale.x    = 1;
        sceneRenderParameters.Scale.y    = 1;
        sceneRenderParameters.FrameIndex = FrameIndex;
        sceneRenderParameters.pCmdBuffer = cmdBuffer;
        sceneRenderParameters.pNode      = appSurfaceVk->pNode;
        sceneRenderParameters.ViewMatrix = frameData.viewMatrix;
        sceneRenderParameters.ProjMatrix = frameData.projectionMatrix;
        //pSceneRendererBase->RenderScene( pScene.get( ), &sceneRenderParameters );

        NuklearRendererSdlVk::RenderParametersVk renderParamsNk;
        renderParamsNk.dims[ 0 ]          = (float) width;
        renderParamsNk.dims[ 1 ]          = (float) height;
        renderParamsNk.scale[ 0 ]         = 1;
        renderParamsNk.scale[ 1 ]         = 1;
        renderParamsNk.aa                 = NK_ANTI_ALIASING_ON;
        renderParamsNk.max_vertex_buffer  = 64 * 1024;
        renderParamsNk.max_element_buffer = 64 * 1024;
        renderParamsNk.FrameIndex         = FrameIndex;
        renderParamsNk.pCmdBuffer         = cmdBuffer;

        pNkRenderer->Render( &renderParamsNk );
        nk_clear( &pNkRenderer->Context );

        vkCmdEndRenderPass( cmdBuffer );

        pDebugRenderer->Flush( FrameIndex );
        pSceneRendererBase->Flush( pScene.get( ), FrameIndex );
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
        submitInfo.pCommandBuffers      = &cmdBuffer;

        CheckedCall( vkEndCommandBuffer( cmdBuffer ) );
        CheckedCall( vkResetFences( device, 1, &fence ) );
        CheckedCall( vkQueueSubmit( queue, 1, &submitInfo, fence ) );

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

            CheckedCall( vkQueuePresentKHR( queue, &presentInfoKHR ) );
        }

        queueFamilyPool->Release(acquiredQueue);

    }
}

bool ViewerApp::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new ViewerApp( );
}
