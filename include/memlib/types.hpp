#pragma once

#include <cstdint>
#include <string>
#include <expected>
#include <utility>
#include <vector>

namespace memlib {
    using u8 = std::uint8_t;
    using i8 = std::int8_t;
    using u16 = std::uint16_t;
    using i16 = std::int16_t;
    using u32 = std::uint32_t;
    using i32 = std::int32_t;
    using u64 = std::uint64_t;
    using i64 = std::int64_t;

    using address_t = std::uintptr_t;
    using vtable = void**;
    using buffer = void*;
    using size_t = std::size_t;

    using byte_t = std::uint8_t;
    using byte_array = std::vector<byte_t>;

    struct ModuleInfo {
        std::string name;
        address_t base;
        size_t size;

        ModuleInfo() = default;

        ModuleInfo(const std::string& n, address_t b, size_t s)
            : name(n), base(b), size(s) {}

        ModuleInfo(const ModuleInfo& other)
            : name(other.name), base(other.base), size(other.size) {}

        ModuleInfo(ModuleInfo&& other) noexcept
            : name(std::move(other.name)), base(other.base), size(other.size) {}

        ModuleInfo& operator=(const ModuleInfo& other) {
            if (this != &other) {
                name = other.name;
                base = other.base;
                size = other.size;
            }
            return *this;
        }

        ModuleInfo& operator=(ModuleInfo&& other) noexcept {
            if (this != &other) {
                name = std::move(other.name);
                base = other.base;
                size = other.size;
            }
            return *this;
        }
    };

    enum class ErrorCode {
        InvalidAddress,
        AccessDenied,
        ProcessNotFound,
        ReadFailed,
        WriteFailed,
        NotInitialized,
        NotAttached,
        BackendError,
        NotSupported,
        Timeout,

        Custom
    };

    struct Error {
        ErrorCode code;
        std::string message;

        Error(ErrorCode c): code(c) {}
        Error(ErrorCode c, const std::string& msg): code(c), message(msg) {}
    };

    template<typename T>
    using Result = std::expected<T, Error>;

    inline std::string to_string(ErrorCode code) {
        switch(code){
            case ErrorCode::InvalidAddress: return "Invalid Address";
            case ErrorCode::ProcessNotFound: return "Process Not Found";
            case ErrorCode::AccessDenied: return "Access Denied";
            case ErrorCode::ReadFailed: return "Read Failed";
            case ErrorCode::WriteFailed: return "Write Failed";
            case ErrorCode::NotInitialized: return "Not Initialized";
            case ErrorCode::NotAttached: return "Not Attached";
            case ErrorCode::BackendError: return "Backend Error";
            case ErrorCode::NotSupported: return "Not Supported";
            case ErrorCode::Timeout: return "Timeout";

            default: return "Custom";
        }
    }

    inline std::string to_string(const Error& err) {
        return err.message.empty() ? to_string(err.code) : err.message;
    }
}
