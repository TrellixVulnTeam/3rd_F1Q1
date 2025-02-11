/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: dct.h,v 1.1 2004/06/03 19:27:07 fenrir Exp $
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

#ifndef _I386_DCT_H
#define _I386_DCT_H 1

void x264_sub4x4_dct_mmxext( int16_t dct[4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
void x264_sub8x8_dct_mmxext( int16_t dct[4][4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );
void x264_sub16x16_dct_mmxext( int16_t dct[16][4][4],  uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 );

void x264_add4x4_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4] );
void x264_add8x8_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[4][4][4] );
void x264_add16x16_idct_mmxext( uint8_t *p_dst, int i_dst, int16_t dct[16][4][4] );

void x264_dct4x4dc_mmxext( int16_t d[4][4] );
void x264_idct4x4dc_mmxext( int16_t d[4][4] );

void quant4x4_sse2(int16_t dct[4][4], const int32_t Qp, int32_t is_intra);
void iquant4x4_sse2(int16_t dct[4][4], const int32_t Qp);

#endif
