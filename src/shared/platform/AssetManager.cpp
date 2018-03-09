#include "AssetManager.h"

#include <FileReader.h>
#include <AppState.h>

#include <fstream>
#include <iterator>
#include <set>
#include <regex>

#if defined( __clang__ ) || defined( __GNUC__ )
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
namespace std {
    namespace filesystem = std::experimental::filesystem::v1;
}

const std::string& apemodeos::AssetFile::GetName( ) const {
    return RelPath;
}

const std::string& apemodeos::AssetFile::GetId( ) const {
    return FullPath;
}

std::string apemodeos::AssetFile::AsTxt( ) const {
    return FileReader( ).ReadTxtFile( FullPath );
}

std::vector< uint8_t > apemodeos::AssetFile::AsBin( ) const {
    return FileReader( ).ReadBinFile( FullPath );
}

inline std::string ToCanonicalAbsolutPath( std::string InRelativePath ) {
    InRelativePath = std::filesystem::canonical( std::filesystem::absolute( InRelativePath ) ).string( );
    std::replace( InRelativePath.begin( ), InRelativePath.end( ), '\\', '/' );
    return InRelativePath;
}

const apemodeos::IAsset* apemodeos::AssetManager::GetAsset( const std::string& InAssetName ) const {
    auto assetIt = AssetFiles.find( InAssetName );
    return assetIt == AssetFiles.end( ) ? 0 : &assetIt->second;
}

void apemodeos::AssetManager::AddFilesFromDirectory( const std::string&                InStorageDirectory,
                                                     const std::vector< std::string >& InFilePatterns ) {
    auto appState = apemode::AppState::Get( );

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

    if ( std::filesystem::exists( storageDirectory ) ) {

        std::string storageDirectoryFull = ToCanonicalAbsolutPath( storageDirectory );

        auto addFileFn = [&]( const std::filesystem::path& filePath ) {
            if ( std::filesystem::is_regular_file( filePath ) ) {

                /* Check if file matches the patterns */

                bool matches = InFilePatterns.empty( );
                for ( const auto& f : InFilePatterns ) {
                    if ( std::regex_match( filePath.string( ), std::regex( f ) ) ) {
                        matches = true;
                        break;
                    }
                }

                if ( matches ) {
                    std::string fullPath = ToCanonicalAbsolutPath( filePath.string( ) );
                    std::string relativePath = fullPath.substr( storageDirectoryFull.size() + 1 );

                    auto assetIt = AssetFiles.find( relativePath );
                    if ( assetIt == AssetFiles.end() ) {
                        appState->Logger->info( "AssetManager: Added: {}", fullPath );
                    } else {
                        appState->Logger->warn( "AssetManager: Replaced: {} -> {}", assetIt->second.FullPath, fullPath );
                    }

                    AssetFiles[ relativePath ].RelPath = relativePath;
                    AssetFiles[ relativePath ].FullPath = fullPath;
                }
            }
        };

        if ( addSubDirectories ) {
            for ( const auto& fileOrFolderPath : std::filesystem::recursive_directory_iterator( storageDirectoryFull ) )
                addFileFn( fileOrFolderPath.path( ) );
        } else {
            for ( const auto& fileOrFolderPath : std::filesystem::directory_iterator( storageDirectoryFull ) )
                addFileFn( fileOrFolderPath.path( ) );
        }
    }
}
