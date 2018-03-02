#include <ViewerApp.h>

using namespace apemode;

inline void DebugBreak( ) {
    apemodevk::platform::DebugBreak( );
}

inline void OutputDebugStringA( const char* pDebugStringA ) {
    SDL_LogInfo( SDL_LOG_CATEGORY_RENDER, pDebugStringA );
}

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

bool ShaderFileReader::ReadShaderTxtFile( const std::string& FilePath,
                                          std::string&       OutFileFullPath,
                                          std::string&       OutFileContent ) {
    OutFileFullPath = apemodeos::ResolveFullPath( FilePath );
    OutFileContent  = pFileManager->ReadTxtFile( FilePath );
    return false == OutFileContent.empty( );
}

void ShaderFeedbackWriter::WriteFeedback( EFeedbackType                     eType,
                                          const std::string&                FullFilePath,
                                          const std::vector< std::string >& Macros,
                                          const void*                       pContent, /* Txt or bin, @see EFeedbackType */
                                          const void*                       pContentEnd ) {
                                              
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
    pCamController = new FreeLookCameraController( );
    pCamInput      = new MouseKeyboardCameraControllerInput( );
    AppState::Get()->Options->add_options( "vk" )
        ( "renderdoc", "Adds renderdoc layer to device layers" )
        ( "vkapidump", "Adds api dump layer to vk device layers" )
        ( "vktrace", "Adds vktrace layer to vk device layers" );
}

ViewerApp::~ViewerApp( ) {
}

AppSurfaceBase* ViewerApp::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool ViewerApp::Initialize(  ) {

    if ( AppBase::Initialize( ) ) {

        FileTracker.FilePatterns.push_back( ".*\\.(vert|frag|comp|geom|tesc|tese|h|hpp|inl|inc|fx)$" );
        FileTracker.ScanDirectory( "../assets/shaders/**", true );

        ShaderFileReader.pFileManager = &FileManager;
        ShaderFeedbackWriter.pFileManager = &FileManager;

        pShaderCompiler = new apemodevk::ShaderCompiler( );
        pShaderCompiler->SetShaderFileReader( &ShaderFileReader );
        pShaderCompiler->SetShaderFeedbackWriter( &ShaderFeedbackWriter );

        totalSecs = 0.0f;

        auto appSurfaceBase = GetSurface();
        if (appSurfaceBase->GetImpl() != kAppSurfaceImpl_SdlVk)
            return false;

        auto appSurface = (AppSurfaceSdlVk*) appSurfaceBase;
        if ( auto swapchain = &appSurface->Swapchain ) {
            FrameId    = 0;
            FrameIndex = 0;
            FrameCount = swapchain->ImgCount;

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
        }

        if ( false == DescPool.RecreateResourcesFor( *appSurface->pNode, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256 ) ) {
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
        initParamsNk.pAlloc          = nullptr;
        initParamsNk.pDevice         = *appSurface->pNode;
        initParamsNk.pPhysicalDevice = *appSurface->pNode;
        initParamsNk.pRenderPass     = hDbgRenderPass;
        //initParamsNk.pRenderPass     = hNkRenderPass;
        initParamsNk.pDescPool       = DescPool;
        initParamsNk.pQueue          = acquiredQueue.pQueue;
        initParamsNk.queueFamilyId   = acquiredQueue.queueFamilyId;

        pNkRenderer->Init( &initParamsNk );

        queueFamilyPool->Release( acquiredQueue );

        pDebugRenderer = new DebugRendererVk( );

        DebugRendererVk::InitParametersVk initParamsDbg;
        initParamsDbg.pAlloc          = nullptr;
        initParamsDbg.pDevice         = *appSurface->pNode;
        initParamsDbg.pPhysicalDevice = *appSurface->pNode;
        initParamsDbg.pRenderPass     = hDbgRenderPass;
        initParamsDbg.pDescPool       = DescPool;
        initParamsDbg.FrameCount      = FrameCount;

        pDebugRenderer->RecreateResources( &initParamsDbg );
        pSceneRendererBase = appSurface->CreateSceneRenderer( );

        SceneRendererVk::RecreateParametersVk recreateParams;
        recreateParams.pNode           = appSurface->pNode;
        recreateParams.pShaderCompiler = pShaderCompiler;
        recreateParams.pRenderPass     = hDbgRenderPass;
        recreateParams.pDescPool       = DescPool;
        recreateParams.FrameCount      = FrameCount;

        SceneRendererVk::SceneUpdateParametersVk updateParams;
        updateParams.pNode           = appSurface->pNode;
        updateParams.pShaderCompiler = pShaderCompiler;
        updateParams.pRenderPass     = hDbgRenderPass;
        updateParams.pDescPool       = DescPool;
        updateParams.FrameCount      = FrameCount;

        // -i "E:\Media\Models\run-hedgehog-run\source\c61cca893cdc4f82b26a1d66585bd55d.zip\HEDGEHOG_ANIMATION_1024\animation_NEW_ONLY_ONE_ACTION_2.fbx" -o "$(SolutionDir)assets\hedgehogp.fbxp" -p
        // -i "E:\Media\Models\bristleback-dota-fan-art\source\POSE.fbx" -o "$(SolutionDir)assets\bristlebackp.fbxp" -p
        // -i "E:\Media\Models\blood-and-fire\source\DragonMain.fbx" -o "$(SolutionDir)assets\DragonMainp.fbxp" -p
        // -i "E:\Media\Models\knight-artorias\source\Artorias.fbx.fbx" -o "$(SolutionDir)assets\Artoriasp.fbxp" -p
        // -i "E:\Media\Models\vanille-flirty-animation\source\happy.fbx" -o "$(SolutionDir)assets\vanille-flirty-animation.fbxp" -p
        // -i "E:\Media\Models\special-sniper-rifle-vss-vintorez\source\vintorez.FBX" -o "$(SolutionDir)assets\vintorez.fbxp" -p
        // -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45p.fbxp" -p
        // -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45.fbxp"
        // -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4p.fbxp" -p
        // -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4.fbxp"
        // -i "E:\Media\Models\carambit\source\Knife.fbx" -o "$(SolutionDir)assets\Knifep.fbxp" -p
        // -i "E:\Media\Models\pontiac-firebird-formula-1974\source\carz.obj 2.zip\carz.obj\mesh.obj" -o "$(SolutionDir)assets\pontiacp.fbxp" -p
        //  .\FbxPipeline.exe -i E:\Media\Models\1972-datsun-240k-gt\source\datsun240k.fbx -o E:\Media\Models\1972-datsun-240k-gt\source\datsun240k.fbxp -p -m .*\.png
        //  .\FbxPipeline -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "E:\Media\Models\mech-m-6k\source\Mech6kv4p.fbxp" -p -m .*\.png
        // ./FbxPipeline -i "/home/user/vserhiienko/models/mech-m-6k/source/93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "/home/user/vserhiienko/models/mech-m-6k/source/Mech6kv4p.fbx" -p -m .*\.png
        //Scenes.push_back( LoadSceneFromFile( "/home/user/vserhiienko/models/mech-m-6k/source/Mech6kv4p.fbx" ) );

        Scenes.push_back( LoadSceneFromFile( "/home/user/vserhiienko/models_v2/9_mm.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "E:/Media/Models/mech-m-6k/source/Mech6kv4p.fbxp" ) );

        //Scenes.push_back( LoadSceneFromFile( "E:/Media/Models/1972-datsun-240k-gt/source/datsun240k.fbxp" ) );
        //Scenes.push_back( LoadSceneFromFile( "/home/user/vserhiienko/models/stesla-elephant-steam-engines/source/stesla.fbxp" ) );

        // Scenes.push_back( LoadSceneFromFile( "../../../assets/DragonMainp.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Artoriasp.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/vanille-flirty-animation.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/vintorez.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4p.fbxp" ) );
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/A45p.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/A45.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Cube10p.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Knifep.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/pontiacp.fbxp" ));
        // Scenes.push_back( LoadSceneFromFile( "F:/Dev/Projects/ProjectFbxPipeline/FbxPipeline/assets/Mech6kv4p.fbxp" ));
        updateParams.pSceneSrc = Scenes.back( )->sourceScene;

        if ( false == pSceneRendererBase->Recreate( &recreateParams ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == pSceneRendererBase->UpdateScene( Scenes.back( ), &updateParams ) ) {
            DebugBreak( );
            return false;
        }

        pSkybox         = new apemodevk::Skybox( );
        pSkyboxRenderer = new apemodevk::SkyboxRenderer( );
        pSamplerManager = new apemodevk::SamplerManager( );

        apemodevk::SkyboxRenderer::RecreateParameters skyboxRendererRecreateParams;
        skyboxRendererRecreateParams.pNode           = appSurface->pNode;
        skyboxRendererRecreateParams.pShaderCompiler = pShaderCompiler;
        skyboxRendererRecreateParams.pRenderPass     = hDbgRenderPass;
        skyboxRendererRecreateParams.pDescPool       = DescPool;
        skyboxRendererRecreateParams.FrameCount      = FrameCount;

        if ( false == pSkyboxRenderer->Recreate( &skyboxRendererRecreateParams ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == pSamplerManager->Recreate( appSurface->pNode ) ) {
            DebugBreak( );
            return false;
        }

        apemodeos::FileManager imgFileManager;
        apemodevk::ImageLoader imgLoader;
        imgLoader.Recreate( appSurface->pNode, nullptr );

        //auto pngContent = imgFileManager.ReadBinFile( "../../../assets/img/DragonMain_Diff.png" );
        //auto loadedPNG  = imgLoader.LoadImageFromData( pngContent, apemodevk::ImageLoader::eImageFileFormat_PNG, true, true );

        //auto ddsContent = imgFileManager.ReadBinFile( "../../../assets/env/kyoto_lod.dds" );
        //auto ddsContent = imgFileManager.ReadBinFile( "../../../assets/env/PaperMill/Specular_HDR.dds" );
        //auto ddsContent = imgFileManager.ReadBinFile( "../assets/textures/Environment/Canyon/Unfiltered_HDR.dds" );
        auto ddsContent = imgFileManager.ReadBinFile( "../../assets/textures/Environment/Canyon/Unfiltered_HDR.dds" );
        pLoadedDDS = imgLoader.LoadImageFromData( ddsContent, apemodevk::ImageLoader::eImageFileFormat_DDS, true, true ).release( );

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
        samplerCreateInfo.maxLod                  = pLoadedDDS->imageCreateInfo.mipLevels;
        samplerCreateInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = false;

        auto samplerIndex = pSamplerManager->GetSamplerIndex(samplerCreateInfo);
        assert ( samplerIndex != 0xffffffff );

        pSkybox->pSampler      = pSamplerManager->StoredSamplers[samplerIndex].pSampler;
        pSkybox->pImgView      = pLoadedDDS->hImgView;
        pSkybox->Dimension     = pLoadedDDS->imageCreateInfo.extent.width;
        pSkybox->eImgLayout    = pLoadedDDS->eImgLayout;
        pSkybox->LevelOfDetail = 0;
        pSkybox->Exposure      = 3;

        return true;
    }

    return false;
}

bool apemode::ViewerApp::OnResized( ) {
    if ( auto appSurfaceVk = (AppSurfaceSdlVk*) GetSurface( ) ) {
        if ( auto swapchain = &appSurfaceVk->Swapchain ) {
            width  = appSurfaceVk->GetWidth( );
            height = appSurfaceVk->GetHeight( );

            VkImageCreateInfo depthImgCreateInfo;
            InitializeStruct( depthImgCreateInfo );
            depthImgCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
            depthImgCreateInfo.format        = sDepthFormat;
            depthImgCreateInfo.extent.width  = swapchain->ImgExtent.width;
            depthImgCreateInfo.extent.height = swapchain->ImgExtent.height;
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

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                if ( false == hDepthImgs[ i ].Recreate( *appSurfaceVk->pNode, *appSurfaceVk->pNode, depthImgCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                auto memoryAllocInfo = hDepthImgs[ i ].GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
                if ( false == hDepthImgMemory[ i ].Recreate( *appSurfaceVk->pNode, memoryAllocInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                if ( false == hDepthImgs[ i ].BindMemory( hDepthImgMemory[ i ], 0 ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                depthImgViewCreateInfo.image = hDepthImgs[ i ];
                if ( false == hDepthImgViews[ i ].Recreate( *appSurfaceVk->pNode, depthImgViewCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            VkAttachmentDescription colorDepthAttachments[ 2 ];
            InitializeStruct( colorDepthAttachments );

            colorDepthAttachments[ 0 ].format         = appSurfaceVk->Surface.eColorFormat;
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

            if ( false == hNkRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoNk ) ) {
                DebugBreak( );
                return false;
            }

            if ( false == hDbgRenderPass.Recreate( *appSurfaceVk->pNode, renderPassCreateInfoDbg ) ) {
                DebugBreak( );
                return false;
            }

            VkFramebufferCreateInfo framebufferCreateInfoNk;
            InitializeStruct( framebufferCreateInfoNk );
            framebufferCreateInfoNk.renderPass      = hNkRenderPass;
            framebufferCreateInfoNk.attachmentCount = 1;
            framebufferCreateInfoNk.width           = swapchain->ImgExtent.width;
            framebufferCreateInfoNk.height          = swapchain->ImgExtent.height;
            framebufferCreateInfoNk.layers          = 1;

            VkFramebufferCreateInfo framebufferCreateInfoDbg;
            InitializeStruct( framebufferCreateInfoDbg );
            framebufferCreateInfoDbg.renderPass      = hDbgRenderPass;
            framebufferCreateInfoDbg.attachmentCount = 2;
            framebufferCreateInfoDbg.width           = swapchain->ImgExtent.width;
            framebufferCreateInfoDbg.height          = swapchain->ImgExtent.height;
            framebufferCreateInfoDbg.layers          = 1;

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                VkImageView attachments[ 1 ] = {swapchain->hImgViews[ i ]};
                framebufferCreateInfoNk.pAttachments = attachments;

                if ( false == hNkFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfoNk ) ) {
                    DebugBreak( );
                    return false;
                }
            }

            for ( uint32_t i = 0; i < FrameCount; ++i ) {
                VkImageView attachments[ 2 ] = {swapchain->hImgViews[ i ], hDepthImgViews[ i ]};
                framebufferCreateInfoDbg.pAttachments = attachments;

                if ( false == hDbgFramebuffers[ i ].Recreate( *appSurfaceVk->pNode, framebufferCreateInfoDbg ) ) {
                    DebugBreak( );
                    return false;
                }
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
        pSceneRendererBase->Reset( Scenes[ 0 ], FrameIndex );

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

        pSkyboxRenderer->Render( pSkybox, &skyboxRenderParams );

        DebugRendererVk::RenderParametersVk renderParamsDbg;
        renderParamsDbg.dims[ 0 ]  = (float) width;
        renderParamsDbg.dims[ 1 ]  = (float) height;
        renderParamsDbg.scale[ 0 ] = 1;
        renderParamsDbg.scale[ 1 ] = 1;
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
        sceneRenderParameters.dims.x     = (float) width;
        sceneRenderParameters.dims.y     = (float) height;
        sceneRenderParameters.scale.x    = 1;
        sceneRenderParameters.scale.y    = 1;
        sceneRenderParameters.FrameIndex = FrameIndex;
        sceneRenderParameters.pCmdBuffer = cmdBuffer;
        sceneRenderParameters.pNode      = appSurfaceVk->pNode;
        sceneRenderParameters.ViewMatrix = frameData.viewMatrix;
        sceneRenderParameters.ProjMatrix = frameData.projectionMatrix;
        pSceneRendererBase->RenderScene( Scenes[ 0 ], &sceneRenderParameters );

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
        pSceneRendererBase->Flush( Scenes[0], FrameIndex );
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
