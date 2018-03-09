#pragma once

#include <map>
#include <stdint.h>

#include <IAssetManager.h>

namespace apemodeos {

    struct AssetFile : IAsset  {
        std::string RelPath;
        std::string FullPath;

        virtual ~AssetFile( ) { }
        virtual const std::string&     GetName( ) const override;
        virtual const std::string&     GetId( ) const override;
        virtual std::string            AsTxt( ) const override;
        virtual std::vector< uint8_t > AsBin( ) const override;
    };

    struct AssetManager : IAssetManager {
        std::map< std::string, AssetFile > AssetFiles;

        virtual ~AssetManager( ) { }
        virtual const IAsset* GetAsset( const std::string& InAssetName ) const override;

        void AddFilesFromDirectory( const std::string& InStorageDirectory, const std::vector< std::string >& InFilePatterns );
    };
}