#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    struct StoredSampler {
        uint64_t            Hash     = 0;
        VkSampler           pSampler = nullptr;
        VkSamplerCreateInfo SamplerCreateInfo;
    };

    class SamplerManager {
    public:
        apemodevk::GraphicsDevice*         pNode;
        apemodevk::vector< StoredSampler > StoredSamplers;

        bool     Recreate( apemodevk::GraphicsDevice* pNode );
        void     Release( apemodevk::GraphicsDevice* pNode );
        uint32_t GetSamplerIndex( const VkSamplerCreateInfo& SamplerCreateInfo );

        static bool IsSamplerIndexValid( uint32_t samplerIndex );
    };
}