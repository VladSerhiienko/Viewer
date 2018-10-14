#pragma once

namespace apemode {
namespace platform {

struct AppSurface {
    int OverrideWidth;
    int OverrideHeight;
    union {
        struct {
            void* hWindow;   // HWND
            void* hInstance; // HINSTANCE
        } Windows;
        struct {
            void* pDisplayX11; // Display*
            void* pWindowX11;  // Window*
        } X11;
        struct {
            void* pViewIOS;
        } iOS;
        struct {
            void* pViewMacOS;
        } macOS;
        struct {
            void* pANativeWindow; // ANativeWindow*
        } Android;
    };
};

} // namespace platform
} // namespace apemode