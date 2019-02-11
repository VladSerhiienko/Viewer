#pragma once

#include <apemode/platform/AppState.h>
#include <apemode/platform/IAssetManager.h>

#include <shaderc/shaderc.hpp>
#ifdef APEMODE_ENABLE_SPIRV_GLSL
#include <spirv_glsl.hpp>
#endif

#include <assert.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace apemode {
namespace shp {

class ICompiledShader {
public:
    virtual ~ICompiledShader( ) = default;

    virtual const uint8_t* GetBytePtr( ) const = 0;
    virtual const char*    GetPreprocessedSrc( ) const = 0;
    virtual const char*    GetAssemblySrc( ) const = 0;
    virtual size_t         GetByteCount( ) const = 0;

#ifdef APEMODE_ENABLE_SPIRV_GLSL
    virtual const spirv_cross::CompilerGLSL& GetGlsl( ) const = 0;
#endif

    inline const uint32_t* GetDwordPtr( ) const {
        return reinterpret_cast< const uint32_t* >( GetBytePtr( ) );
    }

    inline size_t GetDwordCount( ) const {
        assert( GetByteCount( ) % 4 == 0 );
        return GetByteCount( ) >> 2;
    }
};

/* Wrapper for shaderc lib */
class ShaderCompiler {
public:
    struct MacroDefinition {
        const char* pszKey   = nullptr;
        const char* pszValue = nullptr;
    };

    class IMacroDefinitionCollection {
    public:
        virtual ~IMacroDefinitionCollection( )                                = default;
        virtual size_t          GetCount( ) const                             = 0;
        virtual MacroDefinition GetMacroDefinition( size_t macroIndex ) const = 0;
    };

    /* Simple interface to insert included files */
    class IIncludedFileSet {
    public:
        virtual ~IIncludedFileSet( )                                           = default;
        virtual void InsertIncludedFile( const std::string& includedFileName ) = 0;
    };

    /* Simple interface to read files from shader assets */
    class IShaderFileReader {
    public:
        virtual ~IShaderFileReader( )                                 = default;
        virtual bool ReadShaderTxtFile( const std::string& FilePath,
                                        std::string&       OutFileFullPath,
                                        std::string&       OutFileContent ) = 0;
    };

    /* Simple interface to read files from shader assets */
    class IShaderFeedbackWriter {
    public:
        enum EFeedbackTypeBits {
            /* Copy-paste from shaderc */
            eFeedbackType_CompilationStatus_Success = 0,
            eFeedbackType_CompilationStatus_InvalidStage, /* Error stage deduction */
            eFeedbackType_CompilationStatus_CompilationError,
            eFeedbackType_CompilationStatus_InternalError, /* Unexpected failure */
            eFeedbackType_CompilationStatus_NullResultObject,
            eFeedbackType_CompilationStatus_InvalidAssembly,
            eFeedbackType_CompilationStatusMask = 0xf,

            /* Compilation Stage */
            eFeedbackType_CompilationStage_Preprocessed          = 0x10, /* Txt */
            eFeedbackType_CompilationStage_PreprocessedOptimized = 0x20, /* Txt */
            eFeedbackType_CompilationStage_Assembly              = 0x30, /* Txt */
            eFeedbackType_CompilationStage_Spv                   = 0x40, /* Bin */
            eFeedbackType_CompilationStageMask                   = 0xf0,
        };

        using EFeedbackType = std::underlying_type< EFeedbackTypeBits >::type;

        static const EFeedbackType eFeedback_PreprocessingSucceeded =
            eFeedbackType_CompilationStage_Preprocessed | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_OptimizationSucceeded =
            eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_AssemblySucceeded =
            eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_SpvSucceeded =
            eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;

        /**
         * If CompilationStatus bit is not Success, pContent is an error message (Txt).
         * pContent is Bin in case CompilationStatus bit is success and Stage is Spv.
         **/
        virtual void WriteFeedback( EFeedbackType                     eType,
                                    const std::string&                FullFilePath,
                                    const IMacroDefinitionCollection* pMacros,
                                    const void*                       pContent, /* Txt or bin, @see EFeedbackType */
                                    const void*                       pContentEnd ) = 0;
    };

    /* Copy-paste from shaderc */
    enum EShaderType {
        // Forced shader kinds. These shader kinds force the compiler to compile the
        // source code as the specified kind of shader.
        eShaderType_VertexShader,
        eShaderType_FragmentShader,
        eShaderType_ComputeShader,
        eShaderType_GeometryShader,
        eShaderType_TessControlShader,
        eShaderType_TessEvaluationShader,

        eShaderType_GLSL_VertexShader         = eShaderType_VertexShader,
        eShaderType_GLSL_FragmentShader       = eShaderType_FragmentShader,
        eShaderType_GLSL_ComputeShader        = eShaderType_ComputeShader,
        eShaderType_GLSL_GeometryShader       = eShaderType_GeometryShader,
        eShaderType_GLSL_TessControlShader    = eShaderType_TessControlShader,
        eShaderType_GLSL_TessEvaluationShader = eShaderType_TessEvaluationShader,

        // Deduce the shader kind from #pragma annotation in the source code. Compiler
        // will emit error if #pragma annotation is not found.
        eShaderType_GLSL_InferFromSource,

        // Default shader kinds. Compiler will fall back to compile the source code as
        // the specified kind of shader when #pragma annotation is not found in the
        // source code.
        eShaderType_GLSL_Default_VertexShader,
        eShaderType_GLSL_Default_FragmentShader,
        eShaderType_GLSL_Default_ComputeShader,
        eShaderType_GLSL_Default_GeometryShader,
        eShaderType_GLSL_Default_TessControlShader,
        eShaderType_GLSL_Default_TessEvaluationShader,

        eShaderType_SPIRV_assembly,
    };

    /* Copy-paste from shaderc */
    enum EShaderOptimizationType {
        eShaderOptimization_None,        // no optimization
        eShaderOptimization_Size,        // optimize towards reducing code size
        eShaderOptimization_Performance, // optimize towards performance
    };

public:
    ShaderCompiler( );
    virtual ~ShaderCompiler( );

    /* @note No files, only ready to compile shader sources */

    virtual apemode::unique_ptr< ICompiledShader > Compile( const std::string&                ShaderName,
                                                            const std::string&                ShaderCode,
                                                            const IMacroDefinitionCollection* pMacros,
                                                            EShaderType                       eShaderKind,
                                                            EShaderOptimizationType           eShaderOptimization );

    /* @note Compiling from source files */

    virtual IShaderFileReader*     GetShaderFileReader( );
    virtual IShaderFeedbackWriter* GetShaderFeedbackWriter( );
    virtual void                   SetShaderFileReader( IShaderFileReader* pShaderFileReader );
    virtual void                   SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter );

    virtual apemode::unique_ptr< ICompiledShader > Compile( const std::string&                FilePath,
                                                            const IMacroDefinitionCollection* pMacros,
                                                            EShaderType                       eShaderKind,
                                                            EShaderOptimizationType           eShaderOptimization,
                                                            IIncludedFileSet*                 pOutIncludedFiles );

private:
    shaderc::Compiler                                    Compiler;
    apemode::shp::ShaderCompiler::IShaderFileReader*     pShaderFileReader     = nullptr;
    apemode::shp::ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter = nullptr;
};

class ShaderCompilerIncludedFileSet : public ShaderCompiler::IIncludedFileSet {
public:
    std::set< std::string > IncludedFiles;

    void InsertIncludedFile( const std::string& includedFileName ) override {
        IncludedFiles.insert( includedFileName );
    }
};

class ShaderCompilerMacroDefinitionCollection : public ShaderCompiler::IMacroDefinitionCollection {
public:
    std::vector< std::string > Macros;

    void Init( const std::map< std::string, std::string >& definitions ) {
        Macros.clear( );
        Macros.reserve( definitions.size( ) << 1 );

        for ( const auto& p : definitions ) {
            Macros.push_back( p.first );
            Macros.push_back( p.second );
        }
    }

    size_t GetCount( ) const override {
        return Macros.size( ) >> 1;
    }

    ShaderCompiler::MacroDefinition GetMacroDefinition( const size_t macroIndex ) const override {
        assert( Macros.size( ) > ( ( macroIndex << 1 ) + 1 ) );

        ShaderCompiler::MacroDefinition macroDefinition;
        macroDefinition.pszKey   = Macros[ ( macroIndex << 1 ) ].c_str( );
        macroDefinition.pszValue = Macros[ ( macroIndex << 1 ) + 1 ].c_str( );
        return macroDefinition;
    }
};

class ShaderFileReader : public ShaderCompiler::IShaderFileReader {
public:
    apemode::platform::IAssetManager* mAssetManager;

    inline bool ReadShaderTxtFile( const std::string& FilePath,
                                   std::string&       OutFileFullPath,
                                   std::string&       OutFileContent ) override;
};

class ShaderFeedbackWriter : public ShaderCompiler::IShaderFeedbackWriter {
public:
    inline void WriteFeedback( EFeedbackType                                     eType,
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
        inline friend OStream& operator<<( OStream& os, const EFeedbackTypeWithOStream& feedbackType ) {
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
        OutFileContent       = reinterpret_cast< const char* >( assetText.data( ) );
        OutFileFullPath      = pAsset->GetId( );
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
        apemode::LogError( "ShaderCompiler: {} / {}", // / {}",
                           EFeedbackTypeWithOStream( feedbackStage ),
                           EFeedbackTypeWithOStream( feedbackCompilationError ),
                           FullFilePath );
        apemode::LogError( "           Msg: {}", (const char*) pContent );
        // apemode::platform::DebugBreak( );
    } else {
        apemode::LogInfo( "ShaderCompiler: {} / {}", // / {}",
                          EFeedbackTypeWithOStream( feedbackStage ),
                          EFeedbackTypeWithOStream( feedbackCompilationError ),
                          FullFilePath );
        // TODO: Store compiled shader to file system
    }
}

} // namespace shp
} // namespace apemode
