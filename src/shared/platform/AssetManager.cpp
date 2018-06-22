#include "AssetManager.h"

#include <FileReader.h>
#include <AppState.h>
#include <tinydir.h>

#include <stdlib.h>
#include <regex>

const char* apemodeos::AssetFile::GetName( ) const {
    return Name.c_str( );
}

const char* apemodeos::AssetFile::GetId( ) const {
    return Path.c_str( );
}

apemodeos::AssetContentBuffer apemodeos::AssetFile::GetContentAsTextBuffer( ) const {
    apemode_memory_allocation_scope;
    return std::move( FileReader( ).ReadTxtFile( Path.c_str( ) ) );
}

apemodeos::AssetContentBuffer apemodeos::AssetFile::GetContentAsBinaryBuffer( ) const {
    apemode_memory_allocation_scope;
    return std::move( FileReader( ).ReadBinFile( Path.c_str( ) ) );
}

uint64_t apemodeos::AssetFile::GetCurrentVersion( ) const {
    return LastTimeModified.load(  );
}

void apemodeos::AssetFile::SetName( const char* pszAssetName ) {
    Name = pszAssetName;
}

void apemodeos::AssetFile::SetId( const char* pszAssetPath ) {
    Path = pszAssetPath;
}

void apemodeos::AssetFile::SetCurrentVersion( uint64_t version ) {
    LastTimeModified.store( version );
}

const apemodeos::IAsset* apemodeos::AssetManager::Acquire( const char* pszAssetName ) const {
    auto assetIt = AssetFiles.find( pszAssetName );

    if ( assetIt == AssetFiles.end( ) ) {
        return 0;
    }

    const apemodeos::AssetFile* pAsset = &assetIt->second.Asset;
    std::atomic_fetch_add( &assetIt->second.UseCount, uint32_t( 1 ) );
    return pAsset;
}

void apemodeos::AssetManager::Release( const apemodeos::IAsset* pAsset ) const {
    if ( const AssetFile* pAssetFile = static_cast< const AssetFile* >( pAsset ) ) {
        auto assetIt = AssetFiles.find( pAssetFile->GetName( ) );
        if ( assetIt != AssetFiles.end( ) ) {
            std::atomic_fetch_sub( &assetIt->second.UseCount, uint32_t( 1 ) );
        }
    }
}

std::string ToCanonicalAbsolutPath( const char * pszRelativePath );
uint64_t GetLastModifiedTime( const char * pszFilePath );

template < typename TFileCallback >
void ProcessFiles( TFileCallback callback, const tinydir_dir& dir, bool r ) {
    apemode_memory_allocation_scope;

    apemode::LogInfo( "AssetManager: Entering folder: {}", dir.path );

    for ( size_t i = 0; i < dir.n_files; i++ ) {
        tinydir_file file;
        if ( tinydir_readfile_n( &dir, &file, i ) != -1 ) {
            if ( file.is_reg ) {
                apemode::LogInfo( "AssetManager: Processing file: {}", file.path );
                callback( file.path, file );
            } else if ( r && file.is_dir && ( strcmp( file.name, "." ) != 0 ) && ( strcmp( file.name, ".." ) != 0 ) ) {
                tinydir_dir subdir;
                if ( tinydir_open_sorted( &subdir, file.path ) != -1 ) {
                    ProcessFiles( callback, subdir, r );
                    tinydir_close( &subdir );
                }
            }
        }
    }
}

void apemodeos::AssetManager::UpdateAssets( const char*  pszFolderPath,
                                            const char** ppszFilePatterns,
                                            size_t       filePatternCount ) {
    apemode_memory_allocation_scope;

    /* Protect asset files, no need to lock the function. */

    if ( std::atomic_fetch_add( &UseCount, uint32_t( 1 ) ) > 1 ) {
        std::atomic_fetch_sub( &UseCount, uint32_t( 1 ) );
        return;
    }

    /* Process existing assets. */

    auto assetIt = AssetFiles.begin();
    for ( ; assetIt != AssetFiles.end( ); ++assetIt ) {

        /* Check if the file, that is referenced by the asset item exists. */

        const bool bAssetExists = apemodeos::FileExists( assetIt->second.Asset.GetId() );

        /* File was marked as deleted, destroy asset item. */

        if ( ( AssetFile::kVersionDeleted == assetIt->second.Asset.GetCurrentVersion( ) ) && !bAssetExists ) {
            if ( ( !std::atomic_load( &assetIt->second.UseCount ) ) ) {
                apemode::LogInfo( "AssetManager: Deleted file: {}", assetIt->second.Asset.GetId( ) );
                assetIt = AssetFiles.erase( assetIt );
                continue;
            }

            ++assetIt;
            continue;
        }

        /* File was deleted, schedule the asset for destruction. */

        if ( ( AssetFile::kVersionDeleted != assetIt->second.Asset.GetCurrentVersion( ) ) && !bAssetExists ) {
            apemode::LogInfo( "AssetManager: Marked file as deleted: {}", assetIt->second.Asset.GetId( ) );
            assetIt->second.Asset.SetCurrentVersion( AssetFile::kVersionDeleted );
            ++assetIt;
            continue;
        }
    }

    /* Process storage folder argument */

    std::string storageDirectory = pszFolderPath;

    if ( storageDirectory.empty( ) ) {
        storageDirectory = "./";
    }

    bool addSubDirectories = false;
    if ( storageDirectory.size( ) > 4 ) {

        const size_t ds = storageDirectory.size( );
        addSubDirectories |= storageDirectory.compare( ds - 3, 3, "\\**" ) == 0;
        addSubDirectories |= storageDirectory.compare( ds - 3, 3, "/**"  ) == 0;

        if ( addSubDirectories ) {
            storageDirectory = storageDirectory.substr( 0, storageDirectory.size( ) - 2 );
        }
    }

    /* Update versions of the existing asset items. */

    tinydir_dir dir;
    std::string storageDirectoryFull = ToCanonicalAbsolutPath( storageDirectory.c_str( ) );
    if ( !storageDirectoryFull.empty( ) && tinydir_open_sorted( &dir, storageDirectoryFull.c_str( ) ) != -1 ) {

        auto addFileFn = [&]( const char * pszFilePath, tinydir_file file ) {

            /* Check if file matches the patterns */
            bool bMatches = ( filePatternCount == 0 );

            for ( size_t i = 0; i < filePatternCount; ++i ) {
                if ( std::regex_match( pszFilePath, std::regex( ppszFilePatterns[ i ] ) ) ) {
                    bMatches = true;
                    break;
                }
            }

            if ( bMatches ) {
                const std::string fullPath = ToCanonicalAbsolutPath( pszFilePath );
                const std::string relativePath = fullPath.substr( storageDirectoryFull.size( ) + ( storageDirectoryFull.back( ) != '/' ) );

                apemodeos::AssetFile* pAsset = nullptr;

                auto assetIt = AssetFiles.find( relativePath );
                if ( assetIt == AssetFiles.end( ) ) {

                    /* Create a new file asset. */

                    auto &assetItem = AssetFiles[ relativePath ];

                    pAsset = &assetItem.Asset;
                    pAsset->SetName( relativePath.c_str( ) );
                    pAsset->SetId( fullPath.c_str( ) );

                    apemode::LogInfo( "AssetManager: Added file: {}", fullPath );
                } else {
                    pAsset = &assetIt->second.Asset;
                }

                const uint64_t lastWriteTime = GetLastModifiedTime( pszFilePath );

                if ( pAsset && ( pAsset->GetCurrentVersion( ) != lastWriteTime ) ) {

                    /* Update the asset version. */

                    apemode::LogInfo( "AssetManager: Updated file version: {}", fullPath );
                    pAsset->SetCurrentVersion( lastWriteTime );
                }
            }
        };

        ProcessFiles( addFileFn, dir, addSubDirectories );
        tinydir_close( &dir );
    }

    /* Unlock. */

    std::atomic_fetch_sub( &UseCount, uint32_t( 1 ) );
}
