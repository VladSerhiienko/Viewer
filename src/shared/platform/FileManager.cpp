#include "FileTracker.h"

#include <AppState.h>
#include <tinydir.h>

#include <set>
#include <regex>
#include <fstream>
#include <iterator>

bool apemodeos::DirectoryExists( const char * pszPath ) {
#if _WIN32
    DWORD dwAttrib = GetFileAttributes( pszPath );
    return ( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
#else
    struct stat statBuffer;
    return ( stat( pszPath, &statBuffer ) == 0 ) && S_ISDIR( statBuffer.st_mode );
#endif
}

bool apemodeos::FileExists(  const char * pszPath ) {
#if _WIN32
    DWORD dwAttrib = GetFileAttributes( pszPath );
    return ( dwAttrib != INVALID_FILE_ATTRIBUTES && !( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
#else
    struct stat statBuffer;
    return ( stat( pszPath, &statBuffer ) == 0 ) && S_ISREG( statBuffer.st_mode );
#endif
}

uint64_t GetLastModifiedTime( const char * pszFilePath ) {

#if _WIN32
    struct _stat64i32 statBuffer;
    if ( _stat64i32( pszFilePath, &statBuffer ) != 0 )
        return 0;
#else
    struct stat statBuffer;
    if ( stat( pszFilePath, &statBuffer ) != 0 )
        return 0;
#endif

    return statBuffer.st_mtime;
}

#ifndef APEMODE_PATH_MAX
#define APEMODE_PATH_MAX 4096
#endif

std::string ToCanonicalAbsolutPath( const char * pszRelativePath ) {
    char canonicalAbsolutePathBuffer[ APEMODE_PATH_MAX ] = {0};

#if _WIN32
    if ( !_fullpath( canonicalAbsolutePathBuffer, pszRelativePath, sizeof( canonicalAbsolutePathBuffer ) ) ) {
        return "";
    }
#else
    struct stat statBuffer;
    if ( stat( pszRelativePath, &statBuffer ) != 0 )
        return "";

    realpath( pszRelativePath, canonicalAbsolutePathBuffer );
#endif

    char* canonicalAbsolutePathIt    = canonicalAbsolutePathBuffer;
    char* canonicalAbsolutePathEndIt = canonicalAbsolutePathBuffer + strlen( canonicalAbsolutePathBuffer );

    std::replace( canonicalAbsolutePathIt, canonicalAbsolutePathEndIt, '\\', '/' );
    return canonicalAbsolutePathBuffer;
}

template < typename TFileCallback >
void ProcessFiles( TFileCallback callback, const tinydir_dir& dir, bool r ) {
    auto appState = apemode::AppState::Get( );
    apemode::LogInfo( "AssetManager: Entering folder: {}", dir.path );

    for ( size_t i = 0; i < dir.n_files; i++ ) {
        tinydir_file file;
        if ( tinydir_readfile_n( &dir, &file, i ) != -1 ) {
            if ( file.is_reg ) {
                apemode::LogInfo( "AssetManager: Processing file: {}", file.path );
                callback( file.path, file );
            } else {
                if ( r && file.is_dir && strcmp( file.name, "." ) != 0 && strcmp( file.name, ".." ) != 0 ) {
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

bool apemodeos::FileTracker::ScanDirectory( std::string storageDirectory, bool bRemoveDeletedFiles ) {
    apemode_memory_allocation_scope;

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
    std::string storageDirectoryFull = ToCanonicalAbsolutPath( storageDirectory.c_str( ) );
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
                const uint64_t lastWriteTime = GetLastModifiedTime( filePath.c_str( ) );

                auto fileIt = Files.find( filePath );
                if ( fileIt == Files.end( ) ) {
                    apemode::LogInfo( "FileScanner: Added: {} ({})", filePath.c_str( ), lastWriteTime );
                }

                auto& fileState = Files[ filePath ];
                if ( lastWriteTime ) {
                    /* Swap the times to know if it has been changed. */
                    fileState.PrevTime = fileState.CurrTime;
                    fileState.CurrTime = lastWriteTime;
                } else {
                    /* Set to error state. */
                    fileState.CurrTime = 0;
                    apemode::LogInfo( "FileScanner: Error state: {}", filePath );
                }
            }
        };

        auto fileIt = Files.begin( );
        if ( bRemoveDeletedFiles ) {
            /* Remove from Files. */
            for ( ; fileIt != Files.end( ); ) {
                if ( false == FileExists( fileIt->first.c_str( ) ) ) {
                    apemode::LogInfo( "FileScanner: Deleted: {}", fileIt->first.c_str( ) );
                    fileIt = Files.erase( fileIt );
                } else {
                    ++fileIt;
                }
            }
        } else {
            /* Set to deleted state. */
            for ( ; fileIt != Files.end( ); ) {
                if ( false == FileExists( fileIt->first.c_str( ) ) ) {
                    apemode::LogInfo( "FileScanner: Deleted state: {}", fileIt->first.c_str( ) );
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
    apemode_memory_allocation_scope;

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

template < bool bTextMode >
apemodeos::AssetContentBuffer TReadFile( const char* pszFilePath ) {
    apemodeos::AssetContentBuffer assetContentBuffer;

    FILE* pFile = fopen( pszFilePath, "rb" );
    if ( !pFile )
        return {};

    fseek( pFile, 0, SEEK_END );
    const size_t size = static_cast< size_t >( ftell( pFile ) );
    fseek( pFile, 0, SEEK_SET );

    assetContentBuffer.Alloc( size + bTextMode );
    if ( !assetContentBuffer.pData )
        return {};

    fread( assetContentBuffer.pData, size, 1, pFile );
    fclose( pFile );

    if ( bTextMode ) {
        assetContentBuffer.pData[ size ] = 0;
    }

    return std::move( assetContentBuffer );
}

apemodeos::AssetContentBuffer apemodeos::FileReader::ReadBinFile( const char* pszFilePath ) {
    return std::move( TReadFile< false >( pszFilePath ) );
}

apemodeos::AssetContentBuffer apemodeos::FileReader::ReadTxtFile( const char* pszFilePath ) {
    return std::move( TReadFile< true >( pszFilePath ) );
}
