#include <CityHash.Vulkan.h>
#include <SamplerManager.Vulkan.h>

apemodevk::SamplerManager::~SamplerManager( ) {
    Release( );
}

bool apemodevk::SamplerManager::Recreate( apemodevk::GraphicsDevice* pInNode ) {
    pNode = pInNode;
    return ( nullptr != pNode );
}

void apemodevk::SamplerManager::Release( ) {
    apemodevk_memory_allocation_scope;

    if ( pNode ) {
        THandle< VkSampler > hSampler;
        for ( auto& storedSampler : StoredSamplers ) {
            hSampler.Handle                 = storedSampler.second.pSampler;
            hSampler.Deleter.hLogicalDevice = pNode->hLogicalDevice;
            hSampler.Destroy( );
        }

        StoredSamplers.clear( );
        pNode = nullptr;
    }
}

VkSampler apemodevk::SamplerManager::GetSampler( const VkSamplerCreateInfo& samplerCreateInfo ) {
    apemodevk_memory_allocation_scope;

    const auto samplerId = apemodevk::CityHash64( samplerCreateInfo );
    const auto samplerIt = StoredSamplers.find( samplerId );

    if ( samplerIt != StoredSamplers.end( ) )
        return samplerIt->second.pSampler;

    THandle< VkSampler > hSampler;
    if ( hSampler.Recreate( *pNode, samplerCreateInfo ) ) {

        StoredSampler storedSampler;
        storedSampler.pSampler          = hSampler;
        storedSampler.SamplerCreateInfo = samplerCreateInfo;

        StoredSamplers[ samplerId ] = std::move( storedSampler );
        return hSampler.Release( );
    }

    platform::LogFmt( platform::LogLevel::Err, "Failed to create sampler." );
    platform::DebugBreak( );
    return VK_NULL_HANDLE;
}
