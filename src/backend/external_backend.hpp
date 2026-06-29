#pragma once

#if defined (_WIN32)

#include "external_backend_windows.hpp"

namespace memlib {
    
}

#elif defined(__linux__)

#include "external_backend_linux.hpp"

namespace memlib {
    using ExternalBackend = ExternalLinuxBackend;
}

#endif