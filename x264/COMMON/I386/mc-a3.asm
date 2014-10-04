;*****************************************************************************
;* mc-a2.asm: h264 encoder library
;*****************************************************************************


BITS 32

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%macro cglobal 1
    %ifdef PREFIX
        global _%1
        %define %1 _%1
    %else
        global %1
    %endif
%endmacro

;=============================================================================
; Read only data
;=============================================================================

SECTION .rodata data align=16

ALIGN 16
mmx_dw_one:
    times 4 dw 16
mmx_dd_one:
    times 2 dd 512
mmx_dw_20:
    times 4 dw 20
mmx_dw_5:
    times 4 dw -5

SECTION .data

width:
    dd 0
height:
    dd 0
dstp1:
    dd 0
dstp2:
    dd 0
buffer:
    dd 0
dst1:
    dd 0
dst2:
    dd 0
src:
    dd 0


;=============================================================================
; Macros
;=============================================================================

%macro LOAD_4 9
    movd %1, %5
    movd %2, %6
    movd %3, %7
    movd %4, %8
    punpcklbw %1, %9
    punpcklbw %2, %9
    punpcklbw %3, %9
    punpcklbw %4, %9
%endmacro

%macro FILT_2 2
    psubw %1, %2
    psllw %2, 2
    psubw %1, %2
%endmacro

%macro FILT_4 3
    paddw %2, %3
    psllw %2, 2
    paddw %1, %2
    psllw %2, 2
    paddw %1, %2
%endmacro

%macro FILT_6 4
    psubw %1, %2
    psllw %2, 2
    psubw %1, %2
    paddw %1, %3
    paddw %1, %4
    psraw %1, 5
%endmacro

%macro FILT_ALL 1
    LOAD_4      mm1, mm2, mm3, mm4, [%1], [%1 + ecx], [%1 + 2 * ecx], [%1 + ebx], mm0
    FILT_2      mm1, mm2
    movd        mm5, [%1 + 4 * ecx]
    movd        mm6, [%1 + edx]
    FILT_4      mm1, mm3, mm4
    punpcklbw   mm5, mm0
    punpcklbw   mm6, mm0
    psubw       mm1, mm5
    psllw       mm5, 2
    psubw       mm1, mm5
    paddw       mm1, mm6
%endmacro




;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264dec_center_filter_mmxext

;-----------------------------------------------------------------------------
;
; void x264dec_center_filter_mmxext( uint8_t *dst1, int i_dst1_stride,
;                                 uint8_t *dst2, int i_dst2_stride,
;                                  uint8_t *src, int i_src_stride,
;                                  int i_width, int i_height );
;
;-----------------------------------------------------------------------------

ALIGN 16
x264dec_center_filter_mmxext :

    push        edi
    push        esi
    push        ebx
    push        ebp

    mov         esi,      [esp + 36]         ; src

    mov         edx,      [esp + 20]         ; dst1
    mov         [dst1],   edx

    mov         edi,      [esp + 28]         ; dst2
    mov         [dst2],   edi

    mov         eax,      [esp + 44]         ; width
    mov         [width],  eax

    mov         eax,      [esp + 48]         ; height
    mov         [height], eax

    mov         eax,      [esp + 24]         ; dst1_stride
    mov         [dstp1],  eax

    mov         eax,      [esp + 32]         ; dst2_stride
    mov         [dstp2],  eax

    mov         ecx,      [esp + 40]         ; src_stride

    sub         esp,      ecx
    sub         esp,      ecx                ; esp is now at the beginning of the buffer
    mov         [buffer], esp

    ;sub        esi,      2
    sub         esi,      ecx
    sub         esi,      ecx                ; esi - 2 - 2 * stride
    mov         [src],    esi

    ;sub        edi,      2

    mov         ebx,      ecx
    shl         ebx,      1
    add         ebx,      ecx                ; 3 * src_stride

    mov         edx,      ecx
    shl         edx,      1
    add         edx,      ebx                ; 5 * src_stride

    pxor        mm0,      mm0                ; 0 ---> mm0
    movq        mm7,      [mmx_dd_one]       ; for rounding

    mov         ebp,      [height]

loopcy:

    dec         ebp
    mov         eax,    [width]
    mov         edi,    [dst1]
    mov         esp,    [buffer]
    mov         esi,    [src]

    FILT_ALL    esi

    pshufw      mm2,    mm1, 0
    movq        [esp],  mm2
    add         esp,    8
    movq        [esp],  mm1
    add         esp,    8
    paddw       mm1,    [mmx_dw_one]
    psraw       mm1,    5

    packuswb    mm1,    mm1
    movd        [edi],  mm1

    sub         eax,    8
    add         edi,    4
    add         esi,    4

loopcx1:

    sub         eax,    4

    FILT_ALL    esi

    movq        [esp],  mm1
    paddw       mm1,    [mmx_dw_one]
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [edi],  mm1

    add         esp,    8
    add         esi,    4
    add         edi,    4
    test        eax,    eax
    jnz         loopcx1

    FILT_ALL    esi

    pshufw      mm2,    mm1,  7
    movq        [esp],  mm1
    add         esp,    8
    movq        [esp],  mm2
    paddw       mm1,    [mmx_dw_one]
    psraw       mm1,    5
    packuswb    mm1,    mm1
    movd        [edi],  mm1

    mov         esi,    [src]
    add         esi,    ecx
    mov         [src],  esi

    mov         edi,    [dst1]
    add         edi,    [dstp1]
    mov         [dst1], edi

    mov         eax,    [width]
    mov         edi,    [dst2]
    mov         esp,    [buffer]
    add         esp,    4

loopcx2:

    sub         eax,    4

    movq        mm2,    [esp + 2 * eax + 2]
    movq        mm3,    [esp + 2 * eax + 4]
    movq        mm4,    [esp + 2 * eax + 6]
    movq        mm5,    [esp + 2 * eax + 8]
    movq        mm1,    [esp + 2 * eax]
    movq        mm6,    [esp + 2 * eax + 10]
    paddw       mm2,    mm5
    paddw       mm3,    mm4
    paddw       mm1,    mm6

    movq        mm5,    [mmx_dw_20]
    movq        mm4,    [mmx_dw_5]
    movq        mm6,    mm1
    pxor        mm7,    mm7

    punpckhwd   mm5,    mm2
    punpcklwd   mm4,    mm3
    punpcklwd   mm2,    [mmx_dw_20]
    punpckhwd   mm3,    [mmx_dw_5]

    pcmpgtw     mm7,    mm1

    pmaddwd     mm2,    mm4
    pmaddwd     mm3,    mm5

    punpcklwd   mm1,    mm7
    punpckhwd   mm6,    mm7

    paddd       mm2,    mm1
    paddd       mm3,    mm6

    paddd       mm2,    [mmx_dd_one]
    paddd       mm3,    [mmx_dd_one]

    psrad       mm2,    10
    psrad       mm3,    10

    packssdw    mm2,    mm3
    packuswb    mm2,    mm0

    movd        [edi + eax], mm2

    test        eax,    eax
    jnz         loopcx2

    add         edi,    [dstp2]
    mov         [dst2], edi

    test        ebp,    ebp
    jnz         loopcy

    mov         esp,    [buffer]
    shl         ecx,    1
    add         esp,    ecx

    pop         ebp
    pop         ebx
    pop         esi
    pop         edi

    ret
