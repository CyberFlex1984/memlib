#pragma once

#if defined (_WIN32)

#include "external_backend_windows.hpp"

namespace memlib {
    using ExternalBackend = ExternalWindowsBackend;
}

#elif defined(__linux__)

#include "external_backend_linux.hpp"

namespace memlib {
    using ExternalBackend = ExternalLinuxBackend;
}

#elif defined(__APPLE__)

#include "external_backend_macos.hpp"

namespace memlib {
    using ExternalBackend = ExternalMacOSBackend;
}

#endif
