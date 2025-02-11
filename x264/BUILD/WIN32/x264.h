#ifndef _X264_H
#define _X264_H 1

#include <stdarg.h>

#define X264_BUILD 1

/* x264_t:
 *      opaque handler for decoder and encoder */
typedef struct x264_t x264_t;

/****************************************************************************
 * Initialisation structure and function.
 ****************************************************************************/
/* CPU flags
 */
#define X264_CPU_MMX        0x000001    /* mmx */
#define X264_CPU_MMXEXT     0x000002    /* mmx-ext*/
#define X264_CPU_SSE        0x000004    /* sse */
#define X264_CPU_SSE2       0x000008    /* sse 2 */
#define X264_CPU_3DNOW      0x000010    /* 3dnow! */
#define X264_CPU_3DNOWEXT   0x000020    /* 3dnow! ext */
#define X264_CPU_ALTIVEC    0x000040    /* altivec */

/* Analyse flags
 */
#define X264_ANALYSE_I4x4       0x0001  /* Analyse i4x4 */
#define X264_ANALYSE_PSUB16x16  0x0010  /* Analyse p16x8, p8x16 and p8x8 */
#define X264_ANALYSE_PSUB8x8    0x0020  /* Analyse p8x4, p4x8, p4x4 */
#define X264_ANALYSE_BSUB16x16  0x0100  /* Analyse b16x8, b8x16 and b8x8 */
#define X264_DIRECT_PRED_NONE        0
#define X264_DIRECT_PRED_SPATIAL     1
#define X264_DIRECT_PRED_TEMPORAL    2

/* Colorspace type
 */
#define X264_CSP_MASK           0x00ff  /* */
#define X264_CSP_NONE           0x0000  /* Invalid mode     */
#define X264_CSP_I420           0x0001  /* yuv 4:2:0 planar */
#define X264_CSP_I422           0x0002  /* yuv 4:2:2 planar */
#define X264_CSP_I444           0x0003  /* yuv 4:4:4 planar */
#define X264_CSP_YV12           0x0004  /* yuv 4:2:0 planar */
#define X264_CSP_YUYV           0x0005  /* yuv 4:2:2 packed */
#define X264_CSP_RGB            0x0006  /* rgb 24bits       */
#define X264_CSP_BGR            0x0007  /* bgr 24bits       */
#define X264_CSP_BGRA           0x0008  /* bgr 32bits       */
#define X264_CSP_VFLIP          0x1000  /* */

/* Slice type
 */
#define X264_TYPE_AUTO          0x0000  /* Let x264 choose the right type */
#define X264_TYPE_IDR           0x0001
#define X264_TYPE_I             0x0002
#define X264_TYPE_P             0x0003
#define X264_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define X264_TYPE_B             0x0005
#define IS_X264_TYPE_I(x) ((x)==X264_TYPE_I || (x)==X264_TYPE_IDR)
#define IS_X264_TYPE_B(x) ((x)==X264_TYPE_B || (x)==X264_TYPE_BREF)

/* Log level
 */
#define X264_LOG_NONE          (-1)
#define X264_LOG_ERROR          0
#define X264_LOG_WARNING        1
#define X264_LOG_INFO           2
#define X264_LOG_DEBUG          3

typedef struct
{
    /* CPU flags */
    unsigned int cpu;

    /* Video Properties */
    int         i_width;
    int         i_height;
    int         i_csp;  /* CSP of encoded bitstream, only i420 supported */
    int         i_level_idc; 

    struct
    {
        /* they will be reduced to be 0 < x <= 65535 and prime */
        int         i_sar_height;
        int         i_sar_width;
    } vui;

    int         i_fps_num;
    int         i_fps_den;
    int         i_maxframes;        /* Maximum number of frames to read from input file and encode, 0=unlimited */

    /* Bitstream parameters */
    int         i_frame_reference;  /* Maximum number of reference frames */
    int         i_keyint_max;       /* Force an IDR keyframe at this interval */
    int         i_keyint_min;       /* Scenecuts closer together than this are coded as I, not IDR. */
    int         i_scenecut_threshold; /* how aggressively to insert extra I frames */
    int         i_bframe;   /* how many b-frame between 2 references pictures */
    int         b_bframe_adaptive;
    int         i_bframe_bias;
    int         b_bframe_pyramid;   /* Keep some B-frames as references */

    int         b_deblocking_filter;
    int         i_deblocking_filter_alphac0;    /* [-6, 6] -6 light filter, 6 strong */
    int         i_deblocking_filter_beta;       /* [-6, 6]  idem */

    int         b_cabac;
    int         i_cabac_init_idc;


    /* Log */
    void        (*pf_log)( void *, int i_level, const char *psz, va_list );
    void        *p_log_private;
    int         i_log_level;

    /* Encoder analyser parameters */
    struct
    {
        unsigned int intra;     /* intra flags */
        unsigned int inter;     /* inter flags */

        int          i_direct_mv_pred; /* spatial vs temporal mv prediction */
        int          i_subpel_refine; /* subpixel motion estimation quality */
        int          b_chroma_me; /* chroma ME for subpel and mode decision in P-frames */
        int          i_mv_range; /* maximum length of a mv (in pixels) */

        int          b_weighted_bipred; /* implicit weighting for B-frames */

        int          i_chroma_qp_offset;

        int          b_psnr;    /* Do we compute PSNR stats (save a few % of cpu) */
    } analyse;

    /* Rate control parameters */
    struct
    {
        int         i_qp_constant;  /* 1-51 */
        int         i_qp_min;       /* min allowed QP value */
        int         i_qp_max;       /* max allowed QP value */
        int         i_qp_step;      /* max QP step between frames */

        int         b_cbr;          /* constant bitrate */
        int         i_bitrate;
        int         i_rc_buffer_size;
        int         i_rc_init_buffer;
        int         i_rc_sens;      /* rate control sensitivity */
        float       f_ip_factor;
        float       f_pb_factor;

        /* 2pass */
        int         b_stat_write;   /* Enable stat writing in psz_stat_out */
        char        *psz_stat_out;
        int         b_stat_read;    /* Read stat from psz_stat_in and use it */
        char        *psz_stat_in;

        /* 2pass params (same than ffmpeg ones) */
        char        *psz_rc_eq;     /* 2 pass rate control equation */
        float       f_qcompress;    /* 0.0 => cbr, 1.0 => constant qp */
        float       f_qblur;        /* temporally blur quants */
        float       f_complexity_blur; /* temporally blur complexity */
    } rc;

} x264_param_t;

/* x264_param_default:
 *      fill x264_param_t with default values and do CPU detection */
void    x264_param_default( x264_param_t * );

/****************************************************************************
 * Picture structures and functions.
 ****************************************************************************/
typedef struct
{
    int     i_csp;

    int     i_plane;
    int     i_stride[4];
    uint8_t *plane[4];
} x264_image_t;

typedef struct
{
    /* In: force picture type (if not auto) XXX: ignored for now
     * Out: type of the picture encoded */
    int     i_type;
    /* In: force quantizer for > 0 */
    int     i_qpplus1;
    /* In: user pts, Out: pts of encoded picture (user)*/
    int64_t i_pts;

    /* In: raw data */
    x264_image_t img;
} x264_picture_t;

/* x264_picture_alloc:
 *  alloc data for a picture. You must call x264_picture_clean on it. */
void x264_picture_alloc( x264_picture_t *pic, int i_csp, int i_width, int i_height );

/* x264_picture_clean:
 *  free associated resource for a x264_picture_t allocated with
 *  x264_picture_alloc ONLY */
void x264_picture_clean( x264_picture_t *pic );

/****************************************************************************
 * NAL structure and functions:
 ****************************************************************************/
/* nal */
enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,
    NAL_SLICE   = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8
    /* ref_idc == 0 for 6,9,10,11,12 */
};
enum nal_priority_e
{
    NAL_PRIORITY_DISPOSABLE = 0,
    NAL_PRIORITY_LOW        = 1,
    NAL_PRIORITY_HIGH       = 2,
    NAL_PRIORITY_HIGHEST    = 3,
};

typedef struct
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */

    /* This data are raw payload */
    int     i_payload;
    uint8_t *p_payload;
} x264_nal_t;

/* x264_nal_encode:
 *      encode a nal into a buffer, setting the size.
 *      if b_annexeb then a long synch work is added
 *      XXX: it currently doesn't check for overflow */
int x264_nal_encode( void *, int *, int b_annexeb, x264_nal_t *nal );

/* x264_nal_decode:
 *      decode a buffer nal into a x264_nal_t */
int x264_nal_decode( x264_nal_t *nal, void *, int );

/****************************************************************************
 * Encoder functions:
 ****************************************************************************/

/* x264_encoder_open:
 *      create a new encoder handler, all parameters from x264_param_t are copied */
void *x264_encoder_open   ();
int  SetFrameInfoInitial(int aiFrameWidth,int aiFrameHeight,int aiBitCount,void *h);
/* x264_encoder_headers:
 *      return the SPS and PPS that will be used for the whole stream */
//int     x264_encoder_headers( x264_t *, x264_nal_t **, int * );


/* x264_encoder_encode:
 *      encode one picture */
int  EnCodec(void *h,char *apSrcData,int aiSrcDataLen,char *apDstBuff,int *aiDstBuffLen,int *abMark);

/* x264_encoder_close:
 *      close an encoder handler */
void    x264_encoder_close  ( void * );

/* XXX: decoder isn't working so no need to export it */

/****************************************************************************
 * Decoder functions:
 ****************************************************************************
 /* x264_decoder_open:
 */
void *x264_decoder_open   ();
/* x264_decoder_decode:
 */
int     x264_decoder_decode ( x264_t *, x264_picture_t **, x264_nal_t * );
/* x264_decoder_close:
 */
void    x264_decoder_close  ( void * );


/****************************************************************************
 * Private stuff for internal usage:
 ****************************************************************************/
#ifdef __X264__
#   ifdef _MSC_VER
#       define inline __inline
#       define DECLARE_ALIGNED( type, var, n ) __declspec(align(n)) type var
#   else
#       define DECLARE_ALIGNED( type, var, n ) type var __attribute__((aligned(n)))
#   endif
#endif

#endif
