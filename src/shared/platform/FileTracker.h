#pragma once

#include <map>
#include <FileReader.h>

namespace apemodeos {

    class FileTracker {
    public:
        struct ScannedFile {
            uint64_t PrevTime = 0; /* Previous update time */
            uint64_t CurrTime = 0; /* Last update time */

            inline bool Unchanged( ) const { return 0 == ( CurrTime - PrevTime ); } /* File has been changed since last frame (or was deleted). */
            inline bool Deleted( ) const { return 0 == CurrTime && 0 == PrevTime; } /* File has been deleted. */
            inline bool Changed( ) const { return 0 != ( CurrTime - PrevTime ); }   /* File has not been changed since last frame. */
            inline bool Error( ) const { return  0 == CurrTime && 0 != PrevTime; }  /* Failed to read the file info. */
            inline bool New( ) const { return 0 != CurrTime && 0 == PrevTime; }     /* File has been just created. */
        };

        std::map< std::string, ScannedFile > Files;        /* Scanned files */
        std::vector< std::string >           FilePatterns; /* Scan patterns */

        /**
         * @param storageDirectory Directory path (can be relative).
         * @param bRemoveDeletedFiles If true removes deleted files from Files, otherwise sets them to deleted state (@see Deleted()).
         * @note Use "" or "./" for scanning the current directory.
         *       Use "**" at the end of the path for scanning recursively.
         **/
        bool ScanDirectory( std::string InPath, bool bRemoveDeletedFiles );

        /**
         * Populates OutChangedFiles with the file paths, that were changed since the previous ScanDirectory().
         * @param OutChangedFiles Full file paths, that were changed.
         **/
        void CollectChangedFiles( std::vector< std::string >& OutChangedFiles );
    };
}