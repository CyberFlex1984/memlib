#pragma once


#if defined (__linux__)

#include <memlib/types.hpp>
#include <memlib/ibackend.hpp>
#include <string>
#include <memory>

namespace memlib {
    class ExternalLinuxBackend : public IBackend {
        memlib::u32 pid = 0;
        bool attached = false;
        std::string name;

        ExternalLinuxBackend() = default;
    public:
        ExternalLinuxBackend(memlib::u32 pid, const std::string& name);

        static Result<std::unique_ptr<ExternalLinuxBackend>> create(memlib::u32 pid);
        static Result<std::unique_ptr<ExternalLinuxBackend>> create(const std::string& name);

        Result<void> read_bytes(address_t addr, buffer buffer, size_t size) override;
        Result<void> write_bytes(address_t addr, buffer buffer, size_t size) override;
        Result<std::vector<address_t>> scan(
            const Pattern& pattern,
            address_t start,
            address_t end
        ) override;

        bool is_attached() const override;
        Result<uint32_t> get_pid() const override;
        std::string get_name() const override;
    };
}

#endif