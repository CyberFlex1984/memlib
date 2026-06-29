#include <algorithm>
#include <cstdint>
#include <cstring>
#include <expected>
#include <string>

#include <fstream>
#include <filesystem>
#include <system_error>
#include <vector>

#include "scan_utils.hpp"
#include "memlib/types.hpp"

#if defined(__linux__)


#include "external_backend_linux.hpp"


#include <cctype>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>


namespace fs = std::filesystem;

namespace memlib {
    ExternalLinuxBackend::ExternalLinuxBackend(memlib::u32 pid, const std::string& name) : pid(pid), name(name), attached(true) {}

    static Result<memlib::u32> find_pid_by_name(const std::string& name) {
        for(const auto& entry : fs::directory_iterator("/proc")) {
            if(!entry.is_directory()) continue;

            std::string pid_str = entry.path().filename().string();
            if(!std::all_of(pid_str.begin(), pid_str.end(), ::isdigit)) continue;

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

    Result<std::unique_ptr<ExternalLinuxBackend>> ExternalLinuxBackend::create(memlib::u32 pid) {
        return std::make_unique<ExternalLinuxBackend>(pid, "");
    }
    Result<std::unique_ptr<ExternalLinuxBackend>> ExternalLinuxBackend::create(const std::string& name) {
        auto pid = TRY(find_pid_by_name(name));
        return std::make_unique<ExternalLinuxBackend>(pid, name);
    }
    Result<void> ExternalLinuxBackend::read_bytes(address_t addr, buffer buffer, size_t size){
        if(!attached){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        struct iovec local = {
            .iov_base = buffer, .iov_len = size
        };
        struct iovec remote = {
            .iov_base = reinterpret_cast<void*>(addr), .iov_len = size
        };

        ssize_t bytes = process_vm_readv(pid, &local, 1, &remote, 1, 0);

        if(bytes == -1){
            return std::unexpected(Error(ErrorCode::ReadFailed, "process_vm_readv: " + std::string(strerror(errno))));
        }

        if(bytes != static_cast<ssize_t>(size)){
            return std::unexpected(Error(ErrorCode::ReadFailed, "Partial read: " + std::to_string(bytes) + " of " + std::to_string(size)));
        }

        return {};
    }
    Result<void> ExternalLinuxBackend::write_bytes(address_t addr, buffer buffer, size_t size) {
        if(!attached){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        struct iovec local = {
            .iov_base = buffer, .iov_len = size
        };
        struct iovec remote = {
            .iov_base = reinterpret_cast<void*>(addr), .iov_len = size
        };

        ssize_t bytes = process_vm_writev(pid, &local, 1, &remote, 1, 0);

        if(bytes == -1){
            return std::unexpected(Error(ErrorCode::WriteFailed, 
            "process_vm_writev: " + std::string(strerror(errno))));
        }
        if (bytes != static_cast<ssize_t>(size)) {
            return std::unexpected(Error(ErrorCode::WriteFailed, 
            "Partial write: " + std::to_string(bytes) + " of " + std::to_string(size)));
        }
        return {};
    }
    Result<std::vector<address_t>> ExternalLinuxBackend::scan(
        const Pattern& pattern,
        address_t start,
        address_t end)
    {
        return scan_region(*this, pattern, start, end);
    }
    bool ExternalLinuxBackend::is_attached() const {
        if(!attached) return false;

        std::string path = "/proc/" + std::to_string(pid);
        return (attached = fs::exists(path));
    }
    Result<memlib::u32> ExternalLinuxBackend::get_pid() const {
        if(!attached){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        return pid;
    }
    std::string ExternalLinuxBackend::get_name() const {
        return name.empty() ? "ExternalLinux" : ("ExternalLinux(" + name + ")");
    }
}

#endif