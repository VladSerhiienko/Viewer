#include "FileTracker.h"

#include <AppState.h>
#include <tinydir.h>

#include <set>
#include <regex>
#include <fstream>
#include <iterator>

bool apemodeos::DirectoryExists( const std::string& directoryPath ) {
#if _WIN32
    DWORD dwAttrib = GetFileAttributes( directoryPath.c_str( ) );
    return ( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
#else
    struct stat statBuffer;
    return ( stat( directoryPath.c_str( ), &statBuffer ) == 0 ) && S_ISDIR( statBuffer.st_mode );
#endif
}

bool apemodeos::FileExists( const std::string& filePath ) {
#if _WIN32
    DWORD dwAttrib = GetFileAttributes( filePath.c_str( ) );
    return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
#else
    struct stat statBuffer;
    return ( stat( filePath.c_str( ), &statBuffer ) == 0 ) && S_ISREG( statBuffer.st_mode );
#endif
}

uint64_t GetLastModifiedTime( const std::string& filePath ) {
    struct stat statBuffer;

#if _WIN32
    if ( _stat( filePath.c_str( ), &statBuffer ) != 0 )
        return 0;
#else
    if ( stat( filePath.c_str( ), &statBuffer ) != 0 )
        return 0;
#endif

    return statBuffer.st_mtime;
}

std::string ToCanonicalAbsolutPath( const std::string & relativePath ) {
    char canonicalAbsolutePathBuffer[ PATH_MAX ] = {0};

#if _WIN32
    if ( !_fullpath( canonicalAbsolutePathBuffer, relativePath.c_str( ), sizeof( canonicalAbsolutePathBuffer ) ) ) {
        return "";
    }
#else
    struct stat statBuffer;
    if ( stat( relativePath.c_str( ), &statBuffer ) != 0 )
        return "";

    realpath( relativePath.c_str( ), canonicalAbsolutePathBuffer );
#endif

    char* canonicalAbsolutePathIt    = canonicalAbsolutePathBuffer;
    char* canonicalAbsolutePathEndIt = canonicalAbsolutePathBuffer + strlen( canonicalAbsolutePathBuffer );

    std::replace( canonicalAbsolutePathIt, canonicalAbsolutePathEndIt, '\\', '/' );
    return canonicalAbsolutePathBuffer;
}

template < typename TFileCallback >
void ProcessFiles( TFileCallback callback, const tinydir_dir& dir, bool r ) {
    auto appState = apemode::AppState::Get( );
    appState->Logger->info( "AssetManager: Entering folder: {}", dir.path );

    for ( size_t i = 0; i < dir.n_files; i++ ) {
        tinydir_file file;
        if ( tinydir_readfile_n( &dir, &file, i ) != -1 )
            if ( file.is_reg ) {
                appState->Logger->info( "AssetManager: Processing file: {}", file.path );
                callback( file.path, file );
            } else if ( r && file.is_dir && strcmp( file.name, "." ) != 0 && strcmp( file.name, ".." ) != 0 ) {
                tinydir_dir subdir;
                if ( tinydir_open_sorted( &subdir, file.path ) != -1 ) {
                    ProcessFiles( callback, subdir, r );
                    tinydir_close( &subdir );
                }
            }
    }
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

    tinydir_dir dir;
    std::string storageDirectoryFull = ToCanonicalAbsolutPath( storageDirectory );
    if ( !storageDirectoryFull.empty( ) && tinydir_open_sorted( &dir, storageDirectoryFull.c_str( ) ) != -1 ) {

        /* Lambda to check the path and the file to Files. */
        auto addFileFn = [&]( const std::string& filePath, tinydir_file file ) {

            /* Check if file matches the patterns */
            bool matches = FilePatterns.empty( );
            for ( const auto& f : FilePatterns ) {
                if ( std::regex_match( filePath, std::regex( f ) ) ) {
                    matches = true;
                    break;
                }
            }

            if ( matches ) {
                /* Update file state */
                const uint64_t lastWriteTime = GetLastModifiedTime( filePath );

                auto fileIt = Files.find( filePath );
                if ( fileIt == Files.end( ) ) {
                    appState->Logger->info( "FileScanner: Added: {} ({})", filePath.c_str( ), lastWriteTime );
                }

                auto& fileState = Files[ filePath ];
                if ( lastWriteTime ) {
                    /* Swap the times to know if it has been changed. */
                    fileState.PrevTime = fileState.CurrTime;
                    fileState.CurrTime = lastWriteTime;
                } else {
                    /* Set to error state. */
                    fileState.CurrTime = 0;
                    appState->Logger->info( "FileScanner: Error state: {}", filePath );
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

        ProcessFiles( addFileFn, dir, addSubDirectories );
        tinydir_close( &dir );

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
    const std::string resolvedFilePath = ToCanonicalAbsolutPath( filePath );

    std::ifstream fileStream( resolvedFilePath, std::ios::binary | std::ios::ate );
    if ( fileStream.good( ) ) {

        std::streamsize fileSize = fileStream.tellg( );
        fileStream.seekg( 0, std::ios::beg );
        std::vector< uint8_t > fileBuffer( fileSize );

        /* Read file and return */
        if ( fileStream.read( (char*) fileBuffer.data( ), fileSize ).good( ) ) {
            return fileBuffer;
        }
    }

    return {};
}

std::string apemodeos::FileReader::ReadTxtFile( const std::string& filePath ) {
    const std::string resolvedFilePath = ToCanonicalAbsolutPath( filePath );

    std::ifstream fileStream( resolvedFilePath );
    if ( fileStream.good( ) ) {

        /* Read file and return */
        return std::string( std::istreambuf_iterator< char >( fileStream ),
                            std::istreambuf_iterator< char >( ) );
    }

    return "";
}
