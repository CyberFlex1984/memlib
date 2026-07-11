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
        auto _res = memlib::ExternalBackend::create(pid);
        if (!_res) {
            return std::unexpected(_res.error());
        }
        auto impl = std::move(_res).value();
        return Context(std::move(impl));
    }
    Result<Context> Context::external(const std::string& name){
        auto _res = memlib::ExternalBackend::create(name);
        if (!_res) {
            return std::unexpected(_res.error());
        }
        auto impl = std::move(_res).value();
        return Context(std::move(impl));
    }
    Result<Context> Context::ebpf(memlib::u32 pid){
        #if defined(MEMLIB_EBPF)
        auto impl = memlib::EBPFBackend::create(pid);
        if (!impl) {
            return std::unexpected(impl.error());
        }
        return Context(std::move(impl).value());
        #else
        (void)pid;
        return std::unexpected(Error(ErrorCode::NotSupported,
            "The eBPF backend is not enabled for this build"));
        #endif
    }
    Result<Context> Context::ebpf(const std::string& name){
        #if defined(MEMLIB_EBPF)
        auto impl = memlib::EBPFBackend::create(name);
        if (!impl) {
            return std::unexpected(impl.error());
        }
        return Context(std::move(impl).value());
        #else
        (void)name;
        return std::unexpected(Error(ErrorCode::NotSupported,
            "The eBPF backend is not enabled for this build"));
        #endif
    }
}
