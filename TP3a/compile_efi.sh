#!/bin/bash
# Compilar aplicacion.c para UEFI usando gnu-efi

set -e

SRCFILE="aplicacion.c"
OBJFILE="aplicacion.o"
SOFILE="aplicacion.so"
EFIFILE="aplicacion.efi"

EFI_INCLUDE="/usr/include/efi"
EFI_LIB="/usr/lib"
EFI_ARCH="x86_64"

echo "[*] Compilando $SRCFILE..."
gcc -I$EFI_INCLUDE -I$EFI_INCLUDE/$EFI_ARCH \
    -DEFI_FUNCTION_WRAPPER -fPIC -ffreestanding -fno-stack-protector \
    -fno-strict-aliasing -fshort-wchar -mno-red-zone \
    -c $SRCFILE -o $OBJFILE

echo "[*] Enlazando a $SOFILE..."
ld -nostdlib -shared -Bsymbolic \
   -L$EFI_LIB -T$EFI_LIB/elf_x86_64_efi.lds \
   $EFI_LIB/crt0-efi-x86_64.o $OBJFILE \
   -lgnuefi -lefi -o $SOFILE

echo "[*] Generando $EFIFILE..."
objcopy -j .text \
        -j .sdata \
        -j .data \
        -j .dynamic \
        -j .dynsym \
        -j .rel \
        -j .rela \
        -j .rel.* \
        -j .rela.* \
        -j .reloc \
        --target=efi-app-x86_64 \
        $SOFILE $EFIFILE

echo "[+] Compilación completada: $EFIFILE"
chmod +x $EFIFILE
ls -l $EFIFILE
