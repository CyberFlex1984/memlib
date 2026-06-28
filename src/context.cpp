#include <memlib/context.hpp>

#include "backend/internal_backend.hpp"


namespace memlib {
    Result<Context> Context::internal() {
        auto impl = std::make_unique<InternalBackend>();
        return Context(std::move(impl));
    }
    
}