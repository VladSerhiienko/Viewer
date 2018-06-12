#include "AssetManager.h"

#include <FileReader.h>
#include <AppState.h>
#include <tinydir.h>

#include <stdlib.h>
#include <regex>

const char* apemodeos::AssetFile::GetName( ) const {
    return RelPath.c_str( );
}

const char* apemodeos::AssetFile::GetId( ) const {
    return FullPath.c_str( );
}

apemodeos::AssetContentBuffer apemodeos::AssetFile::GetContentAsTextBuffer( ) const {
    return FileReader( ).ReadTxtFile( FullPath );
}

apemodeos::AssetContentBuffer apemodeos::AssetFile::GetContentAsBinaryBuffer( ) const {
    return FileReader( ).ReadBinFile( FullPath );
}

const apemodeos::IAsset* apemodeos::AssetManager::GetAsset( const std::string& InAssetName ) const {
    auto assetIt = AssetFiles.find( InAssetName );
    return assetIt == AssetFiles.end( ) ? 0 : &assetIt->second;
}

std::string ToCanonicalAbsolutPath( const std::string & relativePath );

template < typename TFileCallback >
void ProcessFiles( TFileCallback callback, const tinydir_dir& dir, bool r ) {
    apemode::LogInfo( "AssetManager: Entering folder: {}", dir.path );

    for ( size_t i = 0; i < dir.n_files; i++ ) {
        tinydir_file file;
        if ( tinydir_readfile_n( &dir, &file, i ) != -1 ) {
            if ( file.is_reg ) {
                apemode::LogInfo( "AssetManager: Processing file: {}", file.path );
                callback( file.path, file );
            } else {
                if ( r && file.is_dir && ( strcmp( file.name, "." ) != 0 ) && ( strcmp( file.name, ".." ) != 0 ) ) {
                    tinydir_dir subdir;
                    if ( tinydir_open_sorted( &subdir, file.path ) != -1 ) {
                        ProcessFiles( callback, subdir, r );
                        tinydir_close( &subdir );
                    }
                }
            }
        }
    }
}

void apemodeos::AssetManager::AddFilesFromDirectory( const std::string&                InStorageDirectory,
                                                     const std::vector< std::string >& InFilePatterns ) {
    apemode_memory_allocation_scope;
    std::string storageDirectory = InStorageDirectory;

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

    tinydir_dir dir;
    std::string storageDirectoryFull = ToCanonicalAbsolutPath( storageDirectory );
    if ( !storageDirectoryFull.empty( ) && tinydir_open_sorted( &dir, storageDirectoryFull.c_str( ) ) != -1 ) {

        auto addFileFn = [&]( const std::string& filePath, tinydir_file file ) {

            /* Check if file matches the patterns */
            bool matches = InFilePatterns.empty( );
            for ( const auto& f : InFilePatterns ) {
                if ( std::regex_match( filePath, std::regex( f ) ) ) {
                    matches = true;
                    break;
                }
            }

            if ( matches ) {
                const std::string fullPath = ToCanonicalAbsolutPath( filePath );
                const std::string relativePath = fullPath.substr( storageDirectoryFull.size( ) + ( storageDirectoryFull.back( ) != '/' ) );

                auto assetIt = AssetFiles.find( relativePath );
                if ( assetIt == AssetFiles.end( ) ) {
                    apemode::LogInfo( "AssetManager: Added: {}", fullPath );
                } else {
                    apemode::LogWarn( "AssetManager: Replaced: {} -> {}", assetIt->second.FullPath, fullPath );
                }

                AssetFiles[ relativePath ].RelPath  = relativePath;
                AssetFiles[ relativePath ].FullPath = fullPath;
            }
        };

        ProcessFiles( addFileFn, dir, addSubDirectories );
        tinydir_close( &dir );
    }
}
