#pragma once

#include <apemode/platform/IAssetManager.h>

#include <stdint.h>
#include <atomic>
#include <map>
#include <string>

namespace apemode {
namespace platform {
namespace shared {

struct AssetManager;

struct AssetFile : IAsset {
    static constexpr uint64_t kVersionDeleted = ~0;

    virtual ~AssetFile( ) = default;

    const char*                GetName( ) const override;
    const char*                GetId( ) const override;
    apemode::vector< uint8_t > GetContentAsTextBuffer( ) const override;
    apemode::vector< uint8_t > GetContentAsBinaryBuffer( ) const override;
    uint64_t                   GetCurrentVersion( ) const override;
    void                       SetCurrentVersion( uint64_t lastTimeModified ) override;

    void SetName( const char* pszAssetName );
    void SetId( const char* pszAssetPath );

protected:
    std::string             Name;
    std::string             Path;
    std::atomic< uint64_t > LastTimeModified;
};

/* The class is designed to be thread-safe.
 */
struct AssetManager : IAssetManager {
    AssetManager();
    virtual ~AssetManager( ) = default;

    const IAsset* Acquire( const char* pszAssetName ) const override;
    void          Release( const IAsset* pAsset ) const override;

    void AddAsset( const char* pszAssetName, const char* pszAssetPath );
    void UpdateAssets( const char* pszFolderPath, const char** ppszFilePatterns, size_t filePatternCount );

protected:
    struct AssetFileItem {
        mutable std::atomic< uint32_t > UseCount;
        AssetFile                       Asset;
    };

    std::atomic< bool >                    InUse;
    std::map< std::string, AssetFileItem > AssetFiles;
};

//
// Set of functions and classes to interact with file system.
//

bool DirectoryExists( const char* pszPath );
bool FileExists( const char* pszPath );
// ...

/* Currently only reads files either as a text or buffer.
 */
class FileReader {
public:
    apemode::vector< uint8_t > ReadBinFile( const char* pszFilePath ); /* Returns the content of the file. */
    apemode::vector< uint8_t > ReadTxtFile( const char* pszFilePath ); /* Returns the content of the file. */
};

} // namespace shared
} // namespace platform
} // namespace apemode
