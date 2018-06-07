
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
        void WriteFeedback( EFeedbackType                                     eType,
                            const std::string&                                FullFilePath,
                            const ShaderCompiler::IMacroDefinitionCollection* Macros,
                            const void*                                       pContent, /* Txt or bin, @see EFeedbackType */
                            const void*                                       pContentEnd ) override;
    };
}

namespace apemode {
    class AppState;
    class AppContent;

    class ViewerApp : public apemode::AppBase {
        friend AppState;
        friend AppContent;

        float TotalSecs = 0.0f;

        uint32_t Width      = 0;
        uint32_t Height     = 0;
        uint32_t FrameCount = 0;
        uint32_t FrameIndex = 0;
        uint64_t FrameId    = 0;

        float    WorldRotationY = 0;
        XMFLOAT4 LightDirection;
        XMFLOAT4 LightColor;

        apemodeos::FileTracker          mFileTracker;
        apemodeos::AssetManager         mAssetManager;

        CameraProjectionController                       CamProjController;
        apemode::unique_ptr< CameraControllerInputBase > pCamInput      = nullptr;
        apemode::unique_ptr< CameraControllerBase >      pCamController = nullptr;

        uint32_t                                        BackbufferIndices[ kMaxFrames ] = {0};
        apemodevk::DescriptorPool                       DescriptorPool;
        apemodevk::THandle< VkCommandPool >             hCmdPool[ kMaxFrames ];
        apemodevk::THandle< VkCommandBuffer >           hCmdBuffers[ kMaxFrames ];
        apemodevk::THandle< VkSemaphore >               hPresentCompleteSemaphores[ kMaxFrames ];
        apemodevk::THandle< VkSemaphore >               hRenderCompleteSemaphores[ kMaxFrames ];
        apemodevk::THandle< VkRenderPass >              hNkRenderPass;
        apemodevk::THandle< VkFramebuffer >             hNkFramebuffers[ kMaxFrames ];
        apemodevk::THandle< VkRenderPass >              hDbgRenderPass;
        apemodevk::THandle< VkFramebuffer >             hDbgFramebuffers[ kMaxFrames ];
        apemodevk::THandle< apemodevk::ImageComposite > hDepthImgs[ kMaxFrames ];
        apemodevk::THandle< VkImageView >               hDepthImgViews[ kMaxFrames ];
        apemodevk::THandle< VkDeviceMemory >            hDepthImgMemory[ kMaxFrames ];

        NuklearRendererSdlBase*                      pNkRenderer        = nullptr;
        DebugRendererVk*                             pDebugRenderer     = nullptr;
        SceneRendererBase*                           pSceneRendererBase = nullptr;

        using SceneSourceData = std::pair< const apemodefb::SceneFb *, std::vector< uint8_t > >;

        UniqueScenePtr  pScene;
        SceneSourceData pSceneSource;

        apemode::unique_ptr< apemodevk::ShaderCompiler >       pShaderCompiler;
        apemode::unique_ptr< apemodevk::ShaderFileReader >     pShaderFileReader;
        apemode::unique_ptr< apemodevk::ShaderFeedbackWriter > pShaderFeedbackWriter;
        apemode::unique_ptr< apemodevk::Skybox >               pSkybox;
        apemode::unique_ptr< apemodevk::SkyboxRenderer >       pSkyboxRenderer;
        apemode::unique_ptr< apemodevk::SamplerManager >       pSamplerManager;
        apemodevk::unique_ptr< apemodevk::LoadedImage >        RadianceLoadedImg;
        apemodevk::unique_ptr< apemodevk::LoadedImage >        IrradianceLoadedImg;
        VkSampler                                              pRadianceCubeMapSampler   = VK_NULL_HANDLE;
        VkSampler                                              pIrradianceCubeMapSampler = VK_NULL_HANDLE;

    public:
        ViewerApp( );
        virtual ~ViewerApp( );

    public:
        bool                                           Initialize( ) override;
        apemode::unique_ptr< apemode::AppSurfaceBase > CreateAppSurface( ) override;
        const char*                                    GetAppName( ) override { return "Viewer"; }

    public:
        bool OnResized( );
        void OnFrameMove( ) override;
        void Update( float deltaSecs, apemode::Input const& inputState ) override;
        bool IsRunning( ) override;
    };
}