/*****************************************************************************
 * cabac.c: h264 encoder library
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: cabac.c,v 1.1.1.1 2005/03/16 13:27:22 264 Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common/common.h"
#include "macroblock.h"
//#include "common/global_tbl.h"




//static const uint8_t block_idx_x[16] =
static const DECLARE_ALIGNED( uint8_t, block_idx_x[16], 16 )=
{
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
//static const uint8_t block_idx_y[16] =
static const DECLARE_ALIGNED( uint8_t, block_idx_y[16], 16 )=
{
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};
//static const uint8_t block_idx_xy[4][4] =
static const DECLARE_ALIGNED( uint8_t, block_idx_xy[4][4], 16 )=
{
    { 0, 2, 8,  10},
    { 1, 3, 9,  11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};




static inline void x264_cabac_mb_type_intra( x264_t *h, int i_mb_type,
                    int ctx0, int ctx1, int ctx2, int ctx3, int ctx4, int ctx5 )
{
    if( i_mb_type == I_4x4 )
    {
        x264_cabac_encode_decision( &h->cabac, ctx0, 0 );
    }
    else if( i_mb_type == I_PCM )
    {
        x264_cabac_encode_decision( &h->cabac, ctx0, 1 );
        x264_cabac_encode_terminal( &h->cabac,       1 );
    }
    else
    {
        x264_cabac_encode_decision( &h->cabac, ctx0, 1 );
        x264_cabac_encode_terminal( &h->cabac,       0 );

        x264_cabac_encode_decision( &h->cabac, ctx1, ( h->mb.i_cbp_luma == 0 ? 0 : 1 ));
        if( h->mb.i_cbp_chroma == 0 )
        {
            x264_cabac_encode_decision( &h->cabac, ctx2, 0 );
        }
        else
        {
            x264_cabac_encode_decision( &h->cabac, ctx2, 1 );
            x264_cabac_encode_decision( &h->cabac, ctx3, ( h->mb.i_cbp_chroma == 1 ? 0 : 1 ) );
        }
        x264_cabac_encode_decision( &h->cabac, ctx4, ( (h->mb.i_intra16x16_pred_mode / 2) ? 1 : 0 ));
        x264_cabac_encode_decision( &h->cabac, ctx5, ( (h->mb.i_intra16x16_pred_mode % 2) ? 1 : 0 ));
    }
}

/******************************************************************
函数说明：采用CABAC的方式编码一个宏块的类型
参数：  x264_t *h	编码器的数据结构
  		  
返回值: 无
******************************************************************/
static void x264_cabac_mb_type( x264_t *h )
{
    const int i_mb_type = h->mb.i_type;

    if( h->sh.i_type == SLICE_TYPE_I )
    {
        int ctx = 0;
        if( h->mb.i_mb_x > 0 && h->mb.type[h->mb.i_mb_xy - 1] != I_4x4 )
        {
            ctx++;
        }
        if( h->mb.i_mb_y > 0 && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride] != I_4x4 )
        {
            ctx++;
        }

        x264_cabac_mb_type_intra( h, i_mb_type, 3+ctx, 3+3, 3+4, 3+5, 3+6, 3+7 );
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        /* prefix: 14, suffix: 17 */
        if( i_mb_type == P_L0 )
        {
            if( h->mb.i_partition == D_16x16 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 0 );
                x264_cabac_encode_decision( &h->cabac, 16, 0 );
            }
            else if( h->mb.i_partition == D_16x8 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 1 );
                x264_cabac_encode_decision( &h->cabac, 17, 1 );
            }
            else if( h->mb.i_partition == D_8x16 )
            {
                x264_cabac_encode_decision( &h->cabac, 14, 0 );
                x264_cabac_encode_decision( &h->cabac, 15, 1 );
                x264_cabac_encode_decision( &h->cabac, 17, 0 );
            }
        }
        else if( i_mb_type == P_8x8 )
        {
            x264_cabac_encode_decision( &h->cabac, 14, 0 );
            x264_cabac_encode_decision( &h->cabac, 15, 0 );
            x264_cabac_encode_decision( &h->cabac, 16, 1 );
        }
        else /* intra */
        {
            /* prefix */
            x264_cabac_encode_decision( &h->cabac, 14, 1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, i_mb_type, 17+0, 17+1, 17+2, 17+2, 17+3, 17+3 );
        }
    }
    else if( h->sh.i_type == SLICE_TYPE_B )
    {
        int ctx = 0;
        if( h->mb.i_mb_x > 0 && h->mb.type[h->mb.i_mb_xy - 1] != B_SKIP && h->mb.type[h->mb.i_mb_xy - 1] != B_DIRECT )
        {
            ctx++;
        }
        if( h->mb.i_mb_y > 0 && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride] != B_SKIP && h->mb.type[h->mb.i_mb_xy - h->mb.i_mb_stride] != B_DIRECT )
        {
            ctx++;
        }

        if( i_mb_type == B_DIRECT )
        {
            x264_cabac_encode_decision( &h->cabac, 27+ctx, 0 );
        }
        else if( i_mb_type == B_8x8 )
        {
            x264_cabac_encode_decision( &h->cabac, 27+ctx, 1 );
            x264_cabac_encode_decision( &h->cabac, 27+3,   1 );
            x264_cabac_encode_decision( &h->cabac, 27+4,   1 );

            x264_cabac_encode_decision( &h->cabac, 27+5,   1 );
            x264_cabac_encode_decision( &h->cabac, 27+5,   1 );
            x264_cabac_encode_decision( &h->cabac, 27+5,   1 );
        }
        else if( IS_INTRA( i_mb_type ) )
        {
            /* prefix */
            x264_cabac_encode_decision( &h->cabac, 27+ctx, 1 );
            x264_cabac_encode_decision( &h->cabac, 27+3,   1 );
            x264_cabac_encode_decision( &h->cabac, 27+4,   1 );

            x264_cabac_encode_decision( &h->cabac, 27+5,   1 );
            x264_cabac_encode_decision( &h->cabac, 27+5,   0 );
            x264_cabac_encode_decision( &h->cabac, 27+5,   1 );

            /* suffix */
            x264_cabac_mb_type_intra( h, i_mb_type, 32+0, 32+1, 32+2, 32+2, 32+3, 32+3 );
        }
        else
        {
            static const int i_mb_len[21] =
            {
                3, 6, 6,    /* L0 L0 */
                3, 6, 6,    /* L1 L1 */
                6, 7, 7,    /* BI BI */

                6, 6,       /* L0 L1 */
                6, 6,       /* L1 L0 */
                7, 7,       /* L0 BI */
                7, 7,       /* L1 BI */
                7, 7,       /* BI L0 */
                7, 7,       /* BI L1 */
            };
            static const int i_mb_bits[21][7] =
            {
                { 1, 0, 0, },            { 1, 1, 0, 0, 0, 1, },    { 1, 1, 0, 0, 1, 0, },   /* L0 L0 */
                { 1, 0, 1, },            { 1, 1, 0, 0, 1, 1, },    { 1, 1, 0, 1, 0, 0, },   /* L1 L1 */
                { 1, 1, 0, 0, 0, 0 ,},   { 1, 1, 1, 1, 0, 0 , 0 }, { 1, 1, 1, 1, 0, 0 , 1 },/* BI BI */

                { 1, 1, 0, 1, 0, 1, },   { 1, 1, 0, 1, 1, 0, },     /* L0 L1 */
                { 1, 1, 0, 1, 1, 1, },   { 1, 1, 1, 1, 1, 0, },     /* L1 L0 */
                { 1, 1, 1, 0, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0, 1 },   /* L0 BI */
                { 1, 1, 1, 0, 0, 1, 0 }, { 1, 1, 1, 0, 0, 1, 1 },   /* L1 BI */
                { 1, 1, 1, 0, 1, 0, 0 }, { 1, 1, 1, 0, 1, 0, 1 },   /* BI L0 */
                { 1, 1, 1, 0, 1, 1, 0 }, { 1, 1, 1, 0, 1, 1, 1 }    /* BI L1 */
            };

            const int i_partition = h->mb.i_partition;
            int idx = 0;
            int i;
            switch( i_mb_type )
            {
                /* D_16x16, D_16x8, D_8x16 */
                case B_BI_BI: idx += 3;
                case B_L1_L1: idx += 3;
                case B_L0_L0:
                    if( i_partition == D_16x8 )
                        idx += 1;
                    else if( i_partition == D_8x16 )
                        idx += 2;
                    break;

                /* D_16x8, D_8x16 */
                case B_BI_L1: idx += 2;
                case B_BI_L0: idx += 2;
                case B_L1_BI: idx += 2;
                case B_L0_BI: idx += 2;
                case B_L1_L0: idx += 2;
                case B_L0_L1:
                    idx += 3*3;
                    if( i_partition == D_8x16 )
                        idx++;
                    break;
                default:
                    x264_log(h, X264_LOG_ERROR, "error in B mb type\n" );
                    return;
            }

            x264_cabac_encode_decision( &h->cabac, 27+ctx,                         i_mb_bits[idx][0] );
            x264_cabac_encode_decision( &h->cabac, 27+3,                           i_mb_bits[idx][1] );
            x264_cabac_encode_decision( &h->cabac, 27+(i_mb_bits[idx][1] != 0 ? 4 : 5), i_mb_bits[idx][2] );
            for( i = 3; i < i_mb_len[idx]; i++ )
            {
                x264_cabac_encode_decision( &h->cabac, 27+5,                       i_mb_bits[idx][i] );
            }
        }
    }
    else
    {
        x264_log(h, X264_LOG_ERROR, "unknown SLICE_TYPE unsupported in x264_macroblock_write_cabac\n" );
    }
}

static void x264_cabac_mb_intra4x4_pred_mode( x264_t *h, int i_pred, int i_mode )
{
    if( i_pred == i_mode )
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( &h->cabac, 68, 1 );
    }
    else
    {
        /* b_prev_intra4x4_pred_mode */
        x264_cabac_encode_decision( &h->cabac, 68, 0 );
        if( i_mode > i_pred  )
        {
            i_mode--;
        }
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode     )&0x01 );
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode >> 1)&0x01 );
        x264_cabac_encode_decision( &h->cabac, 69, (i_mode >> 2)&0x01 );
    }
}
static void x264_cabac_mb_intra8x8_pred_mode( x264_t *h )
{
    const int i_mode  = h->mb.i_chroma_pred_mode;
    int       ctx = 0;

    /* No need to test for I4x4 or I_16x16 as cache_save handle that */
    if( h->mb.i_mb_x > 0 && h->mb.chroma_pred_mode[h->mb.i_mb_xy - 1] != 0 )
    {
        ctx++;
    }
    if( h->mb.i_mb_y > 0 && h->mb.chroma_pred_mode[h->mb.i_mb_xy - h->mb.i_mb_stride] != 0 )
    {
        ctx++;
    }

    if( i_mode == 0 )
    {
        x264_cabac_encode_decision( &h->cabac, 64 + ctx, 0 );
    }
    else
    {
        x264_cabac_encode_decision( &h->cabac, 64 + ctx, 1 );
        x264_cabac_encode_decision( &h->cabac, 64 + 3, ( i_mode == 1 ? 0 : 1 ) );
        if( i_mode > 1 )
        {
            x264_cabac_encode_decision( &h->cabac, 64 + 3, ( i_mode == 2 ? 0 : 1 ) );
        }
    }
}

static void x264_cabac_mb_cbp_luma( x264_t *h )
{
    /* TODO: clean up and optimize */
    int i8x8;
    for( i8x8 = 0; i8x8 < 4; i8x8++ )
    {
        int i_mba_xy = -1;
        int i_mbb_xy = -1;
        int x = block_idx_x[4*i8x8];
        int y = block_idx_y[4*i8x8];
        int ctx = 0;

        if( x > 0 )
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_x > 0 )
            i_mba_xy = h->mb.i_mb_xy - 1;

        if( y > 0 )
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_y > 0 )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;


        /* No need to test for PCM and SKIP */
        if( i_mba_xy >= 0 )
        {
            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
            if( ((h->mb.cbp[i_mba_xy] >> i8x8a)&0x01) == 0 )
            {
                ctx++;
            }
        }

        if( i_mbb_xy >= 0 )
        {
            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
            if( ((h->mb.cbp[i_mbb_xy] >> i8x8b)&0x01) == 0 )
            {
                ctx += 2;
            }
        }

        x264_cabac_encode_decision( &h->cabac, 73 + ctx, (h->mb.i_cbp_luma >> i8x8)&0x01 );
    }
}

static void x264_cabac_mb_cbp_chroma( x264_t *h )
{
    int cbp_a = -1;
    int cbp_b = -1;
    int ctx;

    /* No need to test for SKIP/PCM */
    if( h->mb.i_mb_x > 0 )
    {
        cbp_a = (h->mb.cbp[h->mb.i_mb_xy - 1] >> 4)&0x3;
    }

    if( h->mb.i_mb_y > 0 )
    {
        cbp_b = (h->mb.cbp[h->mb.i_mb_xy - h->mb.i_mb_stride] >> 4)&0x3;
    }

    ctx = 0;
    if( cbp_a > 0 ) ctx++;
    if( cbp_b > 0 ) ctx += 2;
    if( h->mb.i_cbp_chroma == 0 )
    {
        x264_cabac_encode_decision( &h->cabac, 77 + ctx, 0 );
    }
    else
    {
        x264_cabac_encode_decision( &h->cabac, 77 + ctx, 1 );

        ctx = 4;
        if( cbp_a == 2 ) ctx++;
        if( cbp_b == 2 ) ctx += 2;
        x264_cabac_encode_decision( &h->cabac, 77 + ctx, h->mb.i_cbp_chroma > 1 ? 1 : 0 );
    }
}

/* TODO check it with != qp per mb */
static void x264_cabac_mb_qp_delta( x264_t *h )
{
    int i_mbn_xy = h->mb.i_mb_xy - 1;
    int i_dqp = h->mb.qp[h->mb.i_mb_xy] - h->mb.i_last_qp;
    int val = i_dqp <= 0 ? (-2*i_dqp) : (2*i_dqp - 1);
    int ctx;

    /* No need to test for PCM / SKIP */
    if( i_mbn_xy >= 0 && h->mb.i_last_dqp != 0 &&
        ( h->mb.type[i_mbn_xy] == I_16x16 || (h->mb.cbp[i_mbn_xy]&0x3f) ) )
        ctx = 1;
    else
        ctx = 0;

    while( val > 0 )
    {
        x264_cabac_encode_decision( &h->cabac,  60 + ctx, 1 );
        if( ctx < 2 )
            ctx = 2;
        else
            ctx = 3;
        val--;
    }
    x264_cabac_encode_decision( &h->cabac,  60 + ctx, 0 );
}

void x264_cabac_mb_skip( x264_t *h, int b_skip )
{
    int ctx = 0;

    if( h->mb.i_mb_x > 0 && !IS_SKIP( h->mb.type[h->mb.i_mb_xy -1]) )
    {
        ctx++;
    }
    if( h->mb.i_mb_y > 0 && !IS_SKIP( h->mb.type[h->mb.i_mb_xy -h->mb.i_mb_stride]) )
    {
        ctx++;
    }

    if( h->sh.i_type == SLICE_TYPE_P )
        x264_cabac_encode_decision( &h->cabac, 11 + ctx, b_skip ? 1 : 0 );
    else /* SLICE_TYPE_B */
        x264_cabac_encode_decision( &h->cabac, 24 + ctx, b_skip ? 1 : 0 );
}

static inline void x264_cabac_mb_sub_p_partition( x264_t *h, int i_sub )
{
    if( i_sub == D_L0_8x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 21, 1 );
    }
    else if( i_sub == D_L0_8x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 21, 0 );
        x264_cabac_encode_decision( &h->cabac, 22, 0 );
    }
    else if( i_sub == D_L0_4x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 21, 0 );
        x264_cabac_encode_decision( &h->cabac, 22, 1 );
        x264_cabac_encode_decision( &h->cabac, 23, 1 );
    }
    else if( i_sub == D_L0_4x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 21, 0 );
        x264_cabac_encode_decision( &h->cabac, 22, 1 );
        x264_cabac_encode_decision( &h->cabac, 23, 0 );
    }
}

static inline void x264_cabac_mb_sub_b_partition( x264_t *h, int i_sub )
{
    if( i_sub == D_DIRECT_8x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 0 );
    }
    else if( i_sub == D_L0_8x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_L1_8x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
    else if( i_sub == D_BI_8x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_L0_8x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
    else if( i_sub == D_L0_4x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_L1_8x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
    else if( i_sub == D_L1_4x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_BI_8x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
    else if( i_sub == D_BI_4x8 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_L0_4x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
    else if( i_sub == D_L1_4x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 0 );
    }
    else if( i_sub == D_BI_4x4 )
    {
        x264_cabac_encode_decision( &h->cabac, 36, 1 );
        x264_cabac_encode_decision( &h->cabac, 37, 1 );
        x264_cabac_encode_decision( &h->cabac, 38, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
        x264_cabac_encode_decision( &h->cabac, 39, 1 );
    }
}

static inline void x264_cabac_mb_ref( x264_t *h, int i_list, int idx )
{
    const int i8 = x264_scan8[idx];
    const int i_refa = h->mb.cache.ref[i_list][i8 - 1];
    const int i_refb = h->mb.cache.ref[i_list][i8 - 8];
    int i_ref  = h->mb.cache.ref[i_list][i8];
    int ctx  = 0;

    if( i_refa > 0 && !h->mb.cache.skip[i8 - 1])
        ctx++;
    if( i_refb > 0 && !h->mb.cache.skip[i8 - 8])
        ctx += 2;

    while( i_ref > 0 )
    {
        x264_cabac_encode_decision( &h->cabac, 54 + ctx, 1 );
        if( ctx < 4 )
            ctx = 4;
        else
            ctx = 5;

        i_ref--;
    }
    x264_cabac_encode_decision( &h->cabac, 54 + ctx, 0 );
}



static inline void  x264_cabac_mb_mvd_cpn( x264_t *h, int i_list, int idx, int l, int mvd )
{
    const int amvd = abs( h->mb.cache.mvd[i_list][x264_scan8[idx] - 1][l] ) +
                     abs( h->mb.cache.mvd[i_list][x264_scan8[idx] - 8][l] );
    const int i_abs = abs( mvd );
    const int i_prefix = X264_MIN( i_abs, 9 );
    const int ctxbase = (l == 0 ? 40 : 47);
    int ctx;
    int i;


    if( amvd < 3 )
        ctx = 0;
    else if( amvd > 32 )
        ctx = 2;
    else
        ctx = 1;

    for( i = 0; i < i_prefix; i++ )
    {
        x264_cabac_encode_decision( &h->cabac, ctxbase + ctx, 1 );
        if( ctx < 3 )
            ctx = 3;
        else if( ctx < 6 )
            ctx++;
    }
    if( i_prefix < 9 )
    {
        x264_cabac_encode_decision( &h->cabac, ctxbase + ctx, 0 );
    }

    if( i_prefix >= 9 )
    {
        int i_suffix = i_abs - 9;
        int k = 3;

        while( i_suffix >= (1<<k) )
        {
            x264_cabac_encode_bypass( &h->cabac, 1 );
            i_suffix -= 1 << k;
            k++;
        }
        x264_cabac_encode_bypass( &h->cabac, 0 );
        while( k-- )
        {
            x264_cabac_encode_bypass( &h->cabac, (i_suffix >> k)&0x01 );
        }
    }

    /* sign */
    if( mvd > 0 )
        x264_cabac_encode_bypass( &h->cabac, 0 );
    else if( mvd < 0 )
        x264_cabac_encode_bypass( &h->cabac, 1 );
}

static inline void  x264_cabac_mb_mvd( x264_t *h, int i_list, int idx, int width, int height )
{
    int mvp[2];
    int mdx, mdy;

    /* Calculate mvd */
    x264_mb_predict_mv( h, i_list, idx, width, mvp );
    mdx = h->mb.cache.mv[i_list][x264_scan8[idx]][0] - mvp[0];
    mdy = h->mb.cache.mv[i_list][x264_scan8[idx]][1] - mvp[1];

    /* encode */
    x264_cabac_mb_mvd_cpn( h, i_list, idx, 0, mdx );
    x264_cabac_mb_mvd_cpn( h, i_list, idx, 1, mdy );

    /* save value */
    x264_macroblock_cache_mvd( h, block_idx_x[idx], block_idx_y[idx], width, height, i_list, mdx, mdy );
}

static inline void x264_cabac_mb8x8_mvd( x264_t *h, int i_list )
{
    int i;
    for( i = 0; i < 4; i++ )
    {
        if( !x264_mb_partition_listX_table[i_list][ h->mb.i_sub_partition[i] ] )
        {
            continue;
        }

        switch( h->mb.i_sub_partition[i] )
        {
            case D_L0_8x8:
            case D_L1_8x8:
            case D_BI_8x8:
                x264_cabac_mb_mvd( h, i_list, 4*i, 2, 2 );
                break;
            case D_L0_8x4:
            case D_L1_8x4:
            case D_BI_8x4:
                x264_cabac_mb_mvd( h, i_list, 4*i+0, 2, 1 );
                x264_cabac_mb_mvd( h, i_list, 4*i+2, 2, 1 );
                break;
            case D_L0_4x8:
            case D_L1_4x8:
            case D_BI_4x8:
                x264_cabac_mb_mvd( h, i_list, 4*i+0, 1, 2 );
                x264_cabac_mb_mvd( h, i_list, 4*i+1, 1, 2 );
                break;
            case D_L0_4x4:
            case D_L1_4x4:
            case D_BI_4x4:
                x264_cabac_mb_mvd( h, i_list, 4*i+0, 1, 1 );
                x264_cabac_mb_mvd( h, i_list, 4*i+1, 1, 1 );
                x264_cabac_mb_mvd( h, i_list, 4*i+2, 1, 1 );
                x264_cabac_mb_mvd( h, i_list, 4*i+3, 1, 1 );
                break;
        }
    }
}

static int x264_cabac_mb_cbf_ctxidxinc( x264_t *h, int i_cat, int i_idx )
{
    /* TODO: clean up/optimize */
    int i_mba_xy = -1;
    int i_mbb_xy = -1;
    int i_nza = -1;
    int i_nzb = -1;
    int ctx = 0;

    if( i_cat == 0 )
    {
        if( h->mb.i_mb_x > 0 )
        {
            i_mba_xy = h->mb.i_mb_xy -1;
            if( h->mb.type[i_mba_xy] == I_16x16 )
            {
                i_nza = h->mb.cbp[i_mba_xy]&0x100;
            }
        }
        if( h->mb.i_mb_y > 0 )
        {
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;
            if( h->mb.type[i_mbb_xy] == I_16x16 )
            {
                i_nzb = h->mb.cbp[i_mbb_xy]&0x100;
            }
        }
    }
    else if( i_cat == 1 || i_cat == 2 )
    {
        int x = block_idx_x[i_idx];
        int y = block_idx_y[i_idx];

        if( x > 0 )
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_x > 0 )
            i_mba_xy = h->mb.i_mb_xy -1;

        if( y > 0 )
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_y > 0 )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;

        /* no need to test for skip/pcm */
        if( i_mba_xy >= 0 )
        {
            const int i8x8a = block_idx_xy[(x-1)&0x03][y]/4;
            if( (h->mb.cbp[i_mba_xy]&0x0f)>> i8x8a )
            {
                i_nza = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 1];
            }
        }
        if( i_mbb_xy >= 0 )
        {
            const int i8x8b = block_idx_xy[x][(y-1)&0x03]/4;
            if( (h->mb.cbp[i_mbb_xy]&0x0f)>> i8x8b )
            {
                i_nzb = h->mb.cache.non_zero_count[x264_scan8[i_idx] - 8];
            }
        }
    }
    else if( i_cat == 3 )
    {
        /* no need to test skip/pcm */
        if( h->mb.i_mb_x > 0 )
        {
            i_mba_xy = h->mb.i_mb_xy -1;
            if( h->mb.cbp[i_mba_xy]&0x30 )
            {
                i_nza = h->mb.cbp[i_mba_xy]&( 0x02 << ( 8 + i_idx) );
            }
        }
        if( h->mb.i_mb_y > 0 )
        {
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;
            if( h->mb.cbp[i_mbb_xy]&0x30 )
            {
                i_nzb = h->mb.cbp[i_mbb_xy]&( 0x02 << ( 8 + i_idx) );
            }
        }
    }
    else if( i_cat == 4 )
    {
        int idxc = i_idx% 4;

        if( idxc == 1 || idxc == 3 )
            i_mba_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_x > 0 )
            i_mba_xy = h->mb.i_mb_xy - 1;

        if( idxc == 2 || idxc == 3 )
            i_mbb_xy = h->mb.i_mb_xy;
        else if( h->mb.i_mb_y > 0 )
            i_mbb_xy = h->mb.i_mb_xy - h->mb.i_mb_stride;

        /* no need to test skip/pcm */
        if( i_mba_xy >= 0 && (h->mb.cbp[i_mba_xy]&0x30) == 0x20 )
        {
            i_nza = h->mb.cache.non_zero_count[x264_scan8[16+i_idx] - 1];
        }
        if( i_mbb_xy >= 0 && (h->mb.cbp[i_mbb_xy]&0x30) == 0x20 )
        {
            i_nzb = h->mb.cache.non_zero_count[x264_scan8[16+i_idx] - 8];
        }
    }

    if( ( i_mba_xy < 0  && IS_INTRA( h->mb.i_type ) ) || i_nza > 0 )
    {
        ctx++;
    }
    if( ( i_mbb_xy < 0  && IS_INTRA( h->mb.i_type ) ) || i_nzb > 0 )
    {
        ctx += 2;
    }

    return 4 * i_cat + ctx;
}


static void block_residual_write_cabac( x264_t *h, int i_ctxBlockCat, int i_idx, int *l, int i_count )
{
    static const int significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int last_significant_coeff_flag_offset[5] = { 0, 15, 29, 44, 47 };
    static const int coeff_abs_level_m1_offset[5] = { 0, 10, 20, 30, 39 };

    int i_coeff_abs_m1[16];
    int i_coeff_sign[16];
    int i_coeff = 0;
    int i_last  = 0;

    int i_abslevel1 = 0;
    int i_abslevelgt1 = 0;

    int i;

    /* i_ctxBlockCat: 0-> DC 16x16  i_idx = 0
     *                1-> AC 16x16  i_idx = luma4x4idx
     *                2-> Luma4x4   i_idx = luma4x4idx
     *                3-> DC Chroma i_idx = iCbCr
     *                4-> AC Chroma i_idx = 4 * iCbCr + chroma4x4idx
     */

    //fprintf( stderr, "l[] = " );
    for( i = 0; i < i_count; i++ )
    {
        //fprintf( stderr, "%d ", l[i] );
        if( l[i] != 0 )
        {
            i_coeff_abs_m1[i_coeff] = abs( l[i] ) - 1;
            i_coeff_sign[i_coeff]   = ( l[i] < 0 ? 1 : 0);
            i_coeff++;

            i_last = i;
        }
    }
    //fprintf( stderr, "\n" );

    if( i_coeff == 0 )
    {
        /* codec block flag */
        x264_cabac_encode_decision( &h->cabac,  85 + x264_cabac_mb_cbf_ctxidxinc( h, i_ctxBlockCat, i_idx ), 0 );
        return;
    }

    /* block coded */
    x264_cabac_encode_decision( &h->cabac,  85 + x264_cabac_mb_cbf_ctxidxinc( h, i_ctxBlockCat, i_idx ), 1 );
    for( i = 0; i < i_count - 1; i++ )
    {
        int i_ctxIdxInc;

        i_ctxIdxInc = X264_MIN( i, i_count - 2 );

        if( l[i] != 0 )
        {
            x264_cabac_encode_decision( &h->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 1 );
            x264_cabac_encode_decision( &h->cabac, 166 + last_significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, i == i_last ? 1 : 0 );
        }
        else
        {
            x264_cabac_encode_decision( &h->cabac, 105 + significant_coeff_flag_offset[i_ctxBlockCat] + i_ctxIdxInc, 0 );
        }
        if( i == i_last )
        {
            break;
        }
    }

    for( i = i_coeff - 1; i >= 0; i-- )
    {
        int i_prefix;
        int i_ctxIdxInc;

        /* write coeff_abs - 1 */

        /* prefix */
        i_prefix = X264_MIN( i_coeff_abs_m1[i], 14 );

        i_ctxIdxInc = (i_abslevelgt1 != 0 ? 0 : X264_MIN( 4, i_abslevel1 + 1 )) + coeff_abs_level_m1_offset[i_ctxBlockCat];
        if( i_prefix == 0 )
        {
            x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 0 );
        }
        else
        {
            int j;
            x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 1 );
            i_ctxIdxInc = 5 + X264_MIN( 4, i_abslevelgt1 ) + coeff_abs_level_m1_offset[i_ctxBlockCat];
            for( j = 0; j < i_prefix - 1; j++ )
            {
                x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 1 );
            }
            if( i_prefix < 14 )
            {
                x264_cabac_encode_decision( &h->cabac,  227 + i_ctxIdxInc, 0 );
            }
        }
        /* suffix */
        if( i_coeff_abs_m1[i] >= 14 )
        {
            int k = 0;
            int i_suffix = i_coeff_abs_m1[i] - 14;

            while( i_suffix >= (1<<k) )
            {
                x264_cabac_encode_bypass( &h->cabac, 1 );
                i_suffix -= 1 << k;
                k++;
            }
            x264_cabac_encode_bypass( &h->cabac, 0 );
            while( k-- )
            {
                x264_cabac_encode_bypass( &h->cabac, (i_suffix >> k)&0x01 );
            }
        }

        /* write sign */
        x264_cabac_encode_bypass( &h->cabac, i_coeff_sign[i] );


        if( i_coeff_abs_m1[i] == 0 )
        {
            i_abslevel1++;
        }
        else
        {
            i_abslevelgt1++;
        }
    }
}



void x264_macroblock_write_cabac( x264_t *h, bs_t *s )
{
    const int i_mb_type = h->mb.i_type;
    const int i_mb_pos_start = bs_pos( s );
    int       i_mb_pos_tex;

    int i_list;
    int i;

    /* Write the MB type */
    x264_cabac_mb_type( h );

    /* PCM special block type UNTESTED */
    if( i_mb_type == I_PCM )
    {
        bs_align_0( s );    /* not sure */
        /* Luma */
        for( i = 0; i < 16*16; i++ )
        {
            const int x = 16 * h->mb.i_mb_x + (i % 16);
            const int y = 16 * h->mb.i_mb_y + (i / 16);
            bs_write( s, 8, h->fenc->plane[0][y*h->mb.pic.i_stride[0]+x] );
        }
        /* Cb */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[1][y*h->mb.pic.i_stride[1]+x] );
        }
        /* Cr */
        for( i = 0; i < 8*8; i++ )
        {
            const int x = 8 * h->mb.i_mb_x + (i % 8);
            const int y = 8 * h->mb.i_mb_y + (i / 8);
            bs_write( s, 8, h->fenc->plane[2][y*h->mb.pic.i_stride[2]+x] );
        }
        x264_cabac_encode_init( &h->cabac, s );
        return;
    }

    if( IS_INTRA( i_mb_type ) )
    {
        /* Prediction */
        if( i_mb_type == I_4x4 )
        {
            for( i = 0; i < 16; i++ )
            {
                const int i_pred = x264_mb_predict_intra4x4_mode( h, i );
                const int i_mode = h->mb.cache.intra4x4_pred_mode[x264_scan8[i]];
                x264_cabac_mb_intra4x4_pred_mode( h, i_pred, i_mode );
            }
        }
        x264_cabac_mb_intra8x8_pred_mode( h );
    }
    else if( i_mb_type == P_L0 )
    {
        if( h->mb.i_partition == D_16x16 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, 0, 0 );
            }
            x264_cabac_mb_mvd( h, 0, 0, 4, 4 );
        }
        else if( h->mb.i_partition == D_16x8 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, 0, 0 );
                x264_cabac_mb_ref( h, 0, 8 );
            }
            x264_cabac_mb_mvd( h, 0, 0, 4, 2 );
            x264_cabac_mb_mvd( h, 0, 8, 4, 2 );
        }
        else if( h->mb.i_partition == D_8x16 )
        {
            if( h->sh.i_num_ref_idx_l0_active > 1 )
            {
                x264_cabac_mb_ref( h, 0, 0 );
                x264_cabac_mb_ref( h, 0, 4 );
            }
            x264_cabac_mb_mvd( h, 0, 0, 2, 4 );
            x264_cabac_mb_mvd( h, 0, 4, 2, 4 );
        }
    }
    else if( i_mb_type == P_8x8 )
    {
        /* sub mb type */
        x264_cabac_mb_sub_p_partition( h, h->mb.i_sub_partition[0] );
        x264_cabac_mb_sub_p_partition( h, h->mb.i_sub_partition[1] );
        x264_cabac_mb_sub_p_partition( h, h->mb.i_sub_partition[2] );
        x264_cabac_mb_sub_p_partition( h, h->mb.i_sub_partition[3] );

        /* ref 0 */
        if( h->sh.i_num_ref_idx_l0_active > 1 )
        {
            x264_cabac_mb_ref( h, 0, 0 );
            x264_cabac_mb_ref( h, 0, 4 );
            x264_cabac_mb_ref( h, 0, 8 );
            x264_cabac_mb_ref( h, 0, 12 );
        }

        x264_cabac_mb8x8_mvd( h, 0 );
    }
    else if( i_mb_type == B_8x8 )
    {
        /* sub mb type */
        x264_cabac_mb_sub_b_partition( h, h->mb.i_sub_partition[0] );
        x264_cabac_mb_sub_b_partition( h, h->mb.i_sub_partition[1] );
        x264_cabac_mb_sub_b_partition( h, h->mb.i_sub_partition[2] );
        x264_cabac_mb_sub_b_partition( h, h->mb.i_sub_partition[3] );

        /* ref */
        for( i_list = 0; i_list < 2; i_list++ )
        {
            if( ( i_list ? h->sh.i_num_ref_idx_l1_active : h->sh.i_num_ref_idx_l0_active ) == 1 )
                continue;
            for( i = 0; i < 4; i++ )
            {
                if( x264_mb_partition_listX_table[i_list][ h->mb.i_sub_partition[i] ] )
                {
                    x264_cabac_mb_ref( h, i_list, 4*i );
                }
            }
        }

        x264_cabac_mb8x8_mvd( h, 0 );
        x264_cabac_mb8x8_mvd( h, 1 );
    }
    else if( i_mb_type != B_DIRECT )
    {
        /* All B mode */
        int b_list[2][2];

        /* init ref list utilisations */
        for( i = 0; i < 2; i++ )
        {
            b_list[0][i] = x264_mb_type_list0_table[i_mb_type][i];
            b_list[1][i] = x264_mb_type_list1_table[i_mb_type][i];
        }

        for( i_list = 0; i_list < 2; i_list++ )
        {
            const int i_ref_max = i_list == 0 ? h->sh.i_num_ref_idx_l0_active : h->sh.i_num_ref_idx_l1_active;

            if( i_ref_max > 1 )
            {
                if( h->mb.i_partition == D_16x16 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, i_list, 0 );
                }
                else if( h->mb.i_partition == D_16x8 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, i_list, 0 );
                    if( b_list[i_list][1] ) x264_cabac_mb_ref( h, i_list, 8 );
                }
                else if( h->mb.i_partition == D_8x16 )
                {
                    if( b_list[i_list][0] ) x264_cabac_mb_ref( h, i_list, 0 );
                    if( b_list[i_list][1] ) x264_cabac_mb_ref( h, i_list, 4 );
                }
            }
        }
        for( i_list = 0; i_list < 2; i_list++ )
        {
            if( h->mb.i_partition == D_16x16 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, i_list, 0, 4, 4 );
            }
            else if( h->mb.i_partition == D_16x8 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, i_list, 0, 4, 2 );
                if( b_list[i_list][1] ) x264_cabac_mb_mvd( h, i_list, 8, 4, 2 );
            }
            else if( h->mb.i_partition == D_8x16 )
            {
                if( b_list[i_list][0] ) x264_cabac_mb_mvd( h, i_list, 0, 2, 4 );
                if( b_list[i_list][1] ) x264_cabac_mb_mvd( h, i_list, 4, 2, 4 );
            }
        }
    }

    i_mb_pos_tex = bs_pos( s );
    h->stat.frame.i_hdr_bits += i_mb_pos_tex - i_mb_pos_start;

    if( i_mb_type != I_16x16 )
    {
        x264_cabac_mb_cbp_luma( h );
        x264_cabac_mb_cbp_chroma( h );
    }

    if( h->mb.i_cbp_luma > 0 || h->mb.i_cbp_chroma > 0 || i_mb_type == I_16x16 )
    {
        x264_cabac_mb_qp_delta( h );

        /* write residual */
        if( i_mb_type == I_16x16 )
        {
            /* DC Luma */
            block_residual_write_cabac( h, 0, 0, h->dct.luma16x16_dc, 16 );

            if( h->mb.i_cbp_luma != 0 )
            {
                /* AC Luma */
                for( i = 0; i < 16; i++ )
                {
                    block_residual_write_cabac( h, 1, i, h->dct.block[i].residual_ac, 15 );
                }
            }
        }
        else
        {
            for( i = 0; i < 16; i++ )
            {
                if( h->mb.i_cbp_luma & ( 1 << ( i / 4 ) ) )
                {
                    block_residual_write_cabac( h, 2, i, h->dct.block[i].luma4x4, 16 );
                }
            }
        }

        if( h->mb.i_cbp_chroma &0x03 )    /* Chroma DC residual present */
        {
            block_residual_write_cabac( h, 3, 0, h->dct.chroma_dc[0], 4 );
            block_residual_write_cabac( h, 3, 1, h->dct.chroma_dc[1], 4 );
        }
        if( h->mb.i_cbp_chroma&0x02 ) /* Chroma AC residual present */
        {
            for( i = 0; i < 8; i++ )
            {
                block_residual_write_cabac( h, 4, i, h->dct.block[16+i].residual_ac, 15 );
            }
        }
    }

    if( IS_INTRA( i_mb_type ) )
        h->stat.frame.i_itex_bits += bs_pos(s) - i_mb_pos_tex;
    else
        h->stat.frame.i_ptex_bits += bs_pos(s) - i_mb_pos_tex;
}

