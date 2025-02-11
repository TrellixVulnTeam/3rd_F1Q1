/*****************************************************************************
 * cpu.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cpu.c,v 1.1.1.1 2005/03/16 13:27:22 264 Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "x264.h"
#include "cpu.h"

#ifdef ARCH_X86
extern int  x264_cpu_cpuid_test( void );
extern uint32_t  x264_cpu_cpuid( uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx );
extern void x264_emms( void );

uint32_t x264_cpu_detect( void )
{
    uint32_t cpu = 0;

    uint32_t eax, ebx, ecx, edx;
    int      b_amd;


    if( !x264_cpu_cpuid_test() )
    {
        /* No cpuid */
        return 0;
    }

    x264_cpu_cpuid( 0, &eax, &ebx, &ecx, &edx);
    if( eax == 0 )
    {
        return 0;
    }
    b_amd   = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

    x264_cpu_cpuid( 1, &eax, &ebx, &ecx, &edx );
    if( (edx&0x00800000) == 0 )
    {
        /* No MMX */
        return 0;
    }
    cpu = X264_CPU_MMX;
    if( (edx&0x02000000) )
    {
        /* SSE - identical to AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT|X264_CPU_SSE;
    }
    if( (edx&0x04000000) )
    {
        /* Is it OK ? */
        cpu |= X264_CPU_SSE2;
    }

    x264_cpu_cpuid( 0x80000000, &eax, &ebx, &ecx, &edx );
    if( eax < 0x80000001 )
    {
        /* no extended capabilities */
        return cpu;
    }

    x264_cpu_cpuid( 0x80000001, &eax, &ebx, &ecx, &edx );
    if( edx&0x80000000 )
    {
        cpu |= X264_CPU_3DNOW;
    }
    if( b_amd && (edx&0x00400000) )
    {
        /* AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT;
    }

    return cpu;
}

void     x264_cpu_restore( uint32_t cpu )
{
    if( cpu&(X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_3DNOW|X264_CPU_3DNOWEXT) )
    {
        x264_emms();
    }
}


#if 0
/*
 * XXX: adapted from libmpeg2 */
#if 0
#define cpuid(op,eax,ebx,ecx,edx)   \
    __asm__ ("push %%ebx\n\t"       \
             "cpuid\n\t"            \
             "movl %%ebx,%1\n\t"    \
             "pop %%ebx"        \
             : "=a" (eax),      \
               "=r" (ebx),      \
               "=c" (ecx),      \
               "=d" (edx)       \
             : "a" (op)         \
             : "cc")
#endif

uint32_t x264_cpu_detect( void )
{
    uint32_t cpu = 0;

    uint32_t eax, ebx, ecx, edx;
    int      b_amd;


    /* Test if cpuid is supported */
    asm volatile(
        "pushf\n"
        "pushf\n"
        "pop %0\n"
        "movl %0,%1\n"
        "xorl $0x200000,%0\n"
        "push %0\n"
        "popf\n"
        "pushf\n"
        "pop %0\n"
        "popf\n"
         : "=r" (eax), "=r" (ebx) : : "cc");

    if( eax == ebx )
    {
        /* No cpuid */
        return 0;
    }

    cpuid( 0, eax, ebx, ecx, edx);
    if( eax == 0 )
    {
        return 0;
    }
    b_amd   = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

    cpuid( 1, eax, ebx, ecx, edx );
    if( (edx&0x00800000) == 0 )
    {
        /* No MMX */
        return 0;
    }
    cpu = X264_CPU_MMX;
    if( (edx&0x02000000) )
    {
        /* SSE - identical to AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT|X264_CPU_SSE;
    }
    if( (edx&0x04000000) )
    {
        /* Is it OK ? */
        cpu |= X264_CPU_SSE2;
    }

    cpuid( 0x80000000, eax, ebx, ecx, edx );
    if( eax < 0x80000001 )
    {
        /* no extended capabilities */
        return cpu;
    }

    cpuid( 0x80000001, eax, ebx, ecx, edx );
    if( edx&0x80000000 )
    {
        cpu |= X264_CPU_3DNOW;
    }
    if( b_amd && (edx&0x00400000) )
    {
        /* AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT;
    }

    return cpu;
}
#endif

#elif defined( ARCH_PPC )

#ifdef SYS_MACOSX
#include <sys/sysctl.h>
uint32_t x264_cpu_detect( void )
{
    /* Thank you VLC */
    uint32_t cpu = 0;
    int      selectors[2] = { CTL_HW, HW_VECTORUNIT };
    int      has_altivec = 0;
    size_t   length = sizeof( has_altivec );
    int      error = sysctl( selectors, 2, &has_altivec, &length, NULL, 0 );

    if( error == 0 && has_altivec != 0 )
    {
        cpu |= X264_CPU_ALTIVEC;
    }

    return cpu;
}

#elif defined( SYS_LINUX )
uint32_t x264_cpu_detect( void )
{
    /* FIXME (Linux PPC) */
    return X264_CPU_ALTIVEC;
}
#endif

void     x264_cpu_restore( uint32_t cpu )
{
}

#else

uint32_t x264_cpu_detect( void )
{
    return 0;
}

void     x264_cpu_restore( uint32_t cpu )
{
}

#endif

