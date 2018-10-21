
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>
#include <apemode/platform/AppInput.h>
#include <apemode/platform/AppSurface.h>
#include <apemode/platform/Stopwatch.h>

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

namespace apemode {
namespace viewer {
namespace vk {

    class AppState;
    class GraphicsLogger;
    class GraphicsAllocator;

    class ViewerShell {
    public:

        struct Frame {
            uint32_t                                        BackbufferIndex;
            apemodevk::THandle< VkSemaphore >               hPresentCompleteSemaphore;
            apemodevk::THandle< VkSemaphore >               hRenderCompleteSemaphore;
            apemodevk::THandle< VkFramebuffer >             hNkFramebuffer;
            apemodevk::THandle< VkFramebuffer >             hDbgFramebuffer;
            apemodevk::THandle< apemodevk::ImageComposite > hDepthImg;
            apemodevk::THandle< VkImageView >               hDepthImgView;
            apemodevk::THandle< VkDeviceMemory >            hDepthImgMemory;
        };

        ViewerShell( );
        ~ViewerShell( );

        /* Returns true if initialization succeeded, false otherwise.
         */
        bool Initialize( const apemodevk::PlatformSurface* pPlatformSurface );

        /* Returns true if the app is running, false otherwise.
         */
        bool Update( VkExtent2D currentExtent, const apemode::platform::AppInput* inputState );

        bool OnResized( );

        void UpdateTime();
        void UpdateCamera(const apemode::platform::AppInput* inputState);
        void UpdateUI(const VkExtent2D currentExtent, const apemode::platform::AppInput* pAppInput);
        void Populate(Frame * pCurrentFrame, Frame * pSwapchainFrame, VkCommandBuffer pCmdBuffer);

    private:
        friend AppState;

        float    TotalSecs  = 0.0f;
        float    DeltaSecs  = 0.0f;
        uint32_t FrameCount = 0;
        uint32_t FrameIndex = 0;
        uint64_t FrameId    = 0;

        float    WorldRotationY = 0;
        XMFLOAT4 LightDirection;
        XMFLOAT4 LightColor;

        apemode::platform::Stopwatch                              Stopwatch;
        apemode::platform::shared::AssetManager                   FileAssetManager;
        apemode::CameraProjectionController                       CamProjController;
        apemode::unique_ptr< apemode::CameraControllerInputBase > pCamInput;
        apemode::unique_ptr< apemode::CameraControllerBase >      pCamController;
        apemode::unique_ptr< GraphicsLogger >                     Logger;
        apemode::unique_ptr< GraphicsAllocator >                  Allocator;
        apemodevk::AppSurface                                     Surface;
        apemodevk::DescriptorPool                                 DescriptorPool;
        apemodevk::THandle< VkRenderPass >                        hNkRenderPass;
        apemodevk::THandle< VkRenderPass >                        hDbgRenderPass;
        apemodevk::vector< Frame >                                Frames;
        apemode::unique_ptr< apemodevk::SamplerManager >          pSamplerManager;
        apemodevk::unique_ptr< apemodevk::UploadedImage >         RadianceImg;
        apemodevk::unique_ptr< apemodevk::UploadedImage >         IrradianceImg;
        VkSampler                                                 pRadianceCubeMapSampler   = VK_NULL_HANDLE;
        VkSampler                                                 pIrradianceCubeMapSampler = VK_NULL_HANDLE;
        apemode::unique_ptr< NuklearRendererBase >                pNkRenderer;
        apemode::unique_ptr< SceneRendererBase >                  pSceneRendererBase;
        apemode::unique_ptr< apemode::vk::Skybox >                pSkybox;
        apemode::unique_ptr< apemode::vk::SkyboxRenderer >        pSkyboxRenderer;
        apemode::unique_ptr< apemode::vk::DebugRenderer >         pDebugRenderer;
        LoadedScene                                               mLoadedScene;
    };

} // namespace vk
} // namespace viewer
} // namespace apemode
