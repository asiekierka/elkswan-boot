#include <wonderful.h>
#include <ws.h>
#include "memory.h"

    .arch   i186
    .code16
    .intel_syntax noprefix

    .section .text, "ax"
    .global launch_entrypoint
launch_entrypoint:
    mov bx, [text_size]
    mov cx, DATA_SEG
    xor di, di
    mov si, [data_size]
    mov dx, [bss_size]
    mov sp, 0x2000
    push [entry_seg]
    push [entry_ofs]

    mov ds, cx
    mov es, cx
    retf