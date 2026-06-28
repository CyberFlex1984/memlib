#include <memory>

#include <memlib/context.hpp>
#include <memlib/memory.hpp>

#include "backend/internal_backend.hpp"
#include "memlib/types.hpp"


namespace memlib {
    Result<Context> Context::internal() {
        auto impl = std::make_unique<InternalBackend>();
        return Context(std::move(impl));
    }
    inline Memory Context::at(address_t addr){
        return Memory(*this, addr);
    }
}
