#if defined(__linux__)

#include <map>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <expected>
#include <string>
#include <sstream>

#include <fstream>
#include <filesystem>
#include <system_error>
#include <vector>

#include "scan_utils.hpp"
#include "memlib/types.hpp"

#include "external_backend_linux.hpp"


#include <cctype>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>


namespace fs = std::filesystem;

namespace memlib {
    ExternalLinuxBackend::ExternalLinuxBackend(memlib::u32 pid, const std::string& name) : pid(pid), name(name), attached(true) {}


    Result<std::unique_ptr<ExternalLinuxBackend>> ExternalLinuxBackend::create(memlib::u32 pid) {
        return std::make_unique<ExternalLinuxBackend>(pid, "");
    }
    Result<std::unique_ptr<ExternalLinuxBackend>> ExternalLinuxBackend::create(const std::string& name) {
        auto pid = find_pid_by_name(name);
        if (!pid) {
            return std::unexpected(pid.error());
        }
        return std::make_unique<ExternalLinuxBackend>(pid.value(), name);
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

    Result<std::vector<ModuleInfo>> ExternalLinuxBackend::get_modules() {
        std::vector<ModuleInfo> modules;

        std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
        std::ifstream maps(maps_path);
        if (!maps.is_open()) {
            return std::unexpected(Error(ErrorCode::BackendError,
                "Failed to open /proc/pid/maps"));
        }

        std::string line;
        std::map<std::string, ModuleInfo> module_map;

        while (std::getline(maps, line)) {
            std::istringstream iss(line);
            std::string addr_range, perms, offset, dev, inode, path;

            iss >> addr_range >> perms >> offset >> dev >> inode;
            std::getline(iss, path);
        
            if (!path.empty() && path[0] == ' ') {
                path.erase(0, 1);
            }

            if (perms[2] != 'x' && perms[0] != 'r') continue;
            if (path.empty() || path[0] == '[') continue;
            if (path.find("/dev/") == 0) continue;

            size_t dash = addr_range.find('-');
            address_t start = std::stoull(addr_range.substr(0, dash), nullptr, 16);
            address_t end = std::stoull(addr_range.substr(dash + 1), nullptr, 16);

            std::string name = path;
            size_t slash = path.find_last_of('/');
            if (slash != std::string::npos) {
                name = path.substr(slash + 1);
            }

            auto it = module_map.find(name);
            if (it != module_map.end()) {
                if (it->second.base > start) {
                    it->second.base = start;
                }
                if (it->second.size < (end - start)) {
                    it->second.size = end - start;
                }
            } else {
                ModuleInfo info{name, start, end - start};
                module_map[name] = info;
            }
        }

        for (auto& [name, info] : module_map) {
            modules.push_back(info);
        }

        return modules;
    }
    Result<ModuleInfo> ExternalLinuxBackend::get_module(const std::string& name) {
        auto modules = get_modules();
        if (!modules) {
            return std::unexpected(modules.error());
        }

        for (const auto& mod : modules.value()) {
            if (mod.name == name) {
                return mod;
            }
        }

        return std::unexpected(Error(ErrorCode::ProcessNotFound,
            "Module not found: " + name));
    }
}

#endif
