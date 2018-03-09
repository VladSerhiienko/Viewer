#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace apemodeos {

    //
    // Set of functions and classes to interact with file system.
    //

    bool PathExists( const std::string& InPath );
    bool DirectoryExists( const std::string& InPath );
    bool FileExists( const std::string& InPath );

    std::string CurrentDirectory( );
    std::string GetFileName( const std::string& InPath );
    std::string ReplaceSlashes( std::string InPath );
    std::string RealPath( std::string InPath );
    std::string ResolveFullPath( const std::string& InPath );

    /**
     * Currently only reads files either as a text or buffer.
     **/
    class FileReader {
    public:
        std::vector< uint8_t > ReadBinFile( const std::string& InPath ); /* Returns the content of the file. */
        std::string            ReadTxtFile( const std::string& InPath ); /* Returns the content of the file. */
    };
}