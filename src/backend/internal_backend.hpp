#pragma once

#include "memlib/ibackend.hpp"
#include "memlib/pattern.hpp"
#include <cstring>
#include <iterator>
#include <memlib/types.hpp>
#include <ranges>
#include <vector>

#if defined(_WIN32)
    #include <windows.h>
    #include <tlhelp32.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <fstream>
    #include <filesystem>
#endif


namespace memlib {
    class InternalBackend : public IBackend {
    public:
        ~InternalBackend() override {}

        Result<void> read_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override {
            std::memcpy(buffer, reinterpret_cast<memlib::buffer>(addr), size);
            return {};
        }
        Result<void> write_bytes(memlib::address_t addr, memlib::buffer buffer, memlib::size_t size) override {
            std::memcpy(reinterpret_cast<memlib::buffer>(addr), buffer, size);
            return {};
        }

        Result<std::vector<address_t>> scan(const Pattern& pattern, address_t start, address_t end) override {
            const byte_t* data = reinterpret_cast<const byte_t*>(start);
            size_t size = end - start;
            size_t pattern_size = pattern.size();

            if(size < pattern_size) {
                return std::vector<address_t>{};
            }

            auto positions = std::views::iota(size_t{0}, size - pattern_size + 1)
                | std::views::filter([&](size_t offset){
                    return pattern.matches(data, offset);
                })
                | std::views::transform([&](size_t offset){
                    return start + offset;
                });
            
            std::vector<address_t> results{};
            std::ranges::copy(positions, std::back_inserter(results));

            return results;
        }

        bool is_attached() const override {
            return true;
        }

        Result<memlib::u32> get_pid() const override {
            #if defined(_WIN32)
                return GetCurrentProcessId();
            #elif defined(__linux__)
                return static_cast<memlib::u32>(getpid());
            #endif
        }
        std::string get_name() const override {
            return "Internal";
        }

#if defined(_WIN32)

        Result<std::vector<ModuleInfo>> get_modules() override {
            std::vector<ModuleInfo> modules;

            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
            if (snapshot == INVALID_HANDLE_VALUE) {
                return std::unexpected(Error(ErrorCode::BackendError,
                    "CreateToolhelp32Snapshot failed: " + std::to_string(GetLastError())));
            }

            MODULEENTRY32 entry;
            entry.dwSize = sizeof(entry);

            if (Module32First(snapshot, &entry)) {
                do {
                    ModuleInfo info;
                    info.name = entry.szModule;
                    info.base = reinterpret_cast<address_t>(entry.modBaseAddr);
                    info.size = entry.modBaseSize;
                    modules.push_back(info);
                } while (Module32Next(snapshot, &entry));
            }

            CloseHandle(snapshot);
            return modules;
        }

        Result<ModuleInfo> get_module(const std::string& name) override {
            auto modules = get_modules();
            if (!modules) {
                return std::unexpected(modules.error());
            }

            std::string name_lower = name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

            for (const auto& mod : modules.value()) {
                std::string mod_name_lower = mod.name;
                std::transform(mod_name_lower.begin(), mod_name_lower.end(),
                            mod_name_lower.begin(), ::tolower);

                if (mod_name_lower == name_lower) {
                    return mod;
                }
            }

            return std::unexpected(Error(ErrorCode::ProcessNotFound,
                "Module not found: " + name));
        }
#elif defined(__linux__)
        Result<std::vector<ModuleInfo>> get_modules() override {
            std::vector<ModuleInfo> modules;

            std::ifstream maps("/proc/self/maps");
            if (!maps.is_open()) {
                return std::unexpected(Error(ErrorCode::BackendError,
                    "Failed to open /proc/self/maps"));
            }

            std::string line;
            while (std::getline(maps, line)) {
                std::istringstream iss(line);
                std::string addr_range, perms, offset, dev, inode, path;

                iss >> addr_range >> perms >> offset >> dev >> inode;
                std::getline(iss, path);

                if (!path.empty() && path[0] == ' ') {
                    path.erase(0, 1);
                }

                if (perms[0] != 'r' && perms[2] != 'x') continue;
                if (path.empty() || path[0] == '[') continue;
                if (path.find("/dev/") == 0) continue;
                if (path.find("[heap]") != std::string::npos) continue;
                if (path.find("[stack]") != std::string::npos) continue;
                if (path.find("[vdso]") != std::string::npos) continue;
                if (path.find("[vsyscall]") != std::string::npos) continue;

                size_t dash = addr_range.find('-');
                address_t start = std::stoull(addr_range.substr(0, dash), nullptr, 16);
                address_t end = std::stoull(addr_range.substr(dash + 1), nullptr, 16);

                std::string mod_name = path;
                size_t slash = path.find_last_of('/');
                if (slash != std::string::npos) {
                    mod_name = path.substr(slash + 1);
                }

                ModuleInfo info;
                info.name = mod_name;
                info.base = start;
                info.size = end - start;
                modules.push_back(info);
            }

            return modules;
        }

        Result<ModuleInfo> get_module(const std::string& name) override {
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
#endif

    };
}