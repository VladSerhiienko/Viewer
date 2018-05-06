#include <CityHash.h>
#include <SamplerManager.Vulkan.h>

bool apemodevk::SamplerManager::Recreate( apemodevk::GraphicsDevice* pInNode ) {
    pNode = pInNode;
    return nullptr != pNode;
}

void apemodevk::SamplerManager::Release( apemodevk::GraphicsDevice* pNode ) {
    THandle< VkSampler > hSampler;
    for ( auto& storedSampler : StoredSamplers ) {
        hSampler.Handle = storedSampler.pSampler;
        hSampler.Deleter.hLogicalDevice = pNode->hLogicalDevice;
        hSampler.Destroy( );
    }
}

constexpr uint32_t kInvalidSamplerIndex = 0xffffffff;

uint32_t apemodevk::SamplerManager::GetSamplerIndex( const VkSamplerCreateInfo& samplerCreateInfo ) {
    const auto samplerHash = apemode::CityHash64( samplerCreateInfo );
    const auto samplerIt = std::find_if( StoredSamplers.begin( ),
                                         StoredSamplers.end( ),
                                         [samplerHash]( const StoredSampler& s ) { return samplerHash == s.Hash; } );

    if ( samplerIt != StoredSamplers.end( ) )
        return (uint32_t) std::distance( StoredSamplers.begin( ), samplerIt );

    THandle< VkSampler > hSampler;
    if ( hSampler.Recreate( *pNode, samplerCreateInfo ) ) {
        const size_t samplerIndex = StoredSamplers.size( );

        StoredSampler storedSampler;
        storedSampler.Hash              = samplerHash;
        storedSampler.pSampler          = hSampler.Release( );
        storedSampler.SamplerCreateInfo = samplerCreateInfo;
        StoredSamplers.push_back( storedSampler );

        return static_cast< uint32_t >( samplerIndex );
    }

    platform::DebugTrace( platform::LogLevel::Err, "Failed to create sampler." );
    platform::DebugBreak( );
    return kInvalidSamplerIndex;
}

bool apemodevk::SamplerManager::IsSamplerIndexValid( const uint32_t samplerIndex ) {
    return kInvalidSamplerIndex != samplerIndex;
}
