
#include <Swapchain.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>
#include <ImageLoader.Vulkan.h>
#include <SamplerManager.Vulkan.h>

#include <AppBase.h>
#include <AppState.h>

#include <AppSurfaceSdlVk.h>
#include <NuklearSdlVk.h>
#include <DebugRendererVk.h>
#include <SceneRendererVk.h>
#include <SkyboxRendererVk.h>

#include <Input.h>
#include <Scene.h>
#include <Camera.h>
#include <AssetManager.h>
#include <FileTracker.h>
#include <CameraControllerInputMouseKeyboard.h>
#include <CameraControllerProjection.h>
#include <CameraControllerModelView.h>
#include <CameraControllerFreeLook.h>

#define kMaxFrames 3

namespace apemodevk {

    class ShaderFileReader : public ShaderCompiler::IShaderFileReader {
    public:
        apemodeos::IAssetManager* mAssetManager;

        bool ReadShaderTxtFile( const std::string& FilePath,
                                std::string&       OutFileFullPath,
                                std::string&       OutFileContent ) override;
    };

    class ShaderFeedbackWriter : public ShaderCompiler::IShaderFeedbackWriter {
    public:
        void WriteFeedback( EFeedbackType                     eType,
                            const std::string&                FullFilePath,
                            const std::vector< std::string >& Macros,
                            const void*                       pContent, /* Txt or bin, @see EFeedbackType */
                            const void*                       pContentEnd ) override;
    };
}

namespace apemode {
    class AppState;
    class AppContent;

    class ViewerApp : public apemode::AppBase {
        friend AppState;
        friend AppContent;

        float    totalSecs = 0.0f;
        nk_color diffColor;
        nk_color specColor;
        uint32_t width      = 0;
        uint32_t height     = 0;
        uint32_t resetFlags = 0;
        uint32_t envId      = 0;
        uint32_t sceneId    = 0;
        uint32_t maskId     = 0;
        uint32_t FrameCount = 0;
        uint32_t FrameIndex = 0;
        uint64_t FrameId    = 0;

        apemodeos::FileTracker          mFileTracker;
        apemodeos::AssetManager         mAssetManager;

        CameraProjectionController                   CamProjController;
        std::unique_ptr< CameraControllerInputBase > pCamInput          = nullptr;
        std::unique_ptr< CameraControllerBase >      pCamController     = nullptr;

        uint32_t                                          BackbufferIndices[ kMaxFrames ] = {0};
        apemodevk::DescriptorPool                         DescPool;
        apemodevk::TDispatchableHandle< VkCommandPool >   hCmdPool[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkCommandBuffer > hCmdBuffers[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkSemaphore >     hPresentCompleteSemaphores[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkSemaphore >     hRenderCompleteSemaphores[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkRenderPass >    hNkRenderPass;
        apemodevk::TDispatchableHandle< VkFramebuffer >   hNkFramebuffers[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkRenderPass >    hDbgRenderPass;
        apemodevk::TDispatchableHandle< VkFramebuffer >   hDbgFramebuffers[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkImage >         hDepthImgs[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkImageView >     hDepthImgViews[ kMaxFrames ];
        apemodevk::TDispatchableHandle< VkDeviceMemory >  hDepthImgMemory[ kMaxFrames ];


        NuklearRendererSdlBase*                      pNkRenderer        = nullptr;
        DebugRendererVk*                             pDebugRenderer     = nullptr;
        SceneRendererBase*                           pSceneRendererBase = nullptr;
        std::vector< Scene* >                        Scenes;


        std::unique_ptr< apemodevk::ShaderCompiler >       pShaderCompiler;
        std::unique_ptr< apemodevk::ShaderFileReader >     pShaderFileReader;
        std::unique_ptr< apemodevk::ShaderFeedbackWriter > pShaderFeedbackWriter;
        std::unique_ptr< apemodevk::Skybox >               pSkybox;
        std::unique_ptr< apemodevk::SkyboxRenderer >       pSkyboxRenderer;
        std::unique_ptr< apemodevk::LoadedImage >          pLoadedDDS;
        std::unique_ptr< apemodevk::SamplerManager >       pSamplerManager;

    public:
        ViewerApp( );
        virtual ~ViewerApp( );

    public:
        virtual bool                     Initialize( ) override;
        virtual apemode::AppSurfaceBase* CreateAppSurface( ) override;

    public:
        bool         OnResized( );
        virtual void OnFrameMove( ) override;
        virtual void Update( float deltaSecs, apemode::Input const& inputState ) override;
        virtual bool IsRunning( ) override;
    };
}