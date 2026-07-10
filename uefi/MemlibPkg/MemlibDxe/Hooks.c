#include "Hooks.h"

#include "SetServicePointer.h"

#define MEMLIB_GUID \
    { 0x4317DF5D, 0x8D31, 0x43D7, { 0xA0, 0x9F, 0xDD, 0x5D, 0x6D, 0xD4, 0x47, 0x33 } }

EFI_SET_VARIABLE OriginalSetVariable = NULL;

EFI_EVENT ExitBootServicesEvent = NULL;
EFI_EVENT VirtualAddressChangeEvent = NULL;

EFI_STATUS EFIAPI hk_SetVariable(
    IN CHAR16* VariableName,
    IN EFI_GUID* VendorGuid,
    IN UINT32 Attributes,
    IN UINTN DataSize,
    IN VOID* Data
){
    
    EFI_GUID MemlibGuid = MEMLIB_GUID;
    if(CompareGuid(VendorGuid, &MemlibGuid) == 0) { //it's memlib!
        if(VariableName != NULL && StrCmp(VariableName, L"Loop") == 0){
            while(1){

            }
        }
    }
    
    return OriginalSetVariable(VariableName, VendorGuid, Attributes, DataSize, Data);
}

VOID EFIAPI OnVirtualAddressChange(EFI_EVENT Event, VOID* Context){
    gRT->ConvertPointer(EFI_OPTIONAL_PTR, (VOID**)&OriginalSetVariable);
}

int Hook_SetEfiVariable() {
    EFI_GUID MemlibGuid = MEMLIB_GUID;
    UINT8 dummy = 0;
    gRT->SetVariable(
        L"MemlibLoaded",
        &MemlibGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(dummy),
        &dummy
    );

    Print(L"Hooking SetVariable...\n");
    Print(L"Original function: %p\n", gRT->SetVariable);
    Print(L"Hook function: %p\n", &hk_SetVariable);
    OriginalSetVariable = (EFI_SET_VARIABLE)SetServicePointer(&gRT->Hdr, (VOID**)&gRT->SetVariable, hk_SetVariable);
    if(OriginalSetVariable == NULL){
        return 1;
    }

    Print(L"Setuping VirtualAddressChangeEvent Event...\n");
    EFI_STATUS status = gBS->CreateEventEx(
        EVT_NOTIFY_SIGNAL,
        TPL_NOTIFY,
        OnVirtualAddressChange,
        NULL,
        &gEfiEventVirtualAddressChangeGuid,
        &VirtualAddressChangeEvent
    );
    if(EFI_ERROR(status)){
        Print(L"Failed to setup VirtualAddressChangeEvent\n");
        return 1;
    }


    return 0;
}