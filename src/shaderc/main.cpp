
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>
#include <nlohmann/json.hpp>

#include <flatbuffers/util.h>
#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"

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

int main( int argc, char** argv ) {
    using json = nlohmann::json;
    apemode::AppState::OnMain( argc, (const char**) argv );

    std::string assetsFolderWildcard = apemode::TGetOption( "assets-folder", std::string( ) );
    if ( assetsFolderWildcard.empty( ) ) {
        apemode::LogError( "No assets folder." );
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

    std::string outputFile = csoJson[ "output" ].get< std::string >( );
    if ( outputFile.empty( ) ) {
        apemode::LogError( "Output CSO file is empty." );
        return 1;
    }

    assert( csoJson[ "commands" ].is_array( ) );
    if ( !csoJson[ "commands" ].is_array( ) ) {
        apemode::LogError( "Parsing error." );
        return 1;
    }

    apemode::shp::ShaderCompiler       shaderCompiler;
    apemode::shp::ShaderFileReader     shaderCompilerFileReader;
    apemode::shp::ShaderFeedbackWriter shaderFeedbackWriter;
    shaderCompilerFileReader.mAssetManager = &assetManager;
    shaderCompiler.SetShaderFileReader( &shaderCompilerFileReader );
    shaderCompiler.SetShaderFeedbackWriter( &shaderFeedbackWriter );

    flatbuffers::FlatBufferBuilder                                builder;
    std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets;

    const json& commandsJson = csoJson[ "commands" ];
    for ( const auto& commandJson : commandsJson ) {
        assert( commandJson[ "srcFile" ].is_string( ) );
        std::string srcFile = commandJson[ "srcFile" ].get< std::string >( );
        apemode::LogInfo( "srcFile: {}", srcFile.c_str( ) );

        if ( auto acquiredSrcFile = assetManager.Acquire( srcFile.c_str( ) ) ) {
            apemode::LogInfo( "assetId: {}", acquiredSrcFile->GetId( ) );

            std::map< std::string, std::string > macroDefinitions;

            auto macrosJsonIt = commandJson.find( "macros" );
            if ( macrosJsonIt != commandJson.end( ) && macrosJsonIt->is_array( ) ) {
                const json& macrosJson = *macrosJsonIt;
                for ( const auto& macroJson : macrosJson ) {
                    assert( macroJson.is_object( ) );
                    assert( macroJson[ "name" ].is_string( ) );

                    std::string macroName         = macroJson[ "name" ].get< std::string >( );
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
            }

            apemode::shp::ShaderCompilerMacroDefinitionCollection     concreteMacros;
            apemode::shp::ShaderCompiler::IMacroDefinitionCollection* opaqueMacros = nullptr;
            if ( !macroDefinitions.empty( ) ) {
                concreteMacros.Init( macroDefinitions );
                opaqueMacros = &concreteMacros;
            }

            std::string macrosString = GetMacrosString( macroDefinitions );

            apemode::shp::ShaderCompiler::EShaderType eShaderType;
            eShaderType = apemode::shp::ShaderCompiler::eShaderType_FragmentShader;

            csofb::EShaderType eShaderTypeFb;
            eShaderTypeFb = csofb::EShaderType_Frag;

            assert( commandJson[ "shaderType" ].is_string( ) );
            const std::string shaderType = commandJson[ "shaderType" ].get< std::string >( );
            if ( shaderType == "vert" ) {
                eShaderType   = apemode::shp::ShaderCompiler::eShaderType_VertexShader;
                eShaderTypeFb = csofb::EShaderType_Vert;
            } else if ( shaderType == "frag" ) {
                eShaderType   = apemode::shp::ShaderCompiler::eShaderType_FragmentShader;
                eShaderTypeFb = csofb::EShaderType_Frag;
            } else if ( shaderType == "comp" ) {
                eShaderType   = apemode::shp::ShaderCompiler::eShaderType_ComputeShader;
                eShaderTypeFb = csofb::EShaderType_Comp;
            } else if ( shaderType == "tesc" ) {
                eShaderType   = apemode::shp::ShaderCompiler::eShaderType_TessControlShader;
                eShaderTypeFb = csofb::EShaderType_Tesc;
            } else if ( shaderType == "tese" ) {
                eShaderType   = apemode::shp::ShaderCompiler::eShaderType_TessEvaluationShader;
                eShaderTypeFb = csofb::EShaderType_Tese;
            } else {
                apemode::LogError( "Shader type should be one of there: vert, frag, comp, tesc, tese, not this: {}",
                                   shaderType );
                return 1;
            }

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

                flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset;
                csoOffset = csofb::CreateCompiledShaderFb( builder, assetOffsetFb, macrosOffsetFb, eShaderTypeFb, spvOffsetFb );
                csoOffsets.push_back( csoOffset );
                
                ReplaceAll( macrosString, ".", "-" );
                ReplaceAll( macrosString, ";", "+" );
                
                std::string cachedCSO = assetsFolder + "/cso/" + srcFile + (macrosString.empty() ? "" : "-defs-") + macrosString + ".spv";
                ReplaceAll( cachedCSO, "//", "/" );
                ReplaceAll( cachedCSO, "\\/", "\\" );
                ReplaceAll( cachedCSO, "\\\\", "\\" );
                
                std::string cachedPreprocessed = cachedCSO + "-preprocessed.txt";
                std::string cachedAssembly = cachedCSO + "-assembly.txt";
    
                flatbuffers::SaveFile( cachedCSO.c_str( ),
                                       (const char*) compiledShader->GetDwordPtr( ),
                                       compiledShader->GetDwordCount( ),
                                       true );
                
                flatbuffers::SaveFile( cachedPreprocessed.c_str( ),
                                       compiledShader->GetPreprocessedSrc( ),
                                       strlen( compiledShader->GetPreprocessedSrc( ) ),
                                       false );
                
                flatbuffers::SaveFile( cachedAssembly.c_str( ),
                                       compiledShader->GetAssemblySrc( ),
                                       strlen( compiledShader->GetAssemblySrc( ) ),
                                       false );
            }

            assetManager.Release( acquiredSrcFile );
        } else {
            apemode::LogError( "Asset not found, the command \"{}\" skipped.", commandJson.dump( ).c_str( ) );
        }
    }

    outputFile = assetsFolder + "/" + outputFile;
    ReplaceAll( outputFile, "*", "" );
    ReplaceAll( outputFile, "//", "/" );
    ReplaceAll( outputFile, "\\/", "\\" );
    ReplaceAll( outputFile, "\\\\", "\\" );

    flatbuffers::Offset< flatbuffers::Vector< flatbuffers::Offset< csofb::CompiledShaderFb > > > csosOffset;
    csosOffset = builder.CreateVector( csoOffsets );

    flatbuffers::Offset< csofb::CollectionFb > collectionFb;
    collectionFb = csofb::CreateCollectionFb( builder, csofb::EVersionFb_Value, csosOffset );

    csofb::FinishCollectionFbBuffer( builder, collectionFb );

    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert( csofb::VerifyCollectionFbBuffer( v ) );


    if ( !flatbuffers::SaveFile( outputFile.c_str( ), (const char*) builder.GetBufferPointer( ), (size_t) builder.GetSize( ), true ) ) {
        apemode::LogError( "Failed to save file." );
        return 1;
    }

    apemode::AppState::OnExit( );
    return 0;
}
