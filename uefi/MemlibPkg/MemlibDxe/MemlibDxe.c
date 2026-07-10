// MemlibDxe.c
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include "Hooks.h"

EFI_STATUS EFIAPI MemlibDxeEntryPoint(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE* SystemTable
) {
    Print(L"Hello from Memlib!\n");

    if(Hook_SetEfiVariable()){
        Print(L"Failed to hook SetVariables and setup event!\n");
        return EFI_ACCESS_DENIED;
    }

    return EFI_SUCCESS;
}