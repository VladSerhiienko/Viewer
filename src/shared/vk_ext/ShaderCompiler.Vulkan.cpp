#include <ShaderCompiler.Vulkan.h>

#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>

struct apemodevk::ShaderCompiler::Impl {
    shaderc::Compiler                                 Compiler;
    apemodevk::ShaderCompiler::IShaderFileReader*     pShaderFileReader     = nullptr;
    apemodevk::ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter = nullptr;
};

class CompiledShader : public apemodevk::ICompiledShader {
public:
    std::vector< uint32_t >   Dwords;
    spirv_cross::CompilerGLSL Glsl;

    CompiledShader( std::vector< uint32_t > dwords ) : Dwords( dwords ), Glsl( dwords ) {
    }

    const spirv_cross::CompilerGLSL& GetGlsl( ) const override {
        return Glsl;
    }

    const uint8_t* GetBytePtr( ) const override {
        return reinterpret_cast< const uint8_t* >( Dwords.data( ) );
    }

    size_t GetByteCount( ) const override {
        return Dwords.size( ) << 2;
    }
};

class Includer : public shaderc::CompileOptions::IncluderInterface {
public:
    struct UserData {
        std::string Content;
        std::string Path;
    };

    apemodevk::ShaderCompiler::IIncludedFileSet*  pIncludedFiles;
    apemodevk::ShaderCompiler::IShaderFileReader& FileReader;

    Includer( apemodevk::ShaderCompiler::IShaderFileReader& FileReader,
              apemodevk::ShaderCompiler::IIncludedFileSet*  pIncludedFiles )
        : FileReader( FileReader ), pIncludedFiles( pIncludedFiles ) {
    }

    // Handles shaderc_include_resolver_fn callbacks.
    shaderc_include_result* GetInclude( const char*          pszRequestedSource,
                                        shaderc_include_type eShaderIncludeType,
                                        const char*          pszRequestingSource,
                                        size_t               includeDepth ) {

        auto userData = apemodevk::make_unique< UserData >( );
        if ( userData && pIncludedFiles && FileReader.ReadShaderTxtFile( pszRequestedSource, userData->Path, userData->Content ) ) {
            pIncludedFiles->InsertIncludedFile( userData->Path );

            auto includeResult                = apemodevk::make_unique< shaderc_include_result >( );
            includeResult->content            = userData->Content.data( );
            includeResult->content_length     = userData->Content.size( );
            includeResult->source_name        = userData->Path.data( );
            includeResult->source_name_length = userData->Path.size( );
            includeResult->user_data          = userData.release( );
            return includeResult.release( );
        }

        return nullptr;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude( shaderc_include_result* data ) {
        apemodevk_delete( data->user_data );
        apemodevk_delete( data );
        // apemodevk_delete ((UserData*) data->user_data);
        // apemodevk_delete data;
    }
};

apemodevk::ShaderCompiler::ShaderCompiler( ) : pImpl( apemodevk_new Impl( ) ) {
}

apemodevk::ShaderCompiler::~ShaderCompiler( ) {
    apemodevk_delete( pImpl );
}

apemodevk::ShaderCompiler::IShaderFileReader* apemodevk::ShaderCompiler::GetShaderFileReader( ) {
    return pImpl->pShaderFileReader;
}

apemodevk::ShaderCompiler::IShaderFeedbackWriter* apemodevk::ShaderCompiler::GetShaderFeedbackWriter( ) {
    return pImpl->pShaderFeedbackWriter;
}

void apemodevk::ShaderCompiler::SetShaderFileReader( IShaderFileReader* pShaderFileReader ) {
    pImpl->pShaderFileReader = pShaderFileReader;
}

void apemodevk::ShaderCompiler::SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter ) {
    pImpl->pShaderFeedbackWriter = pShaderFeedbackWriter;
}

static apemodevk::unique_ptr< apemodevk::ICompiledShader > InternalCompile(
    const std::string&                                           shaderName,
    const std::string&                                           shaderContent,
    const apemodevk::ShaderCompiler::IMacroDefinitionCollection* pMacros,
    const apemodevk::ShaderCompiler::EShaderType                 eShaderKind,
    shaderc::CompileOptions&                                     options,
    const bool                                                   bAssembly,
    shaderc::Compiler*                                           pCompiler,
    apemodevk::ShaderCompiler::IShaderFeedbackWriter*            pShaderFeedbackWriter ) {
    using namespace apemodevk;

    if ( nullptr == pCompiler ) {
        return nullptr;
    }

    if ( pMacros )
        for ( uint32_t i = 0; i < pMacros->GetCount( ); ++i ) {
            const auto macroDefinition = pMacros->GetMacroDefinition( i );
            options.AddMacroDefinition( std::string( macroDefinition.pszKey ), std::string( macroDefinition.pszValue ) );
        }

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pCompiler->PreprocessGlsl( shaderContent, (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pShaderFeedbackWriter ) {
            pShaderFeedbackWriter->WriteFeedback(
                apemodevk::ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed | preprocessedSourceCompilationResult.GetCompilationStatus( ),
                shaderName,
                pMacros,
                preprocessedSourceCompilationResult.GetErrorMessage( ).data( ),
                preprocessedSourceCompilationResult.GetErrorMessage( ).data( ) + preprocessedSourceCompilationResult.GetErrorMessage( ).size( )  );
        }

        platform::DebugBreak( );
        return nullptr;
    }

    if ( nullptr != pShaderFeedbackWriter ) {
        platform::DebugTrace( platform::LogLevel::Info, "ShaderCompiler: Preprocessed GLSL." );

        pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_PreprocessingSucceeded,
                                              shaderName,
                                              pMacros,
                                              preprocessedSourceCompilationResult.cbegin( ),
                                              preprocessedSourceCompilationResult.cend( ) );
    }

    if ( bAssembly ) {
        shaderc::AssemblyCompilationResult assemblyCompilationResult = pCompiler->CompileGlslToSpvAssembly(
            preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

        if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
            platform::DebugTrace( platform::LogLevel::Err,
                                  "ShaderCompiler: Failed to compile processed GLSL to SPV assembly: %s.",
                                  assemblyCompilationResult.GetErrorMessage( ).c_str( ) );

            if ( nullptr != pShaderFeedbackWriter ) {
                pShaderFeedbackWriter->WriteFeedback(
                    ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly | assemblyCompilationResult.GetCompilationStatus( ),
                    shaderName,
                    pMacros,
                    assemblyCompilationResult.GetErrorMessage( ).data( ),
                    assemblyCompilationResult.GetErrorMessage( ).data( ) + assemblyCompilationResult.GetErrorMessage( ).size( ) );
            }

            platform::DebugBreak( );
            return nullptr;
        }

        if ( nullptr != pShaderFeedbackWriter ) {
            platform::DebugTrace( platform::LogLevel::Info, "ShaderCompiler: Compiled GLSL to SPV assembly." );

            pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_AssemblySucceeded,
                                                  shaderName,
                                                  pMacros,
                                                  assemblyCompilationResult.cbegin( ),
                                                  assemblyCompilationResult.cend( ) );
        }
    }

    shaderc::SpvCompilationResult spvCompilationResult = pCompiler->CompileGlslToSpv(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pShaderFeedbackWriter ) {
            platform::DebugTrace( platform::LogLevel::Err,
                                  "ShaderCompiler: Failed to compile processed GLSL to SPV: %s.",
                                  spvCompilationResult.GetErrorMessage( ).c_str( ) );

            pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv | spvCompilationResult.GetCompilationStatus( ),
                                                  shaderName,
                                                  pMacros,
                                                  spvCompilationResult.GetErrorMessage( ).data( ),
                                                  spvCompilationResult.GetErrorMessage( ).data( ) + spvCompilationResult.GetErrorMessage( ).size( ) );
        }

        platform::DebugBreak( );
        return nullptr;
    }

    if ( nullptr != pShaderFeedbackWriter ) {
        platform::DebugTrace( platform::LogLevel::Info, "ShaderCompiler: Compiled GLSL to SPV." );

        pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_SpvSucceeded,
                                              shaderName,
                                              pMacros,
                                              spvCompilationResult.cbegin( ),
                                              spvCompilationResult.cend( ) );
    }

    const size_t dwordCount = (size_t) std::distance( spvCompilationResult.cbegin( ), spvCompilationResult.cend( ) );
    const size_t byteCount  = dwordCount * sizeof( shaderc::SpvCompilationResult::element_type );

    std::vector< uint32_t > dwords;
    dwords.resize( dwordCount );
    memcpy( dwords.data( ), spvCompilationResult.cbegin( ), byteCount );

    return apemodevk::make_unique< CompiledShader >( dwords );
}

apemodevk::unique_ptr< apemodevk::ICompiledShader > apemodevk::ShaderCompiler::Compile(
    const std::string&                shaderName,
    const std::string&                shaderContent,
    const IMacroDefinitionCollection* pMacros,
    const EShaderType                 eShaderKind,
    const EShaderOptimizationType     eShaderOptimization ) {

    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level( eShaderOptimization ) );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );

    return InternalCompile( shaderName,
                            shaderContent,
                            pMacros,
                            eShaderKind,
                            options,
                            true,
                            &pImpl->Compiler,
                            pImpl->pShaderFeedbackWriter );
}

apemodevk::unique_ptr< apemodevk::ICompiledShader > apemodevk::ShaderCompiler::Compile(
    const std::string&                InFilePath,
    const IMacroDefinitionCollection* pMacros,
    const EShaderType                 eShaderKind,
    const EShaderOptimizationType     eShaderOptimization,
    IIncludedFileSet*                 pOutIncludedFiles ) {

    if ( nullptr == pImpl->pShaderFileReader ) {
        platform::DebugTrace( platform::LogLevel::Err, "ShaderCompiler: pShaderFileReader must be set." );
        platform::DebugBreak( );
        return nullptr;
    }

    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level( eShaderOptimization ) );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );
    options.SetIncluder( apemodevk::std_make_unique< Includer >( *pImpl->pShaderFileReader, pOutIncludedFiles ) );

    std::string fullPath;
    std::string fileContent;

    if ( pImpl->pShaderFileReader->ReadShaderTxtFile( InFilePath, fullPath, fileContent ) ) {
        auto compiledShader = InternalCompile( fullPath,
                                               fileContent,
                                               pMacros,
                                               eShaderKind,
                                               options,
                                               true,
                                               &pImpl->Compiler,
                                               pImpl->pShaderFeedbackWriter );
        if ( compiledShader ) {
            pOutIncludedFiles->InsertIncludedFile( fullPath );
            return std::move( compiledShader );
        }
    }

    return nullptr;
}
