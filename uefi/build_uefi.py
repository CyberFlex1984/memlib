#!/usr/bin/env python3
import os
import sys
import subprocess
import shutil
import argparse
import platform
from pathlib import Path

def detect_os():
    return platform.system().lower()

def detect_toolchain():
    os_name = detect_os()
    if os_name == 'windows':
        return 'VS2022'
    elif os_name == 'linux':
        return 'GCC'
    elif os_name == 'darwin':
        return 'XCODE5'
    return 'GCC'

def detect_arch():
    machine = platform.machine().lower()
    if machine in ('x86_64', 'amd64'):
        return 'X64'
    elif machine in ('aarch64', 'arm64'):
        return 'AARCH64'
    return 'X64'

def build_uefi_driver(edk2_path, project_path, output_dir):
    os_name = detect_os()
    toolchain = detect_toolchain()
    arch = detect_arch()

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

    if os_name == 'linux' or os_name == 'darwin':
        shell_cmd = f"""
cd {edk2_path}
source edksetup.sh
build -a {arch} -t {toolchain} -p MemlibPkg/MemlibPkg.dsc -m MemlibPkg/MemlibDxe/MemlibDxe.inf -b DEBUG
"""
        print("Running EDK2 build...")
        subprocess.run(shell_cmd, shell=True, executable='/bin/bash', check=True)

    elif os_name == 'windows':
        shell_cmd = f"""
cd {edk2_path}
call edksetup.bat
build -a {arch} -t {toolchain} -p MemlibPkg/MemlibPkg.dsc -m MemlibPkg/MemlibDxe/MemlibDxe.inf -b DEBUG
"""
        subprocess.run(shell_cmd, shell=True, check=True)


    efi_path = edk2_path / 'Build' / 'MemlibPkg' / f'DEBUG_{toolchain}' / arch / 'MemlibDxe.efi'
    if not efi_path.exists():
        efi_path = edk2_path / 'Build' / 'MemlibPkg' / f'DEBUG_{toolchain}' / 'X64' / 'MemlibDxe.efi'

    if efi_path.exists():
        output_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy(efi_path, output_dir / 'MemlibDxe.efi')
        print(f"UEFI driver copied to: {output_dir / 'MemlibDxe.efi'}")
        
        shutil.rmtree(uefi_dst)
    else:
        print(f"ERROR: .efi not found at {efi_path}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--edk2-path', required=True)
    parser.add_argument('--project-path', required=True)
    parser.add_argument('--output-dir', default='./uefi')
    args = parser.parse_args()

    build_uefi_driver(args.edk2_path, args.project_path, args.output_dir)

if __name__ == '__main__':
    main()