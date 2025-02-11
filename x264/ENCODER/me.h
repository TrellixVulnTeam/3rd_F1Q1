/*****************************************************************************
 * me.h: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: me.h,v 1.1.1.1 2005/03/16 13:27:22 264 Exp $
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

#ifndef _ME_H
#define _ME_H 1

#define COST_MAX (1<<28)

typedef struct
{
    /* input */
    int      i_pixel;   /* PIXEL_WxH */
    int16_t *p_cost_mv; /* lambda * nbits for each possible mv */

    uint8_t *p_fref[6];
    uint8_t *p_fenc[3];
    int      i_stride[2];

    int mvp[2];

    /* output */
    int cost_mv;        /* lambda * nbits for the chosen mv */
    int cost;           /* satd + lambda * nbits */
    int mv[2];
} x264_me_t;

void x264_me_search_ref( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc, int *p_fullpel_thresh );
static inline void x264_me_search( x264_t *h, x264_me_t *m, int (*mvc)[2], int i_mvc )
    { x264_me_search_ref( h, m, mvc, i_mvc, NULL ); }

void x264_me_refine_qpel( x264_t *h, x264_me_t *m );

#endif
