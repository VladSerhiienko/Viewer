#pragma once

#ifdef _WIN32
#ifdef APEMODE_VIEWER_DLLEXPORT
#define APEMODE_VIEWER_API __declspec(dllexport)
#else
#define APEMODE_VIEWER_API __declspec(dllimport)
#endif
#else
#define APEMODE_VIEWER_API
#endif

//#include <apemode/platform/memory/MemoryManager.h>
#include "apemode/platform/IAppShell.h"
#include <memory>

namespace apemode {
namespace viewer {
namespace vk {

std::unique_ptr< apemode::platform::IAppShell > APEMODE_VIEWER_API CreateViewer( int args, const char** ppArgs );

} // namespace vk
} // namespace viewer
} // namespace apemode
