// MemlibDxe.c
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include "SetServicePointer.h"

EFI_SET_VARIABLE oSetVariable = NULL;

EFI_STATUS EFIAPI hk_SetVariable(
    IN CHAR16* VariableName,
    IN EFI_GUID* VendorGuid,
    IN UINT32 Attributes,
    IN UINTN DataSize,
    IN VOID* Data
){

    return oSetVariable(VariableName, VendorGuid, Attributes, DataSize, Data);
}

EFI_STATUS EFIAPI MemlibDxeEntryPoint(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE* SystemTable
) {
    Print(L"Hello from Memlib!\n");

    Print(L"Hooking SetVariable...\n");
    Print(L"Original function: %p\n", gRT->SetVariable);
    Print(L"Hooked function: %p\n", hk_SetVariable);

    oSetVariable = (EFI_SET_VARIABLE)SetServicePointer(&gRT->Hdr, (VOID**)&gRT->SetVariable, (VOID*)hk_SetVariable);

    return EFI_SUCCESS;
}