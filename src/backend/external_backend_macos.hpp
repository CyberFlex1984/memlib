#pragma once

#if defined(__APPLE__)

#include <mach/mach.h>

#include <memory>
#include <string>

#include <memlib/ibackend.hpp>
#include <memlib/types.hpp>

namespace memlib {
    class ExternalMacOSBackend final : public IBackend {
        memlib::u32 pid = 0;
        mach_port_t task = MACH_PORT_NULL;
        mutable bool attached = false;
        std::string name;

    public:
        ExternalMacOSBackend(memlib::u32 pid, mach_port_t task, std::string name);
        ~ExternalMacOSBackend() override;

        ExternalMacOSBackend(const ExternalMacOSBackend&) = delete;
        ExternalMacOSBackend& operator=(const ExternalMacOSBackend&) = delete;

        static Result<std::unique_ptr<ExternalMacOSBackend>> create(memlib::u32 pid);
        static Result<std::unique_ptr<ExternalMacOSBackend>> create(const std::string& name);

        Result<void> read_bytes(address_t addr, buffer buffer, size_t size) override;
        Result<void> write_bytes(address_t addr, buffer buffer, size_t size) override;
        Result<std::vector<address_t>> scan(
            const Pattern& pattern,
            address_t start,
            address_t end
        ) override;

        bool is_attached() const override;
        Result<memlib::u32> get_pid() const override;
        std::string get_name() const override;

        Result<std::vector<ModuleInfo>> get_modules() override;
        Result<ModuleInfo> get_module(const std::string& name) override;
    };
}

#endif
