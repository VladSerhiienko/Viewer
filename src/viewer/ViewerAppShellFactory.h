#pragma once

#include <apemode/platform/memory/MemoryManager.h>
#include <apemode/platform/IAppShell.h>

namespace apemode {
namespace viewer {
namespace vk {

apemode::unique_ptr< apemode::platform::IAppShell > CreateViewer( int args, const char** ppArgs );

} // namespace vk
} // namespace viewer
} // namespace apemode