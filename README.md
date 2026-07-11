# memlib — Universal Memory Manipulation Framework

memlib is a cross-platform C++23 library for reading, writing, and scanning process memory across multiple backends. Designed for flexibility, performance, and stealth — from user-mode to firmware-level access.

## Features

| Backend | Platform | Description |
|---------|----------|-------------|
| Internal | Windows / macOS / GNU/Linux | Direct memory access from within the current process |
| External | Windows / macOS / GNU/Linux | Cross-process memory access via OS API |
| eBPF | GNU/Linux | Kernel-level memory access without kernel modules (Linux 5.8+) |
| UEFI (experimental) | X64 / AARCH64 | Runtime driver with memory access via firmware |

Additional features:

* Pattern Scanner — byte pattern search with wildcards (??)
* Module Enumeration — get loaded modules (Windows/macOS/GNU/Linux)
* Address Arithmetic — pointer dereferencing, offsets, array access
* Zero-cost Abstractions — modern C++ with std::expected
* Cross-platform — Windows, macOS, GNU/Linux, and experimental UEFI
* No exceptions — all errors via Result<T>
* Header-only Optional — easy integration

## Quick Start

### Build

```
mkdir build && cd build
cmake ..
cmake --build .
```

### Usage

```cpp
#include <memlib/memlib.hpp>

int main() {
    auto ctx = std::move(memlib::Context::external("game.exe")).value();
    int health = std::move(ctx.at(0x12345678).read<int>()).value();
    ctx.at(0x12345678).write<int>(999);
    return 0;
}
```

## Architecture

### Backend System

```
IBackend (interface)
    ├── InternalBackend   — direct memory access
    ├── ExternalBackend   — OS API
    ├── EBPFBackend       — eBPF (Linux)
    └── UEFIBackend       — UEFI Runtime Services (TODO)
```

### Core API

```cpp
auto ctx = std::move(Context::external(pid)).value();
auto mem = ctx.at(address);
mem.read<T>();
mem.write<T>();
mem.add(offset);
mem.deref<T>();
```

## Platform Support

| Feature | Windows | macOS | GNU/Linux | UEFI (experimental) |
|---------|---------|-------|-----------|---------------------|
| Internal Backend | ✅ | ✅ | ✅ | — |
| External Backend | ✅ | ✅ | ✅ | — |
| eBPF Backend | — | — | ✅ | — |
| UEFI Driver | — | — | — | X64 / AARCH64 |
| Pattern Scanner | ✅ | ✅ | ✅ | — |
| Module Enumeration | ✅ | ✅ | ✅ | — |

Both `arm64`/`aarch64` and `x86_64` native builds are supported on macOS and
GNU/Linux. The Linux eBPF build maps the target processor to libbpf's target
architecture macros instead of assuming x86-64.

## Build Options

| Option | Default | Description |
|---------|---------|-------------|
| BUILD_EBPF_BACKEND | OFF | Build the GNU/Linux-only eBPF backend (requires libbpf and clang) |
| BUILD_UEFI_DRIVER | OFF | Build the experimental UEFI driver (may download EDK2) |
| BUILD_EXAMPLES | ON | Build example programs |

```bash
cmake -DBUILD_EBPF_BACKEND=ON ..
cmake -DBUILD_UEFI_DRIVER=ON ..
cmake -DBUILD_EXAMPLES=OFF ..
```

## Warning

Cross-process access is subject to each operating system's normal permissions.
On macOS, access to other processes may require an appropriately signed binary
and the task-for-pid entitlement. UEFI support is experimental; AARCH64 builds
on macOS require a complete LLVM installation containing clang, lld-link,
llvm-lib, and llvm-rc.

## License

GNU GPL 3 License — see LICENSE for details.

## Credits

Built by Monster

GitHub: Cyberflex1984

Tg: t.me/M0NST3R_65537

Tg: t.me/snowflake_65537
