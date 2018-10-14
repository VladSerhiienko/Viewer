#pragma once

#include <apemode/vk/GraphicsDevice.Vulkan.h>

namespace apemodevk {
    struct StoredSampler {
        VkSampler           pSampler = nullptr;
        VkSamplerCreateInfo SamplerCreateInfo;
    };

    class SamplerManager {
    public:
        apemodevk::GraphicsDevice*                       pNode;
        apemodevk::vector_map< uint64_t, StoredSampler > StoredSamplers;

        bool      Recreate( apemodevk::GraphicsDevice* pNode );
        void      Release( apemodevk::GraphicsDevice* pNode );
        VkSampler GetSampler( const VkSamplerCreateInfo& SamplerCreateInfo );
    };
}