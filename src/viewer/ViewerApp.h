
#include <Swapchain.Vulkan.h>
#include <ImageUploader.Vulkan.h>
#include <SamplerManager.Vulkan.h>

#include <AppBase.h>
#include <AppState.h>

#include <AppSurfaceSdlVk.h>
#include <NuklearRendererVk.h>
#include <DebugRendererVk.h>
#include <SceneRendererVk.h>
#include <SceneUploaderVk.h>
#include <SkyboxRendererVk.h>

#include <Input.h>
#include <Scene.h>
#include <Camera.h>
#include <AssetManager.h>
#include <CameraControllerInputMouseKeyboard.h>
#include <CameraControllerProjection.h>
#include <CameraControllerModelView.h>
#include <CameraControllerFreeLook.h>

#define kMaxFrames 3

namespace apemode {
    class AppState;
    class AppContent;

    class ViewerApp {
    public:
        ViewerApp( );
        ~ViewerApp( );

        /* Returns true if initialization succeeded, false otherwise.
         */
        bool Initialize( const apemode::PlatformSurface* pPlatformSurface );

        /* Returns true if the app is running, false otherwise.
         */
        bool Update( float deltaSecs, const apemode::Input& inputState, VkExtent2D extent );

    protected:
        bool OnResized( );

    private:
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

        apemodeos::AssetManager mAssetManager;

        CameraProjectionController                       CamProjController;
        apemode::unique_ptr< CameraControllerInputBase > pCamInput      = nullptr;
        apemode::unique_ptr< CameraControllerBase >      pCamController = nullptr;

        AppSurfaceSdlVk AppSurface;

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

        apemode::unique_ptr< apemodevk::SamplerManager >  pSamplerManager;
        apemodevk::unique_ptr< apemodevk::UploadedImage > RadianceImg;
        apemodevk::unique_ptr< apemodevk::UploadedImage > IrradianceImg;
        VkSampler                                         pRadianceCubeMapSampler   = VK_NULL_HANDLE;
        VkSampler                                         pIrradianceCubeMapSampler = VK_NULL_HANDLE;

        apemode::unique_ptr< NuklearRendererSdlBase > pNkRenderer;
        apemode::unique_ptr< SceneRendererBase >      pSceneRendererBase;
        apemode::unique_ptr< vk::Skybox >             pSkybox;
        apemode::unique_ptr< vk::SkyboxRenderer >     pSkyboxRenderer;
        apemode::unique_ptr< vk::DebugRenderer >      pDebugRenderer;

        LoadedScene mLoadedScene;
    };
}