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
* Cross-platform — Windows, macOS, GNU/Linux
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

Native `arm64`/`aarch64` and `x86_64` builds are supported.
AARCH64 UEFI builds are verified on macOS with EDK2, LLVM, and LLD.

## Build Options

| Option | Default | Description |
|---------|---------|-------------|
| BUILD_EBPF_BACKEND | OFF | Build the Linux eBPF backend |
| BUILD_UEFI_DRIVER | OFF | Build the experimental UEFI driver |
| BUILD_EXAMPLES | ON | Build example programs |

```bash
cmake -DBUILD_EBPF_BACKEND=ON ..
cmake -DBUILD_UEFI_DRIVER=ON ..
cmake -DBUILD_EXAMPLES=OFF ..
```

## Warning

macOS cross-process access may require the `task_for_pid` entitlement. UEFI
support is experimental.

## License

GNU GPL 3 License — see LICENSE for details.

## Credits

Built by Monster

GitHub: Cyberflex1984

Tg: t.me/M0NST3R_65537

Tg: t.me/snowflake_65537
