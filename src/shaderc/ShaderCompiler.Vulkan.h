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

    virtual const uint8_t*   GetBytePtr( ) const         = 0;
    virtual std::string_view GetPreprocessedSrc( ) const = 0;
    virtual std::string_view GetAssemblySrc( ) const     = 0;
    virtual size_t           GetByteCount( ) const       = 0;

    inline const uint32_t* GetDwordPtr( ) const {
        return reinterpret_cast< const uint32_t* >( GetBytePtr( ) );
    }

    inline size_t GetDwordCount( ) const {
        assert( GetByteCount( ) % 4 == 0 );
        return GetByteCount( ) >> 2;
    }
};

/* Wrapper for shaderc lib */
class IShaderCompiler {
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
        virtual bool ReadShaderTxtFile( const std::string& filePath,
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
                                    const std::string&                fullFilePath,
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

    virtual ~IShaderCompiler( ) = default;

    /* @note Compiling from source string */

    virtual apemode::unique_ptr< ICompiledShader > Compile( const std::string&                shaderName,
                                                            const std::string&                sourceCode,
                                                            const IMacroDefinitionCollection* pMacros,
                                                            EShaderType                       eShaderKind,
                                                            EShaderOptimizationType           eShaderOptimization ) const = 0;

    /* @note Compiling from source files */

    virtual IShaderFileReader*     GetShaderFileReader( )                                                  = 0;
    virtual IShaderFeedbackWriter* GetShaderFeedbackWriter( )                                              = 0;
    virtual void                   SetShaderFileReader( IShaderFileReader* pShaderFileReader )             = 0;
    virtual void                   SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter ) = 0;

    virtual apemode::unique_ptr< ICompiledShader > Compile( const std::string&                filePath,
                                                            const IMacroDefinitionCollection* pMacros,
                                                            EShaderType                       eShaderKind,
                                                            EShaderOptimizationType           eShaderOptimization,
                                                            IIncludedFileSet*                 pOutIncludedFiles ) const = 0;
};

apemode::unique_ptr< IShaderCompiler > NewShaderCompiler( );

} // namespace shp
} // namespace apemode
