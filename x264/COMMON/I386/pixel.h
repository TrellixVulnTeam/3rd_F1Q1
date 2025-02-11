/*****************************************************************************
 * mc.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: pixel.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _I386_PIXEL_H
#define _I386_PIXEL_H 1

int x264_pixel_sad_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_16x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_8x4_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_4x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_4x4_mmxext( uint8_t *, int, uint8_t *, int );

int x264_pixel_ssd_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_16x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_8x4_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_4x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_ssd_4x4_mmxext( uint8_t *, int, uint8_t *, int );

int x264_pixel_satd_16x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_16x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x16_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_8x4_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_4x8_mmxext( uint8_t *, int, uint8_t *, int );
int x264_pixel_satd_4x4_mmxext( uint8_t *, int, uint8_t *, int );

int x264_pixel_sad_16x16_sse( uint8_t *, int, uint8_t *, int );
int x264_pixel_sad_16x8_sse( uint8_t *, int, uint8_t *, int );

#endif
