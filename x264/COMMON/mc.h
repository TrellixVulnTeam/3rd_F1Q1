/*****************************************************************************
 * mc.h: h264 encoder library (Motion Compensation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: mc.h,v 1.1.1.1 2005/03/16 13:27:22 264 Exp $
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

#ifndef _MC_H
#define _MC_H 1

/* Do the MC
 * XXX: Only width = 4, 8 or 16 are valid
 * width == 4 -> height == 4 or 8
 * width == 8 -> height == 4 or 8 or 16
 * width == 16-> height == 8 or 16
 * */

typedef struct
{
    void (*mc_luma)(uint8_t **, int, uint8_t *, int,
                    int mvx, int mvy,
                    int i_width, int i_height );

    uint8_t* (*get_ref)(uint8_t **, int, uint8_t *, int *,
                        int mvx, int mvy,
                        int i_width, int i_height );

    void (*mc_chroma)(uint8_t *, int, uint8_t *, int,
                      int mvx, int mvy,
                      int i_width, int i_height );
} x264_mc_functions_t;

void x264_mc_init( int cpu, x264_mc_functions_t *pf );

void x264_pixel_avg_w4_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int  );
void x264_pixel_avg_w8_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int  );
void x264_pixel_avg_w16_mmxext( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int  );
void x264_pixel_avg_w16_sse2( uint8_t *,  int, uint8_t *, int, uint8_t *, int, int  );

void x264_mc_copy_w4_mmxext( uint8_t *, int, uint8_t *, int, int );
void x264_mc_copy_w8_mmxext( uint8_t *, int, uint8_t *, int, int );
void x264_mc_copy_w16_mmxext( uint8_t *, int, uint8_t *, int, int );
void x264_mc_copy_w16_sse2( uint8_t *, int, uint8_t *, int, int );
#endif
