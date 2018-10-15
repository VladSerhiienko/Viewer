
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>
#include <apemode/platform/AppInput.h>
#include <apemode/platform/AppSurface.h>

#include <apemode/vk/Swapchain.Vulkan.h>
#include <apemode/vk_ext/AppSurface.Vulkan.h>
#include <apemode/vk_ext/ImageUploader.Vulkan.h>
#include <apemode/vk_ext/SamplerManager.Vulkan.h>

#include <NuklearRendererVk.h>
#include <DebugRendererVk.h>
#include <SceneRendererVk.h>
#include <SceneUploaderVk.h>
#include <SkyboxRendererVk.h>

#include <viewer/Scene.h>
#include <viewer/Camera.h>
#include <viewer/CameraControllerInputMouseKeyboard.h>
#include <viewer/CameraControllerProjection.h>
#include <viewer/CameraControllerModelView.h>
#include <viewer/CameraControllerFreeLook.h>

#define kMaxFrames 3

namespace apemode {
namespace viewer {
namespace vk {

    class AppState;
    class AppContent;

    class ViewerShell {
    public:
        ViewerShell( );
        ~ViewerShell( );

        /* Returns true if initialization succeeded, false otherwise.
         */
        bool Initialize( const apemodevk::PlatformSurface* pPlatformSurface );

        /* Returns true if the app is running, false otherwise.
         */
        bool Update( float deltaSecs, const apemode::platform::AppInput* inputState, VkExtent2D extent );

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

        apemode::platform::shared::AssetManager mAssetManager;

        apemode::CameraProjectionController                       CamProjController;
        apemode::unique_ptr< apemode::CameraControllerInputBase > pCamInput      = nullptr;
        apemode::unique_ptr< apemode::CameraControllerBase >      pCamController = nullptr;

        apemodevk::AppSurface Surface;

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

        apemode::unique_ptr< NuklearRendererBase >         pNkRenderer;
        apemode::unique_ptr< SceneRendererBase >           pSceneRendererBase;
        apemode::unique_ptr< apemode::vk::Skybox >         pSkybox;
        apemode::unique_ptr< apemode::vk::SkyboxRenderer > pSkyboxRenderer;
        apemode::unique_ptr< apemode::vk::DebugRenderer >  pDebugRenderer;

        LoadedScene mLoadedScene;
    };

} // namespace vk
} // namespace viewer
} // namespace apemode