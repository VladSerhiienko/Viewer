
#include <nlohmann/json.hpp>
#include <apemode/platform/AppState.h>
#include <apemode/platform/shared/AssetManager.h>

#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"
#include <flatbuffers/util.h>

using json = nlohmann::json;

using ShaderCompiler = apemode::shp::ShaderCompiler;

class ShaderFileReader : public ShaderCompiler::IShaderFileReader {
public:
    apemode::platform::IAssetManager* mAssetManager;

    bool ReadShaderTxtFile( const std::string& FilePath,
                            std::string&       OutFileFullPath,
                            std::string&       OutFileContent ) override;
};

class ShaderFeedbackWriter : public ShaderCompiler::IShaderFeedbackWriter {
public:
    void WriteFeedback( EFeedbackType                                     eType,
                        const std::string&                                FullFilePath,
                        const ShaderCompiler::IMacroDefinitionCollection* Macros,
                        const void*                                       pContent, /* Txt or bin, @see EFeedbackType */
                        const void*                                       pContentEnd ) override;
};


struct EFeedbackTypeWithOStream {
    ShaderCompiler::IShaderFeedbackWriter::EFeedbackType e;

    EFeedbackTypeWithOStream( ShaderCompiler::IShaderFeedbackWriter::EFeedbackType e ) : e( e ) {
    }

    // clang-format off
    template < typename OStream >
    friend OStream& operator<<( OStream& os, const EFeedbackTypeWithOStream& feedbackType ) {
        switch ( feedbackType.e ) {
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly:                 return os << "Assembly";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed:             return os << "Preprocessed";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_PreprocessedOptimized:    return os << "PreprocessedOptimized";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv:                      return os << "Spv";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_CompilationError:        return os << "CompilationError";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InternalError:           return os << "InternalError";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InvalidAssembly:         return os << "InvalidAssembly";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_InvalidStage:            return os << "InvalidStage";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_NullResultObject:        return os << "NullResultObject";
        case ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success:                 return os << "Success";
        default:                                                                                             return os;
        }
    }
    // clang-format on
};

bool ShaderFileReader::ReadShaderTxtFile( const std::string& InFilePath,
    std::string&       OutFileFullPath,
    std::string&       OutFileContent ) {
    apemode_memory_allocation_scope;
    if ( auto pAsset = mAssetManager->Acquire( InFilePath.c_str( ) ) ) {
        const auto assetText = pAsset->GetContentAsTextBuffer( );
        OutFileContent = reinterpret_cast< const char* >( assetText.data() );
        OutFileFullPath = pAsset->GetId( );
        mAssetManager->Release( pAsset );
        return true;
    }

    // apemodevk::platform::DebugBreak( );
    return false;
}

void ShaderFeedbackWriter::WriteFeedback( EFeedbackType                                     eType,
    const std::string&                                FullFilePath,
    const ShaderCompiler::IMacroDefinitionCollection* pMacros,
    const void*                                       pContent,
    const void*                                       pContentEnd ) {
    apemode_memory_allocation_scope;

    const auto feedbackStage            = eType & eFeedbackType_CompilationStageMask;
    const auto feedbackCompilationError = eType & eFeedbackType_CompilationStatusMask;

    if ( eFeedbackType_CompilationStatus_Success != feedbackCompilationError ) {
        apemode::LogError( "ShaderCompiler: {} / {} / {}",
                            EFeedbackTypeWithOStream( feedbackStage ),
                            EFeedbackTypeWithOStream( feedbackCompilationError ),
                            FullFilePath );
        apemode::LogError( "           Msg: {}", (const char*) pContent );
        // apemode::platform::DebugBreak( );
    } else {
        apemode::LogInfo(   "ShaderCompiler: {} / {} / {}",
                            EFeedbackTypeWithOStream( feedbackStage ),
                            EFeedbackTypeWithOStream( feedbackCompilationError ),
                            FullFilePath );
        // TODO: Store compiled shader to file system
    }
}

std::string GetMacrosString(std::map<std::string, std::string> macros) {
    if (macros.empty())
        return "";
    
    std::string macrosString;
    for (const auto& macro : macros) {
        macrosString += macro.first;
        macrosString += "=";
        macrosString += macro.second;
        macrosString += ";";
    }
    
    macrosString.erase(macrosString.size() - 1);
    return macrosString;
}

void ReplaceAll(std::string & data, std::string toSearch, std::string replaceStr) {
    size_t pos = data.find(toSearch);
    while( pos != std::string::npos) {
        data.replace(pos, toSearch.size(), replaceStr);
        pos =data.find(toSearch, pos + replaceStr.size());
    }
}

int main(int argc, char ** argv) {
    apemode::AppState::OnMain(argc, (const char **)argv);

    std::string assetsFolder = apemode::TGetOption("assets-folder", std::string());
    if (assetsFolder.empty()) {
        apemode::LogError("No assets folder.");
        return 1;
    }
    
    std::string csoJsonFile = apemode::TGetOption("cso-json-file", std::string());
    if (csoJsonFile.empty() || !apemode::platform::shared::FileExists(csoJsonFile.c_str())) {
        apemode::LogError("No CSO file.");
        return 1;
    }
    
    auto csoJsonContents = apemode::platform::shared::FileReader().ReadTxtFile(csoJsonFile.c_str());
    if (csoJsonFile.empty()) {
        apemode::LogError("CSO file is empty.");
        return 1;
    }
    
    apemode::platform::shared::AssetManager assetManager;
    assetManager.UpdateAssets(assetsFolder.c_str(), nullptr, 0);
    
    const json csoJson = json::parse((const char *)csoJsonContents.data());
    apemode::LogInfo("CSO: {}", csoJson.dump().c_str());
    
    if(!csoJson.is_object()) {
        apemode::LogError("Parsing error.");
        return 1;
    }
    
    std::string outputFile = csoJson["output"].get<std::string>();
    if (outputFile.empty()) {
        apemode::LogError("Output CSO file is empty.");
        return 1;
    }
    
    assert(csoJson["commands"].is_array());
    if(!csoJson["commands"].is_array()) {
        apemode::LogError("Parsing error.");
        return 1;
    }
    
    ShaderCompiler shaderCompiler;
    ShaderFileReader shaderCompilerFileReader;
    ShaderFeedbackWriter shaderFeedbackWriter;
    shaderCompilerFileReader.mAssetManager = &assetManager;
    shaderCompiler.SetShaderFileReader(&shaderCompilerFileReader);
    shaderCompiler.SetShaderFeedbackWriter(&shaderFeedbackWriter);
    
    flatbuffers::FlatBufferBuilder builder;
    std::vector< flatbuffers::Offset< csofb::CompiledShaderFb > > csoOffsets;
    
    const json& commandsJson = csoJson["commands"];
    for (const auto& commandJson : commandsJson) {
        assert(commandJson["srcFile"].is_string());
        std::string srcFile = commandJson["srcFile"].get<std::string>();
        apemode::LogInfo("srcFile: {}", srcFile.c_str());
        
        if (auto acquiredSrcFile = assetManager.Acquire(srcFile.c_str())) {
            apemode::LogInfo("assetId: {}", acquiredSrcFile->GetId());
            
            std::map<std::string, std::string> macroDefinitions;
            if (commandJson["macros"].is_array()) {
                const json& macrosJson = commandJson["macros"];
                for (const auto& macroJson : macrosJson) {
                    assert(macroJson.is_object());
                    assert(macroJson["name"].is_string());
                    
                    std::string macroName = macroJson["name"].get<std::string>();
                    macroDefinitions[macroName] = "1";
                    
                    if (macroJson["value"].is_boolean()) {
                        if (false == macroJson["value"].get<bool>()) {
                            macroDefinitions[macroName] = "0";
                        }
                    } else if (macroJson["value"].is_number_integer()) {
                        macroDefinitions[macroName] = std::to_string(macroJson["value"].get<int>());
                    } else if (macroJson["value"].is_number_unsigned()) {
                        macroDefinitions[macroName] = std::to_string(macroJson["value"].get<unsigned>());
                    } else if (macroJson["value"].is_number_float()) {
                        macroDefinitions[macroName] = std::to_string(macroJson["value"].get<int>());
                    }
                }
            }
            
            apemode::shp::ShaderCompilerMacroDefinitionCollection concreteMacros;
            apemode::shp::ShaderCompiler::IMacroDefinitionCollection* opaqueMacros = nullptr;
            if (!macroDefinitions.empty()) {
                concreteMacros.Init(macroDefinitions);
                opaqueMacros = &concreteMacros;
            }
            
            std::string macrosString = GetMacrosString(macroDefinitions);
            
            apemode::shp::ShaderCompiler::EShaderType eShaderType;
            csofb::EShaderType eShaderTypeFb;
            
            eShaderType = apemode::shp::ShaderCompiler::eShaderType_FragmentShader;
            eShaderTypeFb = csofb::EShaderType_Frag;
            
            assert(commandJson["shaderType"].is_string());
            const std::string shaderType = commandJson["shaderType"].get<std::string>();
            if (shaderType == "vert") {
                eShaderType = apemode::shp::ShaderCompiler::eShaderType_VertexShader;
                eShaderTypeFb = csofb::EShaderType_Vert;
            } else if (shaderType == "frag") {
                eShaderType = apemode::shp::ShaderCompiler::eShaderType_FragmentShader;
                eShaderTypeFb = csofb::EShaderType_Frag;
            } else if (shaderType == "comp") {
                eShaderType = apemode::shp::ShaderCompiler::eShaderType_ComputeShader;
                eShaderTypeFb = csofb::EShaderType_Comp;
            } else if (shaderType == "tesc") {
                eShaderType = apemode::shp::ShaderCompiler::eShaderType_TessControlShader;
                eShaderTypeFb = csofb::EShaderType_Tesc;
            } else if (shaderType == "tese") {
                eShaderType = apemode::shp::ShaderCompiler::eShaderType_TessEvaluationShader;
                eShaderTypeFb = csofb::EShaderType_Tese;
            } else {
                apemode::LogError("Shader type should be one of there: vert, frag, comp, tesc, tese, not this: {}", shaderType);
                return 1;
            }

            apemode::shp::ShaderCompilerIncludedFileSet includedFileSet;
            auto compiledShader = shaderCompiler.Compile(srcFile, opaqueMacros, eShaderType, apemode::shp::ShaderCompiler::eShaderOptimization_Performance, &includedFileSet);
            
            std::vector<uint32_t> spv;
            spv.assign(compiledShader->GetDwordPtr(), compiledShader->GetDwordPtr() + compiledShader->GetDwordCount());
            
            flatbuffers::Offset< flatbuffers::String > assetOffsetFb {};
            flatbuffers::Offset< flatbuffers::String > macrosOffsetFb {};
            flatbuffers::Offset< flatbuffers::Vector< uint32_t > > spvOffsetFb {};
            
            assetOffsetFb = builder.CreateString(srcFile);
            spvOffsetFb = builder.CreateVector(spv);
            
            if (macrosString.size()) {
                macrosOffsetFb = builder.CreateString(macrosString);
            }
            
            flatbuffers::Offset< csofb::CompiledShaderFb > csoOffset;
            csoOffset = csofb::CreateCompiledShaderFb(builder, assetOffsetFb, macrosOffsetFb, eShaderTypeFb, spvOffsetFb);
            csoOffsets.push_back(csoOffset);
            
            assetManager.Release(acquiredSrcFile);
        } else {
            apemode::LogError("Asset not found.");
        }
    }
    
    outputFile = assetsFolder + "/" + outputFile;
    
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<csofb::CompiledShaderFb>>> csosOffset;
    csosOffset = builder.CreateVector(csoOffsets);
    
    flatbuffers::Offset< csofb::CollectionFb > collectionFb;
    collectionFb = csofb::CreateCollectionFb(builder, csofb::EVersionFb_Value, csosOffset);

    csofb::FinishCollectionFbBuffer( builder, collectionFb);
    
    flatbuffers::Verifier v( builder.GetBufferPointer( ), builder.GetSize( ) );
    assert(  csofb::VerifyCollectionFbBuffer( v ) );
    
    ReplaceAll(outputFile, "*", "");
    ReplaceAll(outputFile, "//", "/");
    ReplaceAll(outputFile, "\\\\", "\\");
    
    if ( !flatbuffers::SaveFile( outputFile.c_str(), (const char*) builder.GetBufferPointer( ), (size_t) builder.GetSize( ), true ) ) {
       assert( false );
    }

    apemode::AppState::OnExit();
    return 0;
}
