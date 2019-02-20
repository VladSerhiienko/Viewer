
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>
#include <nlohmann/json.hpp>

#include <flatbuffers/util.h>
#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"

using json = nlohmann::json;

namespace {
std::string GetMacrosString( std::map< std::string, std::string > macros ) {
    if ( macros.empty( ) )
        return "";

    std::string macrosString;
    for ( const auto& macro : macros ) {
        macrosString += macro.first;
        macrosString += "=";
        macrosString += macro.second;
        macrosString += ";";
    }

    macrosString.erase( macrosString.size( ) - 1 );
    return macrosString;
}

void ReplaceAll( std::string& data, std::string toSearch, std::string replaceStr ) {
    size_t pos = data.find( toSearch );
    while ( pos != std::string::npos ) {
        data.replace( pos, toSearch.size( ), replaceStr );
        pos = data.find( toSearch, pos + replaceStr.size( ) );
    }
}

apemode::shp::ShaderCompiler::EShaderType GetShaderType( const std::string& shaderType ) {
    if ( shaderType == "vert" ) {
        return apemode::shp::ShaderCompiler::eShaderType_VertexShader;
    } else if ( shaderType == "frag" ) {
        return apemode::shp::ShaderCompiler::eShaderType_FragmentShader;
    } else if ( shaderType == "comp" ) {
        return apemode::shp::ShaderCompiler::eShaderType_ComputeShader;
    } else if ( shaderType == "geom" ) {
        return apemode::shp::ShaderCompiler::eShaderType_GeometryShader;
    } else if ( shaderType == "tesc" ) {
        return apemode::shp::ShaderCompiler::eShaderType_TessControlShader;
    } else if ( shaderType == "tese" ) {
        return apemode::shp::ShaderCompiler::eShaderType_TessEvaluationShader;
    }
    
    apemode::LogError( "Shader type should be one of there: vert, frag, comp, tesc, tese, not this: {}", shaderType );
    return apemode::shp::ShaderCompiler::eShaderType_GLSL_InferFromSource;
}


std::map< std::string, std::string > GetMacroDefinitions(const json& macrosJson) {
    std::map< std::string, std::string > macroDefinitions;
    for ( const auto& macroJson : macrosJson ) {
        assert( macroJson.is_object( ) );
        assert( macroJson[ "name" ].is_string( ) );

        std::string macroName = macroJson[ "name" ].get< std::string >( );
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

flatbuffers::Offset< csofb::CompiledShaderFb > CompileShader( flatbuffers::FlatBufferBuilder&                builder,
                                                              apemode::shp::ShaderCompiler&                  shaderCompiler,
                                                              const std::map< std::string, std::string >&    macroDefinitions,
                                                              const std::string&                             shaderType,
                                                              std::string                                    srcFile,
                                                              const std::string&                             outputFolder ) {
    
    flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset{};

    std::string macrosString = GetMacrosString( macroDefinitions );
    
    apemode::shp::ShaderCompilerMacroDefinitionCollection     concreteMacros;
    apemode::shp::ShaderCompiler::IMacroDefinitionCollection* opaqueMacros = nullptr;
    if ( !macroDefinitions.empty( ) ) {
        concreteMacros.Init( macroDefinitions );
        opaqueMacros = &concreteMacros;
    }

    apemode::shp::ShaderCompiler::EShaderType eShaderType = GetShaderType( shaderType );
    csofb::EShaderType eShaderTypeFb = csofb::EShaderType( eShaderType );

    apemode::shp::ShaderCompilerIncludedFileSet includedFileSet;
    if ( auto compiledShader = shaderCompiler.Compile( srcFile,
                                                       opaqueMacros,
                                                       eShaderType,
                                                       apemode::shp::ShaderCompiler::eShaderOptimization_Performance,
                                                       &includedFileSet ) ) {
        std::vector< uint32_t > spv;
        spv.assign( compiledShader->GetDwordPtr( ), compiledShader->GetDwordPtr( ) + compiledShader->GetDwordCount( ) );

        flatbuffers::Offset< flatbuffers::String > assetOffsetFb{};
        assetOffsetFb = builder.CreateString( srcFile );

        flatbuffers::Offset< flatbuffers::String > macrosOffsetFb{};
        if ( macrosString.size( ) ) {
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

        std::string cachedCSO = outputFolder + "/" + srcFile + ( macrosString.empty( ) ? "" : "-defs-" ) + macrosString + ".spv";

        ReplaceAll( cachedCSO, "//", "/" );
        ReplaceAll( cachedCSO, "\\/", "\\" );
        ReplaceAll( cachedCSO, "\\\\", "\\" );

        std::string cachedPreprocessed = cachedCSO + "-preprocessed.txt";
        std::string cachedAssembly     = cachedCSO + "-assembly.txt";

        flatbuffers::SaveFile(
            cachedCSO.c_str( ), (const char*) compiledShader->GetBytePtr( ), compiledShader->GetByteCount( ), true );

        flatbuffers::SaveFile( cachedPreprocessed.c_str( ),
                               compiledShader->GetPreprocessedSrc( ),
                               strlen( compiledShader->GetPreprocessedSrc( ) ),
                               false );

        flatbuffers::SaveFile( cachedAssembly.c_str( ),
                               compiledShader->GetAssemblySrc( ),
                               strlen( compiledShader->GetAssemblySrc( ) ),
                               false );
    }

    return csoOffset;
}


flatbuffers::Offset< csofb::CompiledShaderFb > CompileShader( flatbuffers::FlatBufferBuilder&                builder,
                                                              const apemode::platform::shared::AssetManager& assetManager,
                                                              apemode::shp::ShaderCompiler&                  shaderCompiler,
                                                              const json&                                    commandJson,
                                                              const std::string&                             outputFolder ) {
    assert( commandJson[ "srcFile" ].is_string( ) );
    std::string srcFile = commandJson[ "srcFile" ].get< std::string >( );
    apemode::LogInfo( "srcFile: {}", srcFile.c_str( ) );

    flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset{};
    if ( auto acquiredSrcFile = assetManager.Acquire( srcFile.c_str( ) ) ) {
        apemode::LogInfo( "assetId: {}", acquiredSrcFile->GetId( ) );
        
        auto macroGroupsJsonIt = commandJson.find( "macroGroups" );
        if ( macroGroupsJsonIt != commandJson.end( ) && macroGroupsJsonIt->is_array( ) ) {
            // ...
        } else {
            std::map< std::string, std::string > macroDefinitions;
            apemode::shp::ShaderCompilerMacroDefinitionCollection concreteMacros;
            apemode::shp::ShaderCompiler::IMacroDefinitionCollection* opaqueMacros = nullptr;
            
            auto macrosJsonIt = commandJson.find( "macros" );
            if ( macrosJsonIt != commandJson.end( ) && macrosJsonIt->is_array( ) ) {
                const json& macrosJson = *macrosJsonIt;
                macroDefinitions = GetMacroDefinitions(macrosJson);
            
                if ( !macroDefinitions.empty( ) ) {
                    concreteMacros.Init( macroDefinitions );
                    opaqueMacros = &concreteMacros;
                }
            }

            assert( commandJson[ "shaderType" ].is_string( ) );
            const std::string shaderType = commandJson[ "shaderType" ].get< std::string >( );

            return CompileShader(builder, shaderCompiler, macroDefinitions, shaderType, srcFile, outputFolder);
        }
    } else {
        apemode::LogError( "Asset not found, the command \"{}\" skipped.", commandJson.dump( ).c_str( ) );
    }

    return {};
}
} // namespace

int main( int argc, char** argv ) {
    apemode::AppState::OnMain( argc, (const char**) argv );

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
    #if defined(_WIN32)
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
    ReplaceAll( assetsFolder, "//", "/" );
    ReplaceAll( assetsFolder, "\\\\", "\\" );

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
    apemode::LogInfo( "CSO JSON dump: {}", csoJson.dump( ).c_str( ) );

    if ( !csoJson.is_object( ) ) {
        apemode::LogError( "Parsing error." );
        return 1;
    }

    assert( csoJson[ "commands" ].is_array( ) );
    if ( !csoJson[ "commands" ].is_array( ) ) {
        apemode::LogError( "Parsing error." );
        return 1;
    }

    apemode::shp::ShaderCompiler shaderCompiler;
    apemode::shp::ShaderFileReader shaderCompilerFileReader;
    apemode::shp::ShaderFeedbackWriter shaderFeedbackWriter;
    shaderCompilerFileReader.mAssetManager = &assetManager;
    shaderCompiler.SetShaderFileReader( &shaderCompilerFileReader );
    shaderCompiler.SetShaderFeedbackWriter( &shaderFeedbackWriter );

    flatbuffers::FlatBufferBuilder builder;
    std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets;

    const json& commandsJson = csoJson[ "commands" ];
    for ( const auto& commandJson : commandsJson ) {
        const flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset =
            CompileShader( builder, assetManager, shaderCompiler, commandJson, outputFolder );

        if ( !csoOffset.IsNull( ) ) {
            csoOffsets.push_back( csoOffset );
        }
    }

    flatbuffers::Offset< flatbuffers::Vector< flatbuffers::Offset< csofb::CompiledShaderFb > > > csosOffset;
    csosOffset = builder.CreateVector( csoOffsets );

    flatbuffers::Offset< csofb::CollectionFb > collectionFb;
    collectionFb = csofb::CreateCollectionFb( builder, csofb::EVersionFb_Value, csosOffset );

    csofb::FinishCollectionFbBuffer( builder, collectionFb );

    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert( csofb::VerifyCollectionFbBuffer( v ) );

    auto csoBuffer = (const char*) builder.GetBufferPointer( );
    auto csoBufferSize = (size_t) builder.GetSize( );
    
    if ( !flatbuffers::SaveFile( outputFile.c_str( ), csoBuffer, csoBufferSize, true ) ) {
        apemode::LogError( "Failed to save file." );
        return 1;
    }

    apemode::AppState::OnExit( );
    return 0;
}
