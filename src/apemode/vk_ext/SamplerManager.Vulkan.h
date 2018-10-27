#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>

namespace apemodevk {
    struct APEMODEVK_API StoredSampler {
        VkSampler           pSampler = nullptr;
        VkSamplerCreateInfo SamplerCreateInfo;
    };

    class APEMODEVK_API SamplerManager {
    public:
        apemodevk::GraphicsDevice*                       pNode;
        apemodevk::vector_map< uint64_t, StoredSampler > StoredSamplers;

        ~SamplerManager( );
        bool      Recreate( apemodevk::GraphicsDevice* pNode );
        void      Release( );
        VkSampler GetSampler( const VkSamplerCreateInfo& SamplerCreateInfo );

    };
}