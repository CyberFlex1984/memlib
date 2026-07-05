#if defined(_WIN32)

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <strings.h>

#include "memlib/types.hpp"

#include "external_backend_windows.hpp"

#include "scan_utils.hpp"

namespace memlib {
    static Result<memlib::u32> find_pid_by_name(const std::string& name) {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if(snapshot == INVALID_HANDLE_VALUE) {
            return std::unexpected(Error(ErrorCode::BackendError, "Failed to create process snapshot"));
        }
    
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(entry);

        if(Process32First(snapshot, &entry)){
            do {
                if(strcasecmp(name.c_str(), entry.szExeFile) == 0){
                    CloseHandle(snapshot);
                    return entry.th32ProcessID;
                }
            } while(Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return std::unexpected(Error(ErrorCode::ProcessNotFound, "Process not found: " + name));
    }
    static HANDLE open_process(memlib::u32 pid) {
        return OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, 
            FALSE, pid);
    }

    ExternalWindowsBackend::ExternalWindowsBackend(
        HANDLE hProcess,
        memlib::u32 pid,
        const std::string& name
    ) : hProcess(hProcess), pid(pid), attached(true), name(name) {}

    ExternalWindowsBackend::~ExternalWindowsBackend(){
        if(hProcess){
            CloseHandle(hProcess);
            hProcess = nullptr;
        }
        attached = false;
    }

    Result<std::unique_ptr<ExternalWindowsBackend>> ExternalWindowsBackend::create(memlib::u32 pid) {
        auto hProcess = open_process(pid);
        if(!hProcess){
            return std::unexpected(Error(ErrorCode::ProcessNotFound,
            "Failed to open process with PID " + std::to_string(pid)));
        }
        return std::make_unique<ExternalWindowsBackend>(hProcess, pid, "");
    }

    Result<std::unique_ptr<ExternalWindowsBackend>> ExternalWindowsBackend::create(const std::string& name) {
        auto pid = TRY(find_pid_by_name(name));

        auto hProcess = open_process(pid);
        if(!hProcess){
            return std::unexpected(Error(ErrorCode::ProcessNotFound,
            "Failed to open process with PID " + std::to_string(pid)));
        }

        return std::make_unique<ExternalWindowsBackend>(hProcess, pid, name);
    }

    bool ExternalWindowsBackend::process_exists() const {
        if(!attached || !hProcess) return false;

        DWORD exitCode;
        if(!GetExitCodeProcess(hProcess, &exitCode)) return false;

        return exitCode == STILL_ACTIVE;
    }

    bool ExternalWindowsBackend::is_attached() const {
        return (attached = (attached && process_exists()));
    }

    Result<memlib::u32> ExternalWindowsBackend::get_pid() const {
        if(!is_attached()){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        return pid;
    }

    std::string ExternalWindowsBackend::get_name() const {
        return name.empty() ? "ExternalWindows" : ("ExternalWindows(" + name + ")");
    }

    Result<void> ExternalWindowsBackend::read_bytes(address_t addr, buffer buffer, size_t size) {
        if(!is_attached()){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        SIZE_T bytesRead;
        if(!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(addr), buffer, size, &bytesRead)) {
            return std::unexpected(Error(ErrorCode::ReadFailed,
                "ReadProcessMemory failed: " + std::to_string(GetLastError())));
        }
        if (bytesRead != size) {
            return std::unexpected(Error(ErrorCode::ReadFailed,
                "Partial read: " + std::to_string(bytesRead) + " of " + std::to_string(size)));
        }
        return {};
    }

    Result<void> ExternalWindowsBackend::write_bytes(address_t addr, buffer buffer, size_t size) {
        if(!is_attached()){
            return std::unexpected(Error(ErrorCode::NotAttached));
        }
        SIZE_T bytesWritten;
        if(!WriteProcessMemory(hProcess, (void*)addr, buffer, size, &bytesWritten)) {
            return std::unexpected(Error(ErrorCode::WriteFailed,
                "WriteProcessMemory failed: " + std::to_string(GetLastError())));
        }
        if(bytesWritten != size){
            return std::unexpected(Error(ErrorCode::WriteFailed,
                "Partial write: " + std::to_string(bytesWritten) + " of " + std::to_string(size)));
        }
        return {};
    }

    Result<std::vector<address_t>> ExternalWindowsBackend::scan(
        const Pattern& pattern,
        address_t start,
        address_t end
    ) {
        if(!is_attached()) {
            return std::unexpected(Error(ErrorCode::NotAttached));
        }

        return scan_region(*this, pattern, start, end);
    }

    Result<std::vector<ModuleInfo>> ExternalWindowsBackend::get_modules() {
        std::vector<ModuleInfo> modules;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return std::unexpected(Error(ErrorCode::BackendError,
                "CreateToolhelp32Snapshot failed"));
        }

        MODULEENTRY32 entry;
        entry.dwSize = sizeof(entry);

        if (Module32First(snapshot, &entry)) {
            do {
                std::string mod_name(entry.szModule);
                if (!mod_name.empty()) {
                    ModuleInfo info{mod_name, (address_t)entry.modBaseAddr, entry.modBaseSize};
                    modules.push_back(info);
                }
            } while (Module32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return modules;
    }
    Result<ModuleInfo> ExternalWindowsBackend::get_module(const std::string& name) {
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
}

#endif