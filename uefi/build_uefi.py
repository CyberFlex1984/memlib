#!/usr/bin/env python3
import os
import subprocess
import shutil
import argparse
import platform
import shlex
import tempfile
from pathlib import Path

def detect_os():
    return platform.system().lower()

def detect_toolchain(arch):
    os_name = detect_os()
    if os_name == 'windows':
        return 'VS2022'
    elif os_name == 'linux':
        return 'GCC5'
    elif os_name == 'darwin':
        return 'CLANGPDB' if arch == 'AARCH64' else 'XCODE5'
    return 'GCC5'

def detect_arch():
    machine = platform.machine().lower()
    if machine in ('x86_64', 'amd64'):
        return 'X64'
    elif machine in ('aarch64', 'arm64'):
        return 'AARCH64'
    raise RuntimeError(f'Unsupported host architecture: {machine}')

def build_environment(os_name, toolchain, arch):
    env = os.environ.copy()
    for variable in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL'):
        env.pop(variable, None)

    if toolchain == 'CLANGPDB':
        required_tools = ('clang', 'lld-link', 'llvm-lib', 'llvm-rc')
        tool_paths = {tool: shutil.which(tool) for tool in required_tools}
        missing = [tool for tool, path in tool_paths.items() if path is None]
        if missing:
            raise RuntimeError(
                'A complete LLVM toolchain is required for AARCH64 UEFI on macOS; '
                f'missing: {", ".join(missing)}'
            )
        tool_dirs = {str(Path(path).resolve().parent) for path in tool_paths.values()}
        if len(tool_dirs) == 1:
            env['CLANG_BIN'] = tool_dirs.pop() + os.sep
        else:
            shim_dir = Path(tempfile.mkdtemp(prefix='memlib-llvm-'))
            for tool, path in tool_paths.items():
                (shim_dir / tool).symlink_to(Path(path).resolve())
            env['CLANG_BIN'] = str(shim_dir) + os.sep
            env['MEMLIB_LLVM_SHIM_DIR'] = str(shim_dir)

    if os_name == 'linux' and toolchain == 'GCC5':
        prefix_variable = f'GCC5_{arch}_PREFIX'
        env.setdefault(prefix_variable, '')

    return env

def build_uefi_driver(edk2_path, project_path, output_dir):
    os_name = detect_os()
    arch = detect_arch()
    toolchain = detect_toolchain(arch)
    env = build_environment(os_name, toolchain, arch)
    shim_dir = env.pop('MEMLIB_LLVM_SHIM_DIR', None)

    print(f"OS: {os_name}")
    print(f"Toolchain: {toolchain}")
    print(f"Architecture: {arch}")

    edk2_path = Path(edk2_path).resolve()
    project_path = Path(project_path).resolve()
    output_dir = Path(output_dir).resolve()

    uefi_src = project_path / 'uefi' / 'MemlibPkg'
    uefi_dst = edk2_path / 'MemlibPkg'

    print(f"Copying {uefi_src} -> {uefi_dst}")
    if uefi_dst.exists():
        shutil.rmtree(uefi_dst)
    shutil.copytree(uefi_src, uefi_dst)

    try:
        if os_name == 'linux' or os_name == 'darwin':
            genfw = edk2_path / 'BaseTools' / 'Source' / 'C' / 'bin' / 'GenFw'
            if not genfw.exists():
                subprocess.run(
                    ['make', '-C', str(edk2_path / 'BaseTools')],
                    check=True,
                    env=env
                )
            shell_cmd = f"""
cd {shlex.quote(str(edk2_path))}
source edksetup.sh
build -a {arch} -t {toolchain} -p MemlibPkg/MemlibPkg.dsc -m MemlibPkg/MemlibDxe/MemlibDxe.inf -b DEBUG
"""
            print("Running EDK2 build...")
            subprocess.run(
                shell_cmd,
                shell=True,
                executable='/bin/bash',
                check=True,
                env=env
            )

        elif os_name == 'windows':
            shell_cmd = f"""
cd /d {edk2_path}
call edksetup.bat
build -a {arch} -t {toolchain} -p MemlibPkg/MemlibPkg.dsc -m MemlibPkg/MemlibDxe/MemlibDxe.inf -b DEBUG
"""
            subprocess.run(shell_cmd, shell=True, check=True, env=env)

        efi_path = edk2_path / 'Build' / 'MemlibPkg' / f'DEBUG_{toolchain}' / arch / 'MemlibDxe.efi'
        if not efi_path.exists():
            raise FileNotFoundError(f'.efi not found at {efi_path}')

        output_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy(efi_path, output_dir / 'MemlibDxe.efi')
        print(f"UEFI driver copied to: {output_dir / 'MemlibDxe.efi'}")
    finally:
        if uefi_dst.exists():
            shutil.rmtree(uefi_dst)
        if shim_dir is not None:
            shutil.rmtree(shim_dir)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--edk2-path', required=True)
    parser.add_argument('--project-path', required=True)
    parser.add_argument('--output-dir', default='./uefi')
    args = parser.parse_args()

    build_uefi_driver(args.edk2_path, args.project_path, args.output_dir)

if __name__ == '__main__':
    main()
