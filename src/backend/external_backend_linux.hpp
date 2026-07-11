#pragma once


#if defined (__linux__)

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>

#include <memlib/types.hpp>
#include <memlib/ibackend.hpp>
#include <string>
#include <memory>

namespace memlib {
    class ExternalLinuxBackend : public IBackend {
        memlib::u32 pid = 0;
        mutable bool attached = false;
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

        Result<std::vector<ModuleInfo>> get_modules() override;
        Result<ModuleInfo> get_module(const std::string& name) override;
    };

    static Result<memlib::u32> find_pid_by_name(const std::string& name) {
        namespace fs = std::filesystem;

        for(const auto& entry : fs::directory_iterator("/proc")) {
            if(!entry.is_directory()) continue;

            std::string pid_str = entry.path().filename().string();
            if(!std::all_of(pid_str.begin(), pid_str.end(), [](unsigned char character) {
                return std::isdigit(character) != 0;
            })) continue;

            std::ifstream comm_file(entry.path() / "comm");
            if(!comm_file) continue;

            std::string comm;
            std::getline(comm_file, comm);

            if(comm == name){
                return static_cast<memlib::u32>(std::stoul(pid_str));
            }
        }
        return std::unexpected(Error(ErrorCode::ProcessNotFound, "Process not found: " + name));
    }
}

#endif
