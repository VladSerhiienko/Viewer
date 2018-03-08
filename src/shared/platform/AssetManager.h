#pragma once

#include <map>
#include <vector>
#include <string>

namespace apemodeos {

    struct AssetFile {
        std::string FullPath;
    };

    class AssetManager {
    public:
        std::map< std::string, AssetFile > AssetFiles;
        void AddFilesFromDirectory( const std::string& InStorageDirectory, const std::vector< std::string >& InFilePatterns );
    };
}