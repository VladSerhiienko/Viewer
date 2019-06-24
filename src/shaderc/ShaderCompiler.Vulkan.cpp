#include "ShaderCompiler.Vulkan.h"
#include <apemode/platform/AppState.h>

using namespace apemode::shp;

class CompiledShader : public ICompiledShader {
public:
    apemode::vector< uint32_t > Dwords;
    apemode::string8            PreprocessedSrc;
    apemode::string8            AssemblySrc;
    // spirv_cross::CompilerGLSL Glsl;

    CompiledShader( apemode::vector< uint32_t >&& dwords, apemode::string8&& preprocessedSrc, apemode::string8&& assemblySrc )
        : Dwords( std::move( dwords ) )
        , PreprocessedSrc( std::move( preprocessedSrc ) )
        , AssemblySrc( std::move( assemblySrc ) ) { // , Glsl( dwords ) {
        apemode::LogInfo( "CompiledShader: Created." );
    }

    ~CompiledShader( ) {
        apemode::LogInfo( "CompiledShader: Destroying." );
    }

    std::string_view GetPreprocessedSrc( ) const override {
        std::string_view sv{PreprocessedSrc.c_str(), PreprocessedSrc.size()};
        return sv;
    }

    std::string_view GetAssemblySrc( ) const override {
        std::string_view sv{AssemblySrc.c_str(), AssemblySrc.size()};
        return sv;
    }

    //    const spirv_cross::CompilerGLSL& GetGlsl( ) const override {
    //        return Glsl;
    //    }

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

    IShaderCompiler::IShaderFileReader& FileReader;
    IShaderCompiler::IIncludedFileSet*  pIncludedFiles;

    Includer( IShaderCompiler::IShaderFileReader& fileReader, IShaderCompiler::IIncludedFileSet* pIncludedFiles )
        : FileReader( fileReader ), pIncludedFiles( pIncludedFiles ) {
    }

    // Handles shaderc_include_resolver_fn callbacks.
    shaderc_include_result* GetInclude( const char*          pszRequestedSource,
                                        shaderc_include_type eShaderIncludeType,
                                        const char*          pszRequestingSource,
                                        size_t               includeDepth ) {
        apemode_memory_allocation_scope;

        auto userData = apemode::make_unique< UserData >( );
        if ( userData && pIncludedFiles &&
             FileReader.ReadShaderTxtFile( pszRequestedSource, userData->Path, userData->Content ) ) {
            pIncludedFiles->InsertIncludedFile( userData->Path );

            auto includeResult                = apemode::make_unique< shaderc_include_result >( );
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
        apemode_memory_allocation_scope;
        apemode_delete( ( (UserData*&) data->user_data ) );
        apemode_delete( data );
    }
};

namespace {
/* Wrapper for shaderc lib */
class ShaderCompiler : public apemode::shp::IShaderCompiler {
public:
public:
    ShaderCompiler( );
    virtual ~ShaderCompiler( );

    /* @note No files, only ready to compile shader sources */

    apemode::unique_ptr< ICompiledShader > Compile( const std::string&                ShaderName,
                                                    const std::string&                ShaderCode,
                                                    const IMacroDefinitionCollection* pMacros,
                                                    EShaderType                       eShaderKind,
                                                    EShaderOptimizationType           eShaderOptimization ) const override;

    /* @note Compiling from source files */

    IShaderFileReader*     GetShaderFileReader( ) override;
    IShaderFeedbackWriter* GetShaderFeedbackWriter( ) override;
    void                   SetShaderFileReader( IShaderFileReader* pShaderFileReader ) override;
    void                   SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter ) override;

    apemode::unique_ptr< ICompiledShader > Compile( const std::string&                FilePath,
                                                    const IMacroDefinitionCollection* pMacros,
                                                    EShaderType                       eShaderKind,
                                                    EShaderOptimizationType           eShaderOptimization,
                                                    IIncludedFileSet*                 pOutIncludedFiles ) const override;

private:
    shaderc::Compiler      Compiler;
    IShaderFileReader*     pShaderFileReader     = nullptr;
    IShaderFeedbackWriter* pShaderFeedbackWriter = nullptr;
};
} // namespace

ShaderCompiler::ShaderCompiler( ) {
}

ShaderCompiler::~ShaderCompiler( ) {
}

ShaderCompiler::IShaderFileReader* ShaderCompiler::GetShaderFileReader( ) {
    return pShaderFileReader;
}

ShaderCompiler::IShaderFeedbackWriter* ShaderCompiler::GetShaderFeedbackWriter( ) {
    return pShaderFeedbackWriter;
}

void ShaderCompiler::SetShaderFileReader( IShaderFileReader* pInShaderFileReader ) {
    pShaderFileReader = pInShaderFileReader;
}

void ShaderCompiler::SetShaderFeedbackWriter( IShaderFeedbackWriter* pInShaderFeedbackWriter ) {
    pShaderFeedbackWriter = pInShaderFeedbackWriter;
}

static apemode::unique_ptr< apemode::shp::ICompiledShader > InternalCompile(
    const std::string&                                 shaderName,
    const std::string&                                 shaderContent,
    const IShaderCompiler::IMacroDefinitionCollection* pMacros,
    const IShaderCompiler::EShaderType                 eShaderKind,
    shaderc::CompileOptions&                           options,
    const bool                                         bAssembly,
    const shaderc::Compiler*                           pCompiler,
    ShaderCompiler::IShaderFeedbackWriter*             pShaderFeedbackWriter ) {
    using namespace apemode::shp;
    apemode_memory_allocation_scope;

    if ( nullptr == pCompiler ) {
        return nullptr;
    }

    if ( pMacros )
        for ( uint32_t i = 0; i < pMacros->GetCount( ); ++i ) {
            const auto macroDefinition = pMacros->GetMacroDefinition( i );
            options.AddMacroDefinition( std::string( macroDefinition.pszKey ), std::string( macroDefinition.pszValue ) );

            apemode::LogInfo( "ShaderCompiler: Adding definition: {}={}", macroDefinition.pszKey, macroDefinition.pszValue );
        }

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pCompiler->PreprocessGlsl( shaderContent, (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pShaderFeedbackWriter ) {
            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                    preprocessedSourceCompilationResult.GetCompilationStatus( ),
                shaderName,
                pMacros,
                preprocessedSourceCompilationResult.GetErrorMessage( ).data( ),
                preprocessedSourceCompilationResult.GetErrorMessage( ).data( ) +
                    preprocessedSourceCompilationResult.GetErrorMessage( ).size( ) );
        }

        assert( false );
        return nullptr;
    }

    if ( nullptr != pShaderFeedbackWriter ) {
        apemode::LogInfo( "ShaderCompiler: Preprocessed GLSL." );

        pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_PreprocessingSucceeded,
                                              shaderName,
                                              pMacros,
                                              preprocessedSourceCompilationResult.cbegin( ),
                                              preprocessedSourceCompilationResult.cend( ) );
    }

    shaderc::AssemblyCompilationResult assemblyCompilationResult = pCompiler->CompileGlslToSpvAssembly(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
        apemode::LogError( "ShaderCompiler: Failed to compile processed GLSL to SPV assembly: {}.",
                           assemblyCompilationResult.GetErrorMessage( ).c_str( ) );

        if ( nullptr != pShaderFeedbackWriter ) {
            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                    assemblyCompilationResult.GetCompilationStatus( ),
                shaderName,
                pMacros,
                assemblyCompilationResult.GetErrorMessage( ).data( ),
                assemblyCompilationResult.GetErrorMessage( ).data( ) + assemblyCompilationResult.GetErrorMessage( ).size( ) );
        }

        assert( false );
        return nullptr;
    }

    if ( nullptr != pShaderFeedbackWriter ) {
        apemode::LogInfo( "ShaderCompiler: Compiled GLSL to SPV assembly." );

        pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_AssemblySucceeded,
                                              shaderName,
                                              pMacros,
                                              assemblyCompilationResult.cbegin( ),
                                              assemblyCompilationResult.cend( ) );
    }

    shaderc::SpvCompilationResult spvCompilationResult = pCompiler->CompileGlslToSpv(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, shaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pShaderFeedbackWriter ) {
            apemode::LogInfo( "ShaderCompiler: Failed to compile processed GLSL to SPV: {}.",
                              spvCompilationResult.GetErrorMessage( ).c_str( ) );

            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                    spvCompilationResult.GetCompilationStatus( ),
                shaderName,
                pMacros,
                spvCompilationResult.GetErrorMessage( ).data( ),
                spvCompilationResult.GetErrorMessage( ).data( ) + spvCompilationResult.GetErrorMessage( ).size( ) );
        }

        assert( false );
        return nullptr;
    }

    if ( nullptr != pShaderFeedbackWriter ) {
        apemode::LogInfo( "ShaderCompiler: Compiled GLSL to SPV." );

        pShaderFeedbackWriter->WriteFeedback( ShaderCompiler::IShaderFeedbackWriter::eFeedback_SpvSucceeded,
                                              shaderName,
                                              pMacros,
                                              spvCompilationResult.cbegin( ),
                                              spvCompilationResult.cend( ) );
    }

    apemode::vector< uint32_t > dwords( spvCompilationResult.cbegin( ), spvCompilationResult.cend( ) );

    apemode::string8 preprocessedSrc( preprocessedSourceCompilationResult.cbegin( ),
                                      preprocessedSourceCompilationResult.cend( ) );

    apemode::string8 assemblySrc( assemblyCompilationResult.cbegin( ), assemblyCompilationResult.cend( ) );

    ICompiledShader* pCompiledShader =
        apemode_new  CompiledShader( std::move( dwords ), std::move( preprocessedSrc ), std::move( assemblySrc ) );

    return apemode::unique_ptr< ICompiledShader >( pCompiledShader );
}

apemode::unique_ptr< apemode::shp::ICompiledShader > ShaderCompiler::Compile(
    const std::string&                shaderName,
    const std::string&                shaderContent,
    const IMacroDefinitionCollection* pMacros,
    const EShaderType                 eShaderKind,
    const EShaderOptimizationType     eShaderOptimization ) const {
    apemode_memory_allocation_scope;

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
                            /* impl */ &Compiler,
                            /* impl */ pShaderFeedbackWriter );
}

apemode::unique_ptr< apemode::shp::ICompiledShader > ShaderCompiler::Compile( const std::string&                InFilePath,
                                                                              const IMacroDefinitionCollection* pMacros,
                                                                              const EShaderType                 eShaderKind,
                                                                              const EShaderOptimizationType eShaderOptimization,
                                                                              IIncludedFileSet* pOutIncludedFiles ) const {
    apemode_memory_allocation_scope;

    if ( nullptr == /* impl */ pShaderFileReader ) {
        apemode::LogError( "ShaderCompiler: pShaderFileReader must be set." );
        assert( false );
        return nullptr;
    }

    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level( eShaderOptimization ) );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );
    options.SetIncluder( std::make_unique< Includer >( /* impl */ *pShaderFileReader, pOutIncludedFiles ) );

    std::string fullPath;
    std::string fileContent;

    apemode::LogInfo( "ShaderCompiler: Compiling {}", InFilePath );

    if ( /* impl */ pShaderFileReader->ReadShaderTxtFile( InFilePath, fullPath, fileContent ) ) {
        auto compiledShader = InternalCompile( fullPath,
                                               fileContent,
                                               pMacros,
                                               eShaderKind,
                                               options,
                                               true,
                                               /* impl */ &Compiler,
                                               /* impl */ pShaderFeedbackWriter );
        if ( compiledShader ) {
            pOutIncludedFiles->InsertIncludedFile( fullPath );
            return compiledShader;
        }
    }

    return nullptr;
}

apemode::unique_ptr< IShaderCompiler > apemode::shp::NewShaderCompiler( ) {
    return apemode::unique_ptr< IShaderCompiler >( apemode_new ShaderCompiler{} );
}
