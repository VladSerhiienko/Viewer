#pragma once

#include <map>
#include <stdint.h>

#include <IAssetManager.h>

namespace apemodeos {

    struct AssetFile : IAsset {
        std::string Name;
        std::string Path;
        uint64_t    LastTimeModified = 0;

        virtual ~AssetFile( ) = default;

        const char*        GetName( ) const                  override;
        const char*        GetId( ) const                    override;
        AssetContentBuffer GetContentAsTextBuffer( ) const   override;
        AssetContentBuffer GetContentAsBinaryBuffer( ) const override;
        uint64_t           GetCurrentVersion( ) const        override;
        void               SetCurrentVersion( uint64_t )     override;
    };

    struct AssetManager : IAssetManager {
        std::map< std::string, AssetFile > AssetFiles;

        virtual ~AssetManager( ) = default;

        const IAsset* GetAsset( const char* InAssetName ) const override;

        void AddFilesFromDirectory( const char* InStorageDirectory, const char** ppszFilePatterns, size_t filePatternCount );
    };
}