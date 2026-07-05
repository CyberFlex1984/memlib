#include <memlib/context.hpp>

#include "backend/internal_backend.hpp"
#include "backend/external_backend.hpp"
#include "backend/ebpf_backend.hpp"
#include "memlib/types.hpp"


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
    Result<Context> Context::ebpf(memlib::u32 pid){
        #if defined (__linux__)
        auto impl = TRY(memlib::EBPFBackend::create(pid));
        return Context(std::move(impl));
        #else
        return std::unexpected(Error(ErrorCode::NotSupported, "This feature supported only in LINUX"));
        #endif
    }
    Result<Context> Context::ebpf(const std::string& name){
        #if defined (__linux__)
        auto impl = TRY(memlib::EBPFBackend::create(name));
        return Context(std::move(impl));
        #else
        return std::unexpected(Error(ErrorCode::NotSupported, "This feature supported only in LINUX"));
        #endif
    }
}