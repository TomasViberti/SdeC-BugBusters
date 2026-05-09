#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print(L"Iniciando analisis de seguridad...\r\n");

  unsigned char code[] = { 0xCC };
  if (code[0] == 0xCC) {
    Print(L"Breakpoint estatico alcanzado.\r\n");
  }

  return EFI_SUCCESS;
}
