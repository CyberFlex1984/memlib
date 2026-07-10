# memlib — Universal Memory Manipulation Framework

memlib is a cross-platform C++23 library for reading, writing, and scanning process memory across multiple backends. Designed for flexibility, performance, and stealth — from user-mode to firmware-level access.

## Features

| Backend | Platform | Description |
|---------|----------|-------------|
| Internal | Windows / Linux | Direct memory access from within the current process |
| External | Windows / Linux | Cross-process memory access via OS API |
| eBPF | Linux | Kernel-level memory access without kernel modules (Linux 5.8+) |
| UEFI (TODO) | Any UEFI | Runtime driver with memory access via firmware |

Additional features:

* Pattern Scanner — byte pattern search with wildcards (??)
* Module Enumeration — get loaded modules (Windows/Linux)
* Address Arithmetic — pointer dereferencing, offsets, array access
* Zero-cost Abstractions — modern C++ with std::expected
* Cross-platform — Windows, Linux, UEFI (TODO)
* No exceptions — all errors via Result<T>
* Header-only Optional — easy integration

## Quick Start

### Build

```
mkdir build && cd build
cmake ..
make
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

| Feature | Windows | Linux | UEFI (TODO) |
|---------|---------|-------|------|
| Internal Backend | ✅ | ✅ | — |
| External Backend | ✅ | ✅ | — |
| eBPF Backend | — | ✅ | — |
| UEFI Backend (TODO) | — | — | ✅ |
| Pattern Scanner | ✅ | ✅ | — |
| Module Enumeration | ✅ | ✅ | — |

## Build Options

| Option | Default | Description |
|---------|---------|-------------|
| BUILD_EBPF_BACKEND | ON | Build eBPF backend |
| BUILD_UEFI_DRIVER | ON | Build UEFI driver |
| BUILD_EXAMPLES | ON | Build example programs |

```bash
cmake -DBUILD_EBPF_BACKEND=OFF ..
cmake -DBUILD_UEFI_DRIVER=OFF ..
cmake -DBUILD_EXAMPLES=OFF ..
```

## Warning

UEFI currently is not supported! If U wanna help and add UEFI support, U'r welcome!

Also U can create pool request for that! (And for CMake version downgrade... (Visual Studio moment...))

## License

GNU GPL 3 License — see LICENSE for details.

## Credits

Built by Monster

GitHub: Cyberflex1984

Tg: t.me/M0NST3R_65537

Tg: t.me/snowflake_65537