#pragma once

#if defined(_WIN32)

#include <windows.h>

#include <memlib/types.hpp>
#include <memlib/ibackend.hpp>
#include <string>
#include <memory>

namespace memlib {
    class ExternalWindowsBackend : public IBackend {
        HANDLE hProcess = nullptr;
        memlib::u32 pid = 0;
        mutable bool attached = false;
        std::string name;
    public:
        ExternalWindowsBackend(HANDLE hProcess, memlib::u32 pid, const std::string& name);

        static Result<std::unique_ptr<ExternalWindowsBackend>> create(memlib::u32 pid);
        static Result<std::unique_ptr<ExternalWindowsBackend>> create(const std::string& name);

        ~ExternalWindowsBackend() override;

        Result<void> read_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override;
        Result<void> write_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override;

        Result<std::vector<address_t>> scan(const Pattern& pattern, address_t start, address_t end) override;

        bool is_attached() const override;
        Result<memlib::u32> get_pid() const override;
        std::string get_name() const override;

        Result<std::vector<ModuleInfo>> get_modules() override;
        Result<ModuleInfo> get_module(const std::string& name) override;
    private:
        bool process_exists() const;
    };
}

#endif