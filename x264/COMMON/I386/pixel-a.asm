;*****************************************************************************
;* pixel.asm: h264 encoder library
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

%macro SAD_INC_2x16P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]
    movq    mm3,    [eax+8]
    movq    mm4,    [ecx+8]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    movq    mm1,    [eax+ebx]
    movq    mm2,    [ecx+edx]
    movq    mm3,    [eax+ebx+8]
    movq    mm4,    [ecx+edx+8]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
%endmacro

%macro SAD_INC_2x8P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]
    movq    mm3,    [eax+ebx]
    movq    mm4,    [ecx+edx]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
%endmacro

%macro SAD_INC_2x4P 0
    movd    mm1,    [eax]
    movd    mm2,    [ecx]
    movd    mm3,    [eax+ebx]
    movd    mm4,    [ecx+edx]

    psadbw  mm1,    mm2
    psadbw  mm3,    mm4
    paddw   mm0,    mm1
    paddw   mm0,    mm3

    lea     eax,    [eax+2*ebx]
    lea     ecx,    [ecx+2*edx]
%endmacro

%macro SSD_INC_1x16P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]
    movq    mm3,    [eax+8]
    movq    mm4,    [ecx+8]

    movq    mm5,    mm2
    movq    mm6,    mm4
    psubusb mm2,    mm1
    psubusb mm4,    mm3
    psubusb mm1,    mm5
    psubusb mm3,    mm6
    por     mm1,    mm2
    por     mm3,    mm4

    movq    mm2,    mm1
    movq    mm4,    mm3
    punpcklbw mm1,  mm7
    punpcklbw mm3,  mm7
    punpckhbw mm2,  mm7
    punpckhbw mm4,  mm7
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2
    pmaddwd mm3,    mm3
    pmaddwd mm4,    mm4

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
    paddd   mm0,    mm2
    paddd   mm0,    mm3
    paddd   mm0,    mm4
%endmacro

%macro SSD_INC_1x8P 0
    movq    mm1,    [eax]
    movq    mm2,    [ecx]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2         ; mm1 = 8bit abs diff

    movq    mm2,    mm1
    punpcklbw mm1,  mm7
    punpckhbw mm2,  mm7         ; (mm1,mm2) = 16bit abs diff
    pmaddwd mm1,    mm1
    pmaddwd mm2,    mm2

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
    paddd   mm0,    mm2
%endmacro

%macro SSD_INC_1x4P 0
    movd    mm1,    [eax]
    movd    mm2,    [ecx]

    movq    mm5,    mm2
    psubusb mm2,    mm1
    psubusb mm1,    mm5
    por     mm1,    mm2
    punpcklbw mm1,  mm7
    pmaddwd mm1,    mm1

    add     eax,    ebx
    add     ecx,    edx
    paddd   mm0,    mm1
%endmacro

%macro SSD_INC_8x16P 0
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
    SSD_INC_1x16P
%endmacro

%macro SSD_INC_4x8P 0
    SSD_INC_1x8P
    SSD_INC_1x8P
    SSD_INC_1x8P
    SSD_INC_1x8P
%endmacro

%macro SSD_INC_4x4P 0
    SSD_INC_1x4P
    SSD_INC_1x4P
    SSD_INC_1x4P
    SSD_INC_1x4P
%endmacro

%macro LOAD_DIFF_4P 5  ; MMP, MMT, MMZ, [pix1], [pix2]
    movd        %1, %4
    punpcklbw   %1, %3
    movd        %2, %5
    punpcklbw   %2, %3
    psubw       %1, %2
%endmacro

%macro LOAD_DIFF_INC_4x4 11 ; p1,p2,p3,p4, t, z, pix1, i_pix1, pix2, i_pix2, offset
    LOAD_DIFF_4P %1, %5, %6, [%7+%11],    [%9+%11]
    LOAD_DIFF_4P %2, %5, %6, [%7+%8+%11], [%9+%10+%11]
    lea %7, [%7+2*%8]
    lea %9, [%9+2*%10]
    LOAD_DIFF_4P %3, %5, %6, [%7+%11],    [%9+%11]
    LOAD_DIFF_4P %4, %5, %6, [%7+%8+%11], [%9+%10+%11]
    lea %7, [%7+2*%8]
    lea %9, [%9+2*%10]
%endmacro

%macro HADAMARD4_SUB_BADC 4
    paddw %1,   %2
    paddw %3,   %4
    paddw %2,   %2
    paddw %4,   %4
    psubw %2,   %1
    psubw %4,   %3
%endmacro

%macro HADAMARD4x4 4
    HADAMARD4_SUB_BADC %1, %2, %3, %4
    HADAMARD4_SUB_BADC %1, %3, %2, %4
%endmacro

%macro SBUTTERFLYwd 3
    movq        %3, %1
    punpcklwd   %1, %2
    punpckhwd   %3, %2
%endmacro

%macro SBUTTERFLYdq 3
    movq        %3, %1
    punpckldq   %1, %2
    punpckhdq   %3, %2
%endmacro

%macro TRANSPOSE4x4 5   ; abcd-t -> adtc
    SBUTTERFLYwd %1, %2, %5
    SBUTTERFLYwd %3, %4, %2
    SBUTTERFLYdq %1, %3, %4
    SBUTTERFLYdq %5, %2, %3
%endmacro

%macro MMX_ABS 2        ; mma, mmt
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
%endmacro

%macro MMX_ABS_SUM 3    ; mma, mmt, mms
    pxor    %2, %2
    psubw   %2, %1
    pmaxsw  %1, %2
    paddusw %3, %1
%endmacro


%macro MMX_SUM_MM 2     ; mmv, mmt
    movq    %2, %1
    psrlq   %1, 32
    paddusw %1, %2
    movq    %2, %1
    psrlq   %1, 16
    paddusw %1, %2
    movd    eax,%1
    and     eax,0xffff
    shr     eax,1
%endmacro

%macro HADAMARD4x4_FIRST 0
    HADAMARD4x4 mm0, mm1, mm2, mm3
    TRANSPOSE4x4 mm0, mm1, mm2, mm3, mm4
    HADAMARD4x4 mm0, mm3, mm4, mm2
    MMX_ABS     mm0, mm7
    MMX_ABS_SUM mm3, mm7, mm0
    MMX_ABS_SUM mm4, mm7, mm0
    MMX_ABS_SUM mm2, mm7, mm0
%endmacro

%macro HADAMARD4x4_NEXT 0
    HADAMARD4x4 mm1, mm2, mm3, mm4
    TRANSPOSE4x4 mm1, mm2, mm3, mm4, mm5
    HADAMARD4x4 mm1, mm4, mm5, mm3
    MMX_ABS_SUM mm1, mm7, mm0
    MMX_ABS_SUM mm4, mm7, mm0
    MMX_ABS_SUM mm5, mm7, mm0
    MMX_ABS_SUM mm3, mm7, mm0
%endmacro

;=============================================================================
; Code
;=============================================================================

SECTION .text

cglobal x264_pixel_sad_16x16_mmxext
cglobal x264_pixel_sad_16x8_mmxext
cglobal x264_pixel_sad_8x16_mmxext
cglobal x264_pixel_sad_8x8_mmxext
cglobal x264_pixel_sad_8x4_mmxext
cglobal x264_pixel_sad_4x8_mmxext
cglobal x264_pixel_sad_4x4_mmxext

cglobal x264_pixel_ssd_16x16_mmxext
cglobal x264_pixel_ssd_16x8_mmxext
cglobal x264_pixel_ssd_8x16_mmxext
cglobal x264_pixel_ssd_8x8_mmxext
cglobal x264_pixel_ssd_8x4_mmxext
cglobal x264_pixel_ssd_4x8_mmxext
cglobal x264_pixel_ssd_4x4_mmxext

cglobal x264_pixel_satd_4x4_mmxext
cglobal x264_pixel_satd_4x8_mmxext
cglobal x264_pixel_satd_8x4_mmxext
cglobal x264_pixel_satd_8x8_mmxext
cglobal x264_pixel_satd_16x8_mmxext
cglobal x264_pixel_satd_8x16_mmxext
cglobal x264_pixel_satd_16x16_mmxext

%macro SAD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm0,    mm0
%endmacro
%macro SAD_END 0
    movd eax,    mm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x16_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_16x8_mmxext:
    SAD_START
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_INC_2x16P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x16_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x8_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_8x4_mmxext:
    SAD_START
    SAD_INC_2x8P
    SAD_INC_2x8P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_4x8_mmxext:
    SAD_START
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_END

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_sad_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_sad_4x4_mmxext:
    SAD_START
    SAD_INC_2x4P
    SAD_INC_2x4P
    SAD_END



%macro SSD_START 0
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7         ; zero
    pxor    mm0,    mm0         ; mm0 holds the sum
%endmacro

%macro SSD_END 0
    movq    mm1,    mm0
    psrlq   mm1,    32
    paddd   mm0,    mm1
    movd    eax,    mm0

    pop ebx
    ret
%endmacro

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_ssd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_ssd_16x16_mmxext:
    SSD_START
    SSD_INC_8x16P
    SSD_INC_8x16P
    SSD_END

ALIGN 16
x264_pixel_ssd_16x8_mmxext:
    SSD_START
    SSD_INC_8x16P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x16_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x8_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_8x4_mmxext:
    SSD_START
    SSD_INC_4x8P
    SSD_END

ALIGN 16
x264_pixel_ssd_4x8_mmxext:
    SSD_START
    SSD_INC_4x4P
    SSD_INC_4x4P
    SSD_END

ALIGN 16
x264_pixel_ssd_4x4_mmxext:
    SSD_START
    SSD_INC_4x4P
    SSD_END



ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x4_mmxext:
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7

    LOAD_DIFF_4P mm0, mm6, mm7, [eax],       [ecx]
    LOAD_DIFF_4P mm1, mm6, mm7, [eax+ebx],   [ecx+edx]
    LOAD_DIFF_4P mm2, mm6, mm7, [eax+2*ebx], [ecx+2*edx]
    add eax, ebx
    add ecx, edx
    LOAD_DIFF_4P mm3, mm6, mm7, [eax+2*ebx], [ecx+2*edx]

    HADAMARD4x4_FIRST

    MMX_SUM_MM  mm0, mm7
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_4x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_4x8_mmxext:
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x4_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x4_mmxext:
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    mov     eax,    [esp+ 8]    ; pix1
    mov     ecx,    [esp+16]    ; pix2

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x8_mmxext:
    push    ebx

    mov     eax,    [esp+ 8]    ; pix1
    mov     ebx,    [esp+12]    ; stride1
    mov     ecx,    [esp+16]    ; pix2
    mov     edx,    [esp+20]    ; stride2

    pxor    mm7,    mm7

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    mov     eax,    [esp+ 8]    ; pix1
    mov     ecx,    [esp+16]    ; pix2

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x8_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x8_mmxext:
    push    ebx
    push    ebp

    mov     eax,    [esp+12]    ; pix1
    mov     ebx,    [esp+16]    ; stride1
    mov     ecx,    [esp+20]    ; pix2
    mov     edx,    [esp+24]    ; stride2

    pxor    mm7,    mm7
    xor     ebp,    ebp

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    mov     ebp, eax

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_NEXT

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    add         eax, ebp

    pop     ebp
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_8x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_8x16_mmxext:
    push    ebx
    push    ebp

    mov     eax,    [esp+12]    ; pix1
    mov     ebx,    [esp+16]    ; stride1
    mov     ecx,    [esp+20]    ; pix2
    mov     edx,    [esp+24]    ; stride2

    pxor    mm7,    mm7
    xor     ebp,    ebp

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    mov     ebp, eax

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    add     eax,    ebp

    pop     ebp
    pop     ebx
    ret

ALIGN 16
;-----------------------------------------------------------------------------
;   int __cdecl x264_pixel_satd_16x16_mmxext (uint8_t *, int, uint8_t *, int )
;-----------------------------------------------------------------------------
x264_pixel_satd_16x16_mmxext:
    push    ebx
    push    ebp

    mov     eax,    [esp+12]    ; pix1
    mov     ebx,    [esp+16]    ; stride1
    mov     ecx,    [esp+20]    ; pix2
    mov     edx,    [esp+24]    ; stride2

    pxor    mm7,    mm7
    xor     ebp,    ebp

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 0
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    mov     ebp, eax

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 4
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    add     ebp,    eax

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 8
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    add     ebp,    eax

    mov     eax,    [esp+12]    ; pix1
    mov     ecx,    [esp+20]    ; pix2

    LOAD_DIFF_INC_4x4 mm0, mm1, mm2, mm3, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_FIRST

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_NEXT

    LOAD_DIFF_INC_4x4 mm1, mm2, mm3, mm4, mm6, mm7, eax, ebx, ecx, edx, 12
    HADAMARD4x4_NEXT

    MMX_SUM_MM  mm0, mm7
    add     eax,    ebp

    pop     ebp
    pop     ebx
    ret

