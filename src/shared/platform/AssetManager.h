#pragma once

#include <map>
#include <stdint.h>

#include <IAssetManager.h>

namespace apemodeos {

    struct AssetFile : IAsset  {
        std::string Name;
        std::string Path;

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

        virtual ~AssetManager( ) { }
        virtual const IAsset* GetAsset( const std::string& InAssetName ) const override;

        void AddFilesFromDirectory( const std::string& InStorageDirectory, const std::vector< std::string >& InFilePatterns );
    };
}