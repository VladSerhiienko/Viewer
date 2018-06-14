#pragma once

#include <string>
#include <vector>
#include <stdint.h>

#include <IAssetManager.h>

namespace apemodeos {

    //
    // Set of functions and classes to interact with file system.
    //

    bool DirectoryExists( const char * pszPath );
    bool FileExists( const char * pszPath );
    // ...

    /**
     * Currently only reads files either as a text or buffer.
     **/
    class FileReader {
    public:
        apemodeos::AssetContentBuffer ReadBinFile( const char * pszFilePath ); /* Returns the content of the file. */
        apemodeos::AssetContentBuffer ReadTxtFile( const char * pszFilePath ); /* Returns the content of the file. */
    };
}