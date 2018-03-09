#include "FileTracker.h"

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

#include <fstream>
#include <iterator>

#include <AppState.h>

std::string apemodeos::CurrentDirectory( ) {
    return std::filesystem::current_path( ).string( );
}

std::string apemodeos::GetFileName( const std::string& filePath ) {
    return std::filesystem::path( filePath ).filename( ).string( );
}

bool apemodeos::PathExists( const std::string& path ) {
    return std::filesystem::exists( path );
}

bool apemodeos::DirectoryExists( const std::string& directoryPath ) {
    return std::filesystem::is_directory( directoryPath );
}

bool apemodeos::FileExists( const std::string& filePath ) {
    return std::filesystem::is_regular_file( filePath );
}

std::string apemodeos::ReplaceSlashes( std::string path ) {
    std::replace( path.begin( ), path.end( ), '\\', '/' );
    return path;
}

std::string apemodeos::RealPath( std::string path ) {
    return std::filesystem::canonical( path ).string( );
}

std::string apemodeos::ResolveFullPath( const std::string& path ) {
    return ReplaceSlashes( RealPath( std::filesystem::absolute( path ).string( ) ) );
}

bool apemodeos::FileTracker::ScanDirectory( std::string storageDirectory, bool bRemoveDeletedFiles ) {
    auto appState = apemode::AppState::Get( );

    if ( storageDirectory.empty( ) )
        storageDirectory = "./";

    bool addSubDirectories = false;

    if ( storageDirectory.size( ) > 4 ) {
        const size_t ds = storageDirectory.size( );
        addSubDirectories |= storageDirectory.compare( ds - 3, 3, "\\**" ) == 0;
        addSubDirectories |= storageDirectory.compare( ds - 3, 3, "/**" )  == 0;

        if ( addSubDirectories )
            storageDirectory = storageDirectory.substr( 0, storageDirectory.size( ) - 2 );
    }

    if ( DirectoryExists( storageDirectory ) ) {
        const std::string storageDirectoryFull = ResolveFullPath( storageDirectory );

        /* Lambda to check the path and the file to Files. */

        auto addFileFn = [&]( const std::filesystem::path& filePath ) {
            if ( std::filesystem::is_regular_file( filePath ) ) {

                /* Check if file matches the patterns */

                bool matches = FilePatterns.empty( );
                for ( const auto& f : FilePatterns ) {
                    if ( std::regex_match( filePath.string( ), std::regex( f ) ) ) {
                        matches = true;
                        break;
                    }
                }

                if ( matches ) {

                    /* Update file state */

                    std::error_code lastWriteTimeError;
                    const auto      filePathFull  = ResolveFullPath( filePath.string( ) );
                    const auto      lastWriteTime = std::filesystem::last_write_time( filePathFull, lastWriteTimeError );

                    auto fileIt = Files.find( filePathFull );
                    if ( fileIt == Files.end( ) ) {
                        appState->Logger->info( "FileScanner: Added: {} ({})", filePathFull.c_str( ), lastWriteTime.time_since_epoch( ).count( ) );
                    }

                    auto& fileState = Files[ filePathFull ];
                    if ( lastWriteTimeError == std::error_code( ) ) {
                        /* Swap the times to know if it has been changed. */
                        fileState.PrevTime = fileState.CurrTime;
                        fileState.CurrTime = lastWriteTime.time_since_epoch( ).count( );
                    } else {
                        /* Set to error state. */
                        fileState.CurrTime = 0;
                        appState->Logger->info( "FileScanner: Error state: {}", filePath.string( ).c_str( ) );
                    }
                }
            }
        };

        auto fileIt = Files.begin( );
        if ( bRemoveDeletedFiles ) {
            /* Remove from Files. */
            for ( ; fileIt != Files.end( ); ) {
                if ( false == FileExists( fileIt->first ) ) {
                    appState->Logger->debug( "FileScanner: Deleted: {}", fileIt->first.c_str( ) );
                    fileIt = Files.erase( fileIt );
                } else {
                    ++fileIt;
                }
            }
        } else {
            /* Set to deleted state. */
            for ( ; fileIt != Files.end( ); ) {
                if ( false == FileExists( fileIt->first ) ) {
                    appState->Logger->debug( "FileScanner: Deleted state: {}", fileIt->first.c_str( ) );
                    fileIt->second.CurrTime = 0;
                    fileIt->second.PrevTime = 0;
                }
            }
        }

        if ( addSubDirectories ) {
            for ( const auto& fileOrFolderPath : std::filesystem::recursive_directory_iterator( storageDirectoryFull ) )
                addFileFn( fileOrFolderPath.path( ) );
        } else {
            for ( const auto& fileOrFolderPath : std::filesystem::directory_iterator( storageDirectoryFull ) )
                addFileFn( fileOrFolderPath.path( ) );
        }

        return true;
    }

    return false;
}

void apemodeos::FileTracker::CollectChangedFiles( std::vector< std::string >& OutChangedFiles ) {
    uint32_t changedFileCount = 0;

    for ( auto& file : Files )
        changedFileCount += ( uint32_t )( file.second.Changed( ) || file.second.New( ) );

    if ( changedFileCount ) {
        OutChangedFiles.reserve( changedFileCount );

        for ( auto& file : Files )
            if ( file.second.Changed( ) || file.second.New( ) )
                OutChangedFiles.push_back( file.first );
    }
}

std::vector< uint8_t > apemodeos::FileReader::ReadBinFile( const std::string& filePath ) {
    const std::string filePathFull = ResolveFullPath( filePath );

    /* Read file and return */
    std::ifstream fileStream( filePathFull, std::ios::binary | std::ios::ate );
    std::streamsize fileWholeSize = fileStream.tellg( );
    fileStream.seekg( 0, std::ios::beg );
    std::vector< uint8_t > fileBuffer( fileWholeSize );
    if ( fileStream.read( (char*) fileBuffer.data( ), fileWholeSize ).good( ) ) {
        return fileBuffer;
    }

    return {};
}

std::string apemodeos::FileReader::ReadTxtFile( const std::string& filePath ) {
    const std::string filePathFull = ResolveFullPath( filePath );

    /* Read file and return */
    std::ifstream fileStream( filePathFull );
    if ( fileStream.good( ) ) {
        return std::string( std::istreambuf_iterator< char >( fileStream ),
                            std::istreambuf_iterator< char >( ) );
    }

    return "";
}
