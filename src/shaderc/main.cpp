
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>
#include <nlohmann/json.hpp>

#include <flatbuffers/util.h>
#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"

using json = nlohmann::json;

namespace {

class ShaderCompilerIncludedFileSet : public apemode::shp::IShaderCompiler::IIncludedFileSet {
public:
    std::set< std::string > IncludedFiles;

    void InsertIncludedFile( const std::string& includedFileName ) override {
        IncludedFiles.insert( includedFileName );
    }
};

class ShaderCompilerMacroDefinitionCollection : public apemode::shp::IShaderCompiler::IMacroDefinitionCollection {
public:
    std::map< std::string, std::string > Macros;

    void Init( std::map< std::string, std::string >&& macros ) {
        Macros = std::move(macros);
    }

    size_t GetCount( ) const override {
        return Macros.size( );
    }

    apemode::shp::IShaderCompiler::MacroDefinition GetMacroDefinition( const size_t macroIndex ) const override {
        auto macroIt = Macros.begin( );
        std::advance( macroIt, macroIndex );

        apemode::shp::IShaderCompiler::MacroDefinition macroDefinition;
        macroDefinition.pszKey   = macroIt->first.c_str( );
        macroDefinition.pszValue = macroIt->second.c_str( );
        return macroDefinition;
    }
};

class ShaderFileReader : public apemode::shp::IShaderCompiler::IShaderFileReader {
public:
    apemode::platform::IAssetManager* mAssetManager;

    bool ReadShaderTxtFile( const std::string& InFilePath, std::string& OutFileFullPath, std::string& OutFileContent ) {
        apemode_memory_allocation_scope;
        if ( auto pAsset = mAssetManager->Acquire( InFilePath.c_str( ) ) ) {
            const auto assetText = pAsset->GetContentAsTextBuffer( );
            OutFileContent       = reinterpret_cast< const char* >( assetText.data( ) );
            OutFileFullPath      = pAsset->GetId( );
            mAssetManager->Release( pAsset );
            return true;
        }

        // apemodevk::platform::DebugBreak( );
        return false;
    }
};

struct EFeedbackTypeWithOStream {
    const apemode::shp::IShaderCompiler::IShaderFeedbackWriter::EFeedbackType e;

    EFeedbackTypeWithOStream( const apemode::shp::IShaderCompiler::IShaderFeedbackWriter::EFeedbackType e ) : e( e ) {
    }

    // clang-format off
    template < typename OStream >
    inline friend OStream& operator<<( OStream& os, const EFeedbackTypeWithOStream& feedbackType ) {
        using e = apemode::shp::IShaderCompiler::IShaderFeedbackWriter;
        switch ( feedbackType.e ) {
        case e::eFeedbackType_CompilationStage_Assembly:                 return os << "Assembly";
        case e::eFeedbackType_CompilationStage_Preprocessed:             return os << "Preprocessed";
        case e::eFeedbackType_CompilationStage_PreprocessedOptimized:    return os << "PreprocessedOptimized";
        case e::eFeedbackType_CompilationStage_Spv:                      return os << "Spv";
        case e::eFeedbackType_CompilationStatus_CompilationError:        return os << "CompilationError";
        case e::eFeedbackType_CompilationStatus_InternalError:           return os << "InternalError";
        case e::eFeedbackType_CompilationStatus_InvalidAssembly:         return os << "InvalidAssembly";
        case e::eFeedbackType_CompilationStatus_InvalidStage:            return os << "InvalidStage";
        case e::eFeedbackType_CompilationStatus_NullResultObject:        return os << "NullResultObject";
        case e::eFeedbackType_CompilationStatus_Success:                 return os << "Success";
        default:                                                                                                            return os;
        }
    }
    // clang-format on
};

class ShaderFeedbackWriter : public apemode::shp::IShaderCompiler::IShaderFeedbackWriter {
public:
    void WriteFeedback( EFeedbackType                                                    eType,
                        const std::string&                                               FullFilePath,
                        const apemode::shp::IShaderCompiler::IMacroDefinitionCollection* pMacros,
                        const void*                                                      pContent,
                        const void*                                                      pContentEnd ) {
        apemode_memory_allocation_scope;

        const auto feedbackStage            = eType & eFeedbackType_CompilationStageMask;
        const auto feedbackCompilationError = eType & eFeedbackType_CompilationStatusMask;

        if ( eFeedbackType_CompilationStatus_Success != feedbackCompilationError ) {
            apemode::LogError( "ShaderCompiler: {}/{}: {}",
                               EFeedbackTypeWithOStream( feedbackStage ),
                               EFeedbackTypeWithOStream( feedbackCompilationError ),
                               FullFilePath );
            apemode::LogError( " Msg: {}", (const char*) pContent );
            assert( false );
        } else {
            apemode::LogInfo( "ShaderCompiler: {}/{}: {}",
                              EFeedbackTypeWithOStream( feedbackStage ),
                              EFeedbackTypeWithOStream( feedbackCompilationError ),
                              FullFilePath );
        }
    }
};

class ShaderCompilerMacroGroupCollection {
public:
    std::vector< ShaderCompilerMacroDefinitionCollection > MacroGroups;

    void Init( std::vector< ShaderCompilerMacroDefinitionCollection >&& groups ) {
        MacroGroups = std::move( groups );
    }

    size_t GetCount( ) const {
        return MacroGroups.size( );
    }

    const apemode::shp::IShaderCompiler::IMacroDefinitionCollection* GetMacroGroup( const size_t groupIndex ) const {
        return &MacroGroups[ groupIndex ];
    }
};

void AssertContainsWhitespace( const std::string& s ) {
    bool containsWhitespace = false;
    for ( auto c : s ) {
        containsWhitespace |= isspace( c );
    }

    assert( !containsWhitespace );
    if ( containsWhitespace ) {
        apemode::LogError( "Should not contain whitespace, '{}'", s );
    }
}

std::string GetMacrosString( const std::map< std::string, std::string >& macros ) {
    if ( macros.empty( ) ) {
        return "";
    }

    std::string macrosString;
    for ( const auto& macro : macros ) {
        AssertContainsWhitespace( macro.first );
        macrosString += macro.first;
        macrosString += "=";
        macrosString += macro.second;
        macrosString += ";";
    }

    macrosString.erase( macrosString.size( ) - 1 );
    return macrosString;
}

void ReplaceAll( std::string& data, const std::string& toSearch, const std::string& replaceStr ) {
    size_t pos = data.find( toSearch );
    while ( pos != std::string::npos ) {
        data.replace( pos, toSearch.size( ), replaceStr );
        pos = data.find( toSearch, pos + replaceStr.size( ) );
    }
}

apemode::shp::IShaderCompiler::EShaderType GetShaderType( const std::string& shaderType ) {
    if ( shaderType == "vert" ) {
        return apemode::shp::IShaderCompiler::eShaderType_VertexShader;
    } else if ( shaderType == "frag" ) {
        return apemode::shp::IShaderCompiler::eShaderType_FragmentShader;
    } else if ( shaderType == "comp" ) {
        return apemode::shp::IShaderCompiler::eShaderType_ComputeShader;
    } else if ( shaderType == "geom" ) {
        return apemode::shp::IShaderCompiler::eShaderType_GeometryShader;
    } else if ( shaderType == "tesc" ) {
        return apemode::shp::IShaderCompiler::eShaderType_TessControlShader;
    } else if ( shaderType == "tese" ) {
        return apemode::shp::IShaderCompiler::eShaderType_TessEvaluationShader;
    }

    apemode::LogError( "Shader type should be one of there: vert, frag, comp, tesc, tese, instead got value: {}", shaderType );
    return apemode::shp::IShaderCompiler::eShaderType_GLSL_InferFromSource;
}

std::map< std::string, std::string > GetMacroDefinitions( const json& macrosJson ) {
    std::map< std::string, std::string > macroDefinitions;
    for ( const auto& macroJson : macrosJson ) {
        assert( macroJson.is_object( ) );
        assert( macroJson[ "name" ].is_string( ) );

        const std::string macroName = macroJson[ "name" ].get< std::string >( );
        macroDefinitions[ macroName ] = "1";

        auto valueJsonIt = macroJson.find( "value" );
        if ( valueJsonIt != macroJson.end( ) && valueJsonIt->is_boolean( ) ) {
            if ( false == valueJsonIt->get< bool >( ) ) {
                macroDefinitions[ macroName ] = "0";
            }
        } else if ( valueJsonIt->is_number_integer( ) ) {
            macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< int >( ) );
        } else if ( valueJsonIt->is_number_unsigned( ) ) {
            macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< unsigned >( ) );
        } else if ( valueJsonIt->is_number_float( ) ) {
            macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< int >( ) );
        }
    }

    return macroDefinitions;
}

ShaderCompilerMacroDefinitionCollection GetMacroGroup( const json& groupJson ) {
    std::map< std::string, std::string > macroDefinitions;
    for ( const auto& macroJson : groupJson ) {
        assert( macroJson.is_object( ) );
        assert( macroJson[ "name" ].is_string( ) );

        const std::string macroName = macroJson[ "name" ].get< std::string >( );
        macroDefinitions[ macroName ] = "1";

        auto valueJsonIt = macroJson.find( "value" );
        if ( valueJsonIt != macroJson.end( ) ) {
            if ( valueJsonIt->is_boolean( ) ) {
                if ( false == valueJsonIt->get< bool >( ) ) {
                    macroDefinitions[ macroName ] = "0";
                }
            } else if ( valueJsonIt->is_number_integer( ) ) {
                macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< int >( ) );
            } else if ( valueJsonIt->is_number_unsigned( ) ) {
                macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< unsigned >( ) );
            } else if ( valueJsonIt->is_number_float( ) ) {
                macroDefinitions[ macroName ] = std::to_string( valueJsonIt->get< int >( ) );
            }
        }
    }

    ShaderCompilerMacroDefinitionCollection group;
    group.Init( std::move( macroDefinitions ) );
    return group;
}

flatbuffers::Offset< csofb::CompiledShaderFb > CompileShaderVariant(
    flatbuffers::FlatBufferBuilder&             builder,
    const apemode::shp::IShaderCompiler&        shaderCompiler,
    std::map< std::string, std::string >        macroDefinitions,
    const std::string&                          shaderType,
    std::string                                 srcFile,
    const std::string&                          outputFolder ) {
    flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset{};

    std::string macrosString = GetMacrosString( macroDefinitions );

    ShaderCompilerMacroDefinitionCollection concreteMacros;
    concreteMacros.Init( std::move( macroDefinitions ) );

    apemode::shp::IShaderCompiler::EShaderType eShaderType   = GetShaderType( shaderType );
    csofb::EShaderType                         eShaderTypeFb = csofb::EShaderType( eShaderType );

    ShaderCompilerIncludedFileSet includedFileSet;
    if ( auto compiledShader = shaderCompiler.Compile( srcFile,
                                                       &concreteMacros,
                                                       eShaderType,
                                                       apemode::shp::IShaderCompiler::eShaderOptimization_Performance,
                                                       &includedFileSet ) ) {
        std::vector< uint32_t > spv;
        spv.assign( compiledShader->GetDwordPtr( ), compiledShader->GetDwordPtr( ) + compiledShader->GetDwordCount( ) );

        flatbuffers::Offset< flatbuffers::String > assetOffsetFb{};
        assetOffsetFb = builder.CreateString( srcFile );

        flatbuffers::Offset< flatbuffers::String > macrosOffsetFb{};
        if ( !macrosString.empty( ) ) {
            macrosOffsetFb = builder.CreateString( macrosString );
        }

        flatbuffers::Offset< flatbuffers::Vector< uint32_t > > spvOffsetFb{};
        spvOffsetFb = builder.CreateVector( spv );

        csoOffset = csofb::CreateCompiledShaderFb( builder, assetOffsetFb, macrosOffsetFb, eShaderTypeFb, spvOffsetFb );

        ReplaceAll( macrosString, ".", "-" );
        ReplaceAll( macrosString, ";", "+" );

        const size_t fileStartPos = srcFile.find_last_of( "/\\" );
        if ( fileStartPos != srcFile.npos ) {
            srcFile = srcFile.substr( fileStartPos + 1 );
        }

        std::string cachedCSO =
            outputFolder + "/" + srcFile + ( macrosString.empty( ) ? "" : "-defs-" ) + macrosString + ".spv";

        ReplaceAll( cachedCSO, "//", "/" );
        ReplaceAll( cachedCSO, "\\/", "\\" );
        ReplaceAll( cachedCSO, "\\\\", "\\" );

        std::string cachedPreprocessed = cachedCSO + "-preprocessed.txt";
        std::string cachedAssembly     = cachedCSO + "-assembly.txt";

        flatbuffers::SaveFile(
            cachedCSO.c_str( ), (const char*) compiledShader->GetBytePtr( ), compiledShader->GetByteCount( ), true );

        flatbuffers::SaveFile( cachedPreprocessed.c_str( ),
                               compiledShader->GetPreprocessedSrc( ).data( ),
                               compiledShader->GetPreprocessedSrc( ).size( ),
                               false );

        flatbuffers::SaveFile( cachedAssembly.c_str( ),
                               compiledShader->GetAssemblySrc( ).data( ),
                               compiledShader->GetAssemblySrc( ).size( ),
                               false );
    }

    return csoOffset;
}

void CompileShaderVariantsRecursively( std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets,
                                       flatbuffers::FlatBufferBuilder&                               builder,
                                       const apemode::shp::IShaderCompiler&                          shaderCompiler,
                                       const ShaderCompilerMacroGroupCollection&                     macroGroups,
                                       const size_t                                                  macroGroupIndex,
                                       const std::string&                                            shaderType,
                                       std::string                                                   srcFile,
                                       const std::string&                                            outputFolder,
                                       std::vector< size_t >&                                        macroIndices ) {
    assert( macroIndices.size( ) == macroGroups.MacroGroups.size( ) );
    if ( macroGroupIndex >= macroGroups.MacroGroups.size( ) ) {
        std::map< std::string, std::string > macroDefinitions;

        apemode::LogInfo( "Collecting definitions ..." );
        for ( size_t i = 0; i < macroGroups.GetCount( ); ++i ) {
            auto g = macroGroups.GetMacroGroup( i );
            auto j = macroIndices[ i ];
            auto k = g->GetMacroDefinition( j ).pszKey;
            auto v = g->GetMacroDefinition( j ).pszValue;

            if ( strcmp( k, "" ) != 0 ) {
                apemode::LogInfo( "Adding: group {}, macro {}, {}={}", i, j, k, v );
                macroDefinitions[ k ] = v;
            } else {
                apemode::LogInfo( "Skipping: group={}", i );
            }
        }

        apemode::LogInfo( "Sending to compiler ..." );
        csoOffsets.push_back(
            CompileShaderVariant( builder, shaderCompiler, std::move( macroDefinitions ), shaderType, srcFile, outputFolder ) );
        return;
    }

    auto& group = macroGroups.MacroGroups[ macroGroupIndex ];
    for ( size_t i = 0; i < group.GetCount( ); ++i ) {
        macroIndices[ macroGroupIndex ] = i;
        CompileShaderVariantsRecursively( csoOffsets,
                                          builder,
                                          shaderCompiler,
                                          macroGroups,
                                          macroGroupIndex + 1,
                                          shaderType,
                                          srcFile,
                                          outputFolder,
                                          macroIndices );
    }
}

void CompileShaderVariantsRecursively( std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets,
                                       flatbuffers::FlatBufferBuilder&                               builder,
                                       const apemode::shp::IShaderCompiler&                          shaderCompiler,
                                       const ShaderCompilerMacroGroupCollection&                     macroGroups,
                                       const std::string&                                            shaderType,
                                       std::string                                                   srcFile,
                                       const std::string&                                            outputFolder ) {
    std::vector< size_t > macroIndices( macroGroups.GetCount( ) );
    CompileShaderVariantsRecursively(
        csoOffsets, builder, shaderCompiler, macroGroups, 0, shaderType, srcFile, outputFolder, macroIndices );
}

std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > CompileShader(
    flatbuffers::FlatBufferBuilder&                builder,
    const apemode::platform::shared::AssetManager& assetManager,
    apemode::shp::IShaderCompiler&                 shaderCompiler,
    const json&                                    commandJson,
    const std::string&                             outputFolder ) {
    std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets;

    assert( commandJson[ "srcFile" ].is_string( ) );
    std::string srcFile = commandJson[ "srcFile" ].get< std::string >( );
    apemode::LogInfo( "srcFile: {}", srcFile.c_str( ) );

    flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset{};
    if ( auto acquiredSrcFile = assetManager.Acquire( srcFile.c_str( ) ) ) {
        apemode::LogInfo( "assetId: {}", acquiredSrcFile->GetId( ) );

        auto definitionGroupsJsonIt = commandJson.find( "definitionGroups" );
        if ( definitionGroupsJsonIt != commandJson.end( ) && definitionGroupsJsonIt->is_array( ) ) {
            ShaderCompilerMacroGroupCollection macroGroups;

            const json& definitionGroupsJson = *definitionGroupsJsonIt;
            for ( auto& definitionGroupJson : definitionGroupsJson ) {
                macroGroups.MacroGroups.push_back( GetMacroGroup( definitionGroupJson ) );
            }

            assert( commandJson[ "shaderType" ].is_string( ) );
            const std::string shaderType = commandJson[ "shaderType" ].get< std::string >( );

            CompileShaderVariantsRecursively(
                csoOffsets, builder, shaderCompiler, macroGroups, shaderType, srcFile, outputFolder );
        } else {
            std::map< std::string, std::string > macroDefinitions;
            ShaderCompilerMacroDefinitionCollection macros;

            auto macrosJsonIt = commandJson.find( "macros" );
            if ( macrosJsonIt != commandJson.end( ) && macrosJsonIt->is_array( ) ) {
                const json& macrosJson = *macrosJsonIt;
                macroDefinitions = GetMacroDefinitions( macrosJson );

                if ( !macroDefinitions.empty( ) ) {
                    macros.Init( std::move( macroDefinitions ) );
                }
            }

            assert( commandJson[ "shaderType" ].is_string( ) );
            const std::string shaderType = commandJson[ "shaderType" ].get< std::string >( );

            csoOffsets.push_back(
                CompileShaderVariant( builder, shaderCompiler, macroDefinitions, shaderType, srcFile, outputFolder ) );
        }

        return csoOffsets;
    }

    apemode::LogError( "Asset not found, the command \"{}\" skipped.", commandJson.dump( ).c_str( ) );
    return {};
}
} // namespace

int main( int argc, char** argv ) {
    /* Input parameters:
     * --assets-folder "/Users/vlad.serhiienko/Projects/Home/Viewer/assets"
     * --cso-json-file "/Users/vlad.serhiienko/Projects/Home/Viewer/assets/shaders/Viewer.cso.json"
     * --cso-file "/Users/vlad.serhiienko/Projects/Home/Viewer/assets/shaders/Viewer.cso"
     */
    apemode::AppState::OnMain( argc, (const char**) argv );
    apemode::AppStateExitGuard eg{};

    std::string assetsFolderWildcard = apemode::TGetOption( "assets-folder", std::string( ) );
    if ( assetsFolderWildcard.empty( ) ) {
        apemode::LogError( "No assets folder." );
        return 1;
    }

    std::string outputFile = apemode::TGetOption( "cso-file", std::string( ) );
    if ( outputFile.empty( ) ) {
        apemode::LogError( "Output CSO file is empty." );
        return 1;
    }

    std::string outputFolder = outputFile + ".d";
#if defined( _WIN32 )
    CreateDirectory( outputFolder.c_str( ), NULL );
#else
    mkdir( outputFolder.c_str( ), S_IRWXU | S_IRWXG | S_IRWXO );
#endif

    if ( outputFile.empty( ) ) {
        apemode::LogError( "Output CSO file is empty." );
        return 1;
    }

    std::string assetsFolder = assetsFolderWildcard;
    ReplaceAll( assetsFolder, "*", "" );
    ReplaceAll( assetsFolder, "\\", "/" );
    ReplaceAll( assetsFolder, "//", "/" );

    apemode::LogInfo( "Asset folder: {}", assetsFolder );
    apemode::LogInfo( "Asset folder (wildcard): {}", assetsFolderWildcard );

    apemode::platform::shared::AssetManager assetManager;
    assetManager.UpdateAssets( assetsFolderWildcard.c_str( ), nullptr, 0 );

    std::string csoJsonFile = apemode::TGetOption( "cso-json-file", std::string( ) );
    if ( csoJsonFile.empty( ) || !apemode::platform::shared::FileExists( csoJsonFile.c_str( ) ) ) {
        apemode::LogError( "No CSO file." );
        return 1;
    }

    apemode::LogInfo( "CSO JSON file: {}", csoJsonFile );

    auto csoJsonContents = apemode::platform::shared::FileReader( ).ReadTxtFile( csoJsonFile.c_str( ) );
    if ( csoJsonFile.empty( ) ) {
        apemode::LogError( "CSO file is empty." );
        return 1;
    }

    const json csoJson = json::parse( (const char*) csoJsonContents.data( ) );
    if ( !csoJson.is_object( ) ) {
        apemode::LogError( "Parsing error." );
        return 1;
    }

    assert( csoJson[ "commands" ].is_array( ) );
    if ( !csoJson[ "commands" ].is_array( ) ) {
        apemode::LogError( "Parsing error." );
        return 1;
    }

    auto shaderCompiler = apemode::shp::NewShaderCompiler( );

    ShaderFileReader     shaderCompilerFileReader;
    ShaderFeedbackWriter shaderFeedbackWriter;

    shaderCompilerFileReader.mAssetManager = &assetManager;
    shaderCompiler->SetShaderFileReader( &shaderCompilerFileReader );
    shaderCompiler->SetShaderFeedbackWriter( &shaderFeedbackWriter );

    flatbuffers::FlatBufferBuilder builder;
    std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets;

    const json& commandsJson = csoJson[ "commands" ];
    for ( const auto& commandJson : commandsJson ) {
        std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets =
            CompileShader( builder, assetManager, *shaderCompiler, commandJson, outputFolder );

        for ( auto& csoOffset : csoOffsets ) {
            if ( !csoOffset.IsNull( ) ) {
                csoOffsets.push_back( csoOffset );
            }
        }
    }

    flatbuffers::Offset< flatbuffers::Vector< flatbuffers::Offset< csofb::CompiledShaderFb > > > csosOffset;
    csosOffset = builder.CreateVector( csoOffsets );

    flatbuffers::Offset< csofb::CollectionFb > collectionFb;
    collectionFb = csofb::CreateCollectionFb( builder, csofb::EVersionFb_Value, csosOffset );

    csofb::FinishCollectionFbBuffer( builder, collectionFb );

    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert( csofb::VerifyCollectionFbBuffer( v ) );

    auto csoBuffer     = (const char*) builder.GetBufferPointer( );
    auto csoBufferSize = (size_t) builder.GetSize( );

    if ( !flatbuffers::SaveFile( outputFile.c_str( ), csoBuffer, csoBufferSize, true ) ) {
        apemode::LogError( "Failed to write CSO ({} bytes) to file: '{}'", csoBufferSize, outputFile );
        return 1;
    }

    apemode::LogInfo( "Done." );
    return 0;
}
