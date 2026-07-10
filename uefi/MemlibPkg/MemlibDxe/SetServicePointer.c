#include "SetServicePointer.h"

#define CR0_WP         ((UINTN)0x00010000)
#define CR0_PG         ((UINTN)0x80000000)
#define CR4_CET        ((UINTN)0x00800000)
#define CR4_LA57       ((UINTN)0x00001000)
#define MSR_EFER       ((UINTN)0xC0000080)
#define EFER_LMA       ((UINTN)0x00000400)
#define EFER_UAIE      ((UINTN)0x00100000)

VOID* SetServicePointer(
    IN OUT EFI_TABLE_HEADER* ServiceTableHeader,
    IN OUT VOID **ServiceTableFunction,
    IN VOID *NewFunction
) {
    if (ServiceTableHeader == NULL || ServiceTableFunction == NULL || NewFunction == NULL) {
        return NULL;
    }
    
    if (gBS == NULL || gBS->CalculateCrc32 == NULL) {
        return NULL;
    }

    EFI_TPL OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);

    UINTN Cr0 = AsmReadCr0();
    BOOLEAN WpSet = (Cr0 & CR0_WP) != 0;
    if (WpSet) {
        AsmWriteCr0(Cr0 & ~CR0_WP);
    }

    VOID* OriginalFunction = *ServiceTableFunction;

    if (OriginalFunction == NewFunction) {
        if (WpSet) {
            AsmWriteCr0(Cr0);
        }
        gBS->RestoreTPL(OldTpl);
        return OriginalFunction;
    }

    *ServiceTableFunction = NewFunction;

    ServiceTableHeader->CRC32 = 0;
    gBS->CalculateCrc32((UINT8*)ServiceTableHeader, ServiceTableHeader->HeaderSize, &ServiceTableHeader->CRC32);

    if (WpSet) {
        AsmWriteCr0(Cr0);
    }

    gBS->RestoreTPL(OldTpl);

    return OriginalFunction;
}