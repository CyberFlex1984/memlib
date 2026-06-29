#include <memlib/context.hpp>

#include "backend/internal_backend.hpp"
#include "backend/external_backend.hpp"


namespace memlib {
    Result<Context> Context::internal() {
        auto impl = std::make_unique<InternalBackend>();
        return Context(std::move(impl));
    }
    Result<Context> Context::external(memlib::u32 pid){
        auto impl = TRY(memlib::ExternalBackend::create(pid));
        return Context(std::move(impl));
    }
    Result<Context> Context::external(const std::string& name){
        auto impl = TRY(memlib::ExternalBackend::create(name));
        return Context(std::move(impl));
    }
}