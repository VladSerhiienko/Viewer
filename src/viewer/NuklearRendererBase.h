#pragma once

#include <apemode/platform/AppInput.h>
#include <apemode/platform/IAssetManager.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

namespace apemode {
class NuklearRendererBase {
public:
    enum Theme { Black, White, Red, Blue, Dark };
    enum Impl { kImpl_Unknown, kImpl_GL, kImpl_Vk };

    struct Vertex {
        float   pos[ 2 ];
        float   uv[ 2 ];
        nk_byte col[ 4 ];
    };

    struct InitParametersBase {
        typedef void ( *NkClipbardPasteFn )( nk_handle, struct nk_text_edit * );
        typedef void ( *NkClipbardCopyFn )( nk_handle, const char *, int );

        NkClipbardPasteFn                pClipboardPasteCallback = nullptr; /* Optional */
        NkClipbardCopyFn                 pClipboardCopyCallback  = nullptr; /* Optional */
        const apemode::platform::IAsset *pFontAsset              = nullptr; /* Required */
    };

    struct RenderParametersBase {
        float            Dims[ 2 ]            = {};                  /* Required */
        float            Scale[ 2 ]           = {};                  /* Required */
        nk_anti_aliasing eAntiAliasing        = NK_ANTI_ALIASING_ON; /* Ok */
        uint32_t         MaxVertexBufferSize  = 65536;               /* Ok */
        uint32_t         MaxElementBufferSize = 65536;               /* Ok */
    };

public:
    Impl                 Impl;
    nk_draw_null_texture NullTexture;
    nk_font *            pDefaultFont;
    nk_font_atlas        Atlas;
    nk_buffer            RenderCmds;
    nk_context           Context;

public:
    void FontStashBegin( nk_font_atlas **ppAtlas );
    bool FontStashEnd( InitParametersBase *pInitParams );
    int  HandleInput( const apemode::platform::AppInput *pAppInput );
    void Shutdown( );
    void SetStyle( Theme eTheme );

public:
    virtual ~NuklearRendererBase( ) = default;

    virtual bool  Init( InitParametersBase *pInitParams );
    virtual bool  Render( RenderParametersBase *pRenderParams );
    virtual void  DeviceDestroy( );
    virtual bool  DeviceCreate( InitParametersBase *pInitParams );
    virtual void *DeviceUploadAtlas( InitParametersBase *pInitParams, const void *pImgData, uint32_t width, uint32_t height );
};
} // namespace apemode
