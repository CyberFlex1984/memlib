#if defined(__APPLE__)

#include "external_backend_macos.hpp"

#include "scan_utils.hpp"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <libproc.h>
#include <limits>
#include <map>
#include <mach/mach_error.h>
#include <mach/mach_vm.h>
#include <string_view>
#include <utility>
#include <vector>
#include <unistd.h>

namespace memlib {
    namespace {
        Error mach_error(ErrorCode code, std::string_view operation, kern_return_t result) {
            return Error(code,
                std::string(operation) + ": " + mach_error_string(result));
        }

        Result<memlib::u32> find_pid_by_name_macos(const std::string& name) {
            const int buffer_size = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
            if (buffer_size <= 0) {
                return std::unexpected(Error(ErrorCode::BackendError,
                    "proc_listpids failed: " + std::string(std::strerror(errno))));
            }

            std::vector<pid_t> pids(static_cast<size_t>(buffer_size) / sizeof(pid_t));
            const int bytes = proc_listpids(
                PROC_ALL_PIDS,
                0,
                pids.data(),
                static_cast<int>(pids.size() * sizeof(pid_t))
            );
            if (bytes <= 0) {
                return std::unexpected(Error(ErrorCode::BackendError,
                    "proc_listpids failed: " + std::string(std::strerror(errno))));
            }

            pids.resize(static_cast<size_t>(bytes) / sizeof(pid_t));
            for (const pid_t candidate : pids) {
                if (candidate <= 0) {
                    continue;
                }

                char process_name[PROC_PIDPATHINFO_MAXSIZE]{};
                if (proc_name(candidate, process_name, sizeof(process_name)) > 0 &&
                    name == process_name) {
                    return static_cast<memlib::u32>(candidate);
                }
            }

            return std::unexpected(Error(ErrorCode::ProcessNotFound,
                "Process not found: " + name));
        }
    }

    ExternalMacOSBackend::ExternalMacOSBackend(
        memlib::u32 pid,
        mach_port_t task,
        std::string name
    ) : pid(pid), task(task), attached(true), name(std::move(name)) {}

    ExternalMacOSBackend::~ExternalMacOSBackend() {
        if (task != MACH_PORT_NULL) {
            mach_port_deallocate(mach_task_self(), task);
        }
    }

    Result<std::unique_ptr<ExternalMacOSBackend>> ExternalMacOSBackend::create(memlib::u32 pid) {
        mach_port_t task = MACH_PORT_NULL;
        kern_return_t result = KERN_SUCCESS;
        if (pid == static_cast<memlib::u32>(getpid())) {
            task = mach_task_self();
            result = mach_port_mod_refs(
                mach_task_self(),
                task,
                MACH_PORT_RIGHT_SEND,
                1
            );
        } else {
            result = task_for_pid(
                mach_task_self(),
                static_cast<pid_t>(pid),
                &task
            );
        }
        if (result != KERN_SUCCESS) {
            const ErrorCode code = result == KERN_PROTECTION_FAILURE
                ? ErrorCode::AccessDenied
                : ErrorCode::BackendError;
            return std::unexpected(mach_error(code, "task_for_pid", result));
        }

        return std::make_unique<ExternalMacOSBackend>(pid, task, "");
    }

    Result<std::unique_ptr<ExternalMacOSBackend>> ExternalMacOSBackend::create(
        const std::string& name
    ) {
        auto pid_result = find_pid_by_name_macos(name);
        if (!pid_result) {
            return std::unexpected(pid_result.error());
        }

        auto backend = create(pid_result.value());
        if (backend) {
            backend.value()->name = name;
        }
        return backend;
    }

    Result<void> ExternalMacOSBackend::read_bytes(
        address_t addr,
        buffer destination,
        size_t size
    ) {
        if (!is_attached()) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        mach_vm_size_t bytes_read = 0;
        const kern_return_t result = mach_vm_read_overwrite(
            task,
            static_cast<mach_vm_address_t>(addr),
            static_cast<mach_vm_size_t>(size),
            reinterpret_cast<mach_vm_address_t>(destination),
            &bytes_read
        );
        if (result != KERN_SUCCESS) {
            return std::unexpected(mach_error(ErrorCode::ReadFailed,
                "mach_vm_read_overwrite", result));
        }
        if (bytes_read != size) {
            return std::unexpected(Error(ErrorCode::ReadFailed,
                "Partial read: " + std::to_string(bytes_read) + " of " + std::to_string(size)));
        }

        return {};
    }

    Result<void> ExternalMacOSBackend::write_bytes(
        address_t addr,
        buffer source,
        size_t size
    ) {
        if (!is_attached()) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        auto* bytes = static_cast<uint8_t*>(source);
        size_t offset = 0;
        while (offset < size) {
            const auto chunk = static_cast<mach_msg_type_number_t>(std::min<size_t>(
                size - offset,
                std::numeric_limits<mach_msg_type_number_t>::max()
            ));
            const kern_return_t result = mach_vm_write(
                task,
                static_cast<mach_vm_address_t>(addr + offset),
                reinterpret_cast<vm_offset_t>(bytes + offset),
                chunk
            );
            if (result != KERN_SUCCESS) {
                return std::unexpected(mach_error(ErrorCode::WriteFailed,
                    "mach_vm_write", result));
            }
            offset += chunk;
        }

        return {};
    }

    Result<std::vector<address_t>> ExternalMacOSBackend::scan(
        const Pattern& pattern,
        address_t start,
        address_t end
    ) {
        return scan_region(*this, pattern, start, end);
    }

    bool ExternalMacOSBackend::is_attached() const {
        if (!attached) {
            return false;
        }
        if (kill(static_cast<pid_t>(pid), 0) == 0 || errno == EPERM) {
            return true;
        }
        attached = false;
        return false;
    }

    Result<memlib::u32> ExternalMacOSBackend::get_pid() const {
        if (!is_attached()) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        return pid;
    }

    std::string ExternalMacOSBackend::get_name() const {
        return name.empty() ? "ExternalMacOS" : "ExternalMacOS(" + name + ")";
    }

    Result<std::vector<ModuleInfo>> ExternalMacOSBackend::get_modules() {
        if (!is_attached()) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        std::map<std::string, ModuleInfo> module_map;
        mach_vm_address_t address = 0;
        while (true) {
            mach_vm_size_t region_size = 0;
            vm_region_basic_info_data_64_t info{};
            mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
            mach_port_t object_name = MACH_PORT_NULL;
            const kern_return_t result = mach_vm_region(
                task,
                &address,
                &region_size,
                VM_REGION_BASIC_INFO_64,
                reinterpret_cast<vm_region_info_t>(&info),
                &info_count,
                &object_name
            );
            if (object_name != MACH_PORT_NULL) {
                mach_port_deallocate(mach_task_self(), object_name);
            }
            if (result == KERN_INVALID_ADDRESS) {
                break;
            }
            if (result != KERN_SUCCESS) {
                return std::unexpected(mach_error(ErrorCode::BackendError,
                    "mach_vm_region", result));
            }

            char path[PROC_PIDPATHINFO_MAXSIZE]{};
            if (proc_regionfilename(
                    static_cast<pid_t>(pid),
                    address,
                    path,
                    sizeof(path)
                ) > 0) {
                std::string full_path(path);
                const auto slash = full_path.find_last_of('/');
                const std::string module_name = slash == std::string::npos
                    ? full_path
                    : full_path.substr(slash + 1);
                const address_t start = static_cast<address_t>(address);
                const address_t end = start + static_cast<address_t>(region_size);

                auto [entry, inserted] = module_map.try_emplace(
                    full_path,
                    module_name,
                    start,
                    static_cast<size_t>(region_size)
                );
                if (!inserted) {
                    const address_t old_end = entry->second.base + entry->second.size;
                    entry->second.base = std::min(entry->second.base, start);
                    entry->second.size = std::max(old_end, end) - entry->second.base;
                }
            }

            if (region_size == 0 || address > std::numeric_limits<mach_vm_address_t>::max() - region_size) {
                break;
            }
            address += region_size;
        }

        std::vector<ModuleInfo> modules;
        modules.reserve(module_map.size());
        for (auto& [path, module] : module_map) {
            modules.push_back(std::move(module));
        }
        return modules;
    }

    Result<ModuleInfo> ExternalMacOSBackend::get_module(const std::string& name) {
        auto modules = get_modules();
        if (!modules) {
            return std::unexpected(modules.error());
        }
        for (const auto& module : modules.value()) {
            if (module.name == name) {
                return module;
            }
        }
        return std::unexpected(Error(ErrorCode::ProcessNotFound,
            "Module not found: " + name));
    }
}

#endif
