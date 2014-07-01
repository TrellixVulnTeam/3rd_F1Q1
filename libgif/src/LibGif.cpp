// LibGif.cpp : Defines the entry point for the DLL application.
//

#include "LibGif.h"
#include "GifEnc.h"

#define USE_DIFFPROC
#define USE_DRAWCURSOR

//#define DEBUG_OUT
#ifdef  DEBUG_OUT
	HFILE hRgbFile = NULL;
#endif 

void ConstructBitmap(BITMAP* b, int w, int h, int bits, void* buf)
{
	b->bmType	= 'BM';
	b->bmBitsPixel	= bits;
	b->bmWidth	= w;
	b->bmHeight	= h;
	b->bmWidthBytes = w * (bits/8);

	b->bmPlanes	= 1;
	b->bmBits	= buf;

	b->bmWidthBytes = FOUR_ALIGN(b->bmWidthBytes);
	/*if(b->bmWidthBytes%4)
		b->bmWidthBytes += 4 - (b->bmWidthBytes%4);*/
}

//�õ���ͬ����
RECT DiffRect(const BITMAP* b1, const BITMAP* b2)
{		
	int w = b1->bmWidth;
	int h = b2->bmHeight;
	int wb1 = b1->bmWidthBytes;
	int wb2 = b2->bmWidthBytes;
	
	int dx[2] = { w, 0 };
	int dy[2] = { h, 0 };
	const BYTE* bits1 = (const BYTE*)b1->bmBits;
	const BYTE* bits2 = (const BYTE*)b2->bmBits;

	for (int y = 0; y < h; y ++) 
		{
		const BYTE* line1 = bits1 + wb1 * y;
		const BYTE* line2 = bits2 + wb2 * y;
		
	
		for (int x = 0; x < w; x ++) 
			{
			UINT nPix1 =0 ,nPix2 = 0;

			memcpy(&nPix1,line1 + x*3,3);
			memcpy(&nPix2,line2 + x*3,3);

			if (nPix1 != nPix2) 
				{
				break;
				}
			}
		
		if (x >= w) 
			{
			continue;
			}
		
		
		for (int r = w; r > x + 1; r --) 
			{	
			UINT nPix1 =0,nPix2 = 0;
			
			memcpy(&nPix1,line1 + (r-1)*3,3);
			memcpy(&nPix2,line2 + (r-1)*3,3);
			
			if (nPix1 != nPix2) 
				{
				break;
				}		
			}
		
		dx[0] = min(dx[0], x);
		dx[1] = max(dx[1], r);
		dy[0] = min(dy[0], y);
		dy[1] = max(dy[1], y);
		}
	
	dx[0] = min(dx[0], dx[1]);
	dy[0] = min(dy[0], dy[1]);
	
	RECT rt = { dx[0], dy[0], dx[1], dy[1] };
	
	return rt;
}

//�õ���ɫ������
INT GetPalette(LPBYTE ppPalette,LPBYTE ppData,LPBYTE lpColorInx,BYTE nBitsPixel,int nWidth,int nHeight)
{
	int nOffset,nIndex=0;
	BYTE r,g,b;
	UINT nColorIndex;
	BYTE* buf = ppData;
	CQuantizer quan((1<<COLORS_BITS)-1,COLORS_BITS);
	int nLnBytes;
	int nColorCount = 0;
	RGBQUAD *pQuad;

	quan.ProcessImage(ppData,nWidth,nHeight);
	nColorCount	= quan.GetColorCount();
	pQuad		= new RGBQUAD[nColorCount];
	quan.SetColorTable(pQuad);

	//ÿ��������4�ֽڵ�������
	nLnBytes = ((nWidth*nBitsPixel*8+31)/32)*4;

	for(int  i=0; i<nHeight; i++)
		{
		nOffset = (i)*nLnBytes;
			
		for(int j=0; j<nWidth; j++)
			{
			b = buf[nOffset+j*3];
			g = buf[nOffset+j*3+1];
			r = buf[nOffset+j*3+2];

			quan.GetColorIndex(r,g,b,&nColorIndex);
			lpColorInx[nIndex++] = (BYTE)nColorIndex;		
			}
		}


	memset(ppPalette,0,256*3);	
	for(i=0; i<(int)nColorCount; i++)
		{
		ppPalette[i*3] = pQuad[i].rgbRed;
		ppPalette[i*3+1] = pQuad[i].rgbGreen;
		ppPalette[i*3+2] = pQuad[i].rgbBlue;
		}

	ppPalette[nColorCount*3]   = 254;
	ppPalette[nColorCount*3+1] = 254;
	ppPalette[nColorCount*3+2] = 254;

	delete []pQuad;

	return nColorCount;
}

//����ͼƬ
void InvertedLine(LPVOID lpRgbBuf,int nWidth,int nHeight)
{	
	LPBYTE	lpLineBuf	= NULL;
	LPBYTE	lpBufPos	= (LPBYTE)lpRgbBuf;
	int	nLineSize	= 0;	
	int	i,nLineNum	= nHeight/2;
	
	nLineSize = nWidth*3;	
	if(nLineSize%4)
		nLineSize += 4- (nLineSize%4);

	lpLineBuf = (LPBYTE)LocalAlloc(LPTR,nLineSize);	
	for(i=0;i<nLineNum;i++)
		{
		CopyMemory(lpLineBuf,lpBufPos+i*nLineSize,nLineSize);	
		CopyMemory(lpBufPos+i*nLineSize,lpBufPos+(nHeight-i-1)*nLineSize,nLineSize);	
		CopyMemory(lpBufPos+(nHeight-i-1)*nLineSize,lpLineBuf,nLineSize);	
		}
	
	
	LocalFree(lpLineBuf);
	
	return ;
}

// typedef short int code_int;
// HFILE		 g_hOutFile;
// int		 g_init_bits;
// unsigned long    g_nAccum;
// int              g_nCurbits;
// code_int	 g_nMaxcode;
// code_int	 g_nDictionaryPos;
// int		 g_nTransInx;	
// int		 g_nclear_flg;
// int		 g_nClearCode;
// int		 g_nEOFCode;
// int		 g_nEncCount;
// int		 g_nTotalCount;
// int		 g_nbits;		
// long		 g_htab[HSIZE];
// unsigned short   g_codetab[HSIZE];	//
// char		 g_szAccum[256];	//256
// 
// 
// void clear_hash(long hsize)
// {
// 	memset(g_htab,0xff,sizeof(g_htab));
// 	memset(g_codetab,0,sizeof(g_codetab));
// }
// 
// static const unsigned long code_mask[] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
// 					   0x001F, 0x003F, 0x007F, 0x00FF,
// 					   0x01FF, 0x03FF, 0x07FF, 0x0FFF,
// 					   0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };
// 
// void flush_char()
// {
// 	if (g_nEncCount > 0) 
// 		{
// 		_lwrite(g_hOutFile,(LPCSTR)&g_nEncCount,1);
// 		_lwrite(g_hOutFile,(LPCSTR)g_szAccum,g_nEncCount);
// 		
// 		g_nTotalCount += g_nEncCount;
// 		g_nEncCount=0;
// 		}
// }
// 
// //�ַ����ۼ����
// void char_out(int c)
// {
// 	g_szAccum[g_nEncCount++]=(char)c;
// 
// 	if(g_nEncCount >= OUT_MAX_LEN )
// 		{
// 		flush_char();
// 		}
// }
// 
// //���ѹ����
// void output(code_int code)
// {
// 
// 
// 	g_nAccum &= code_mask[g_nCurbits];	
// 	if( g_nCurbits > 0 )
// 		{
// 		g_nAccum |= ((long)code << g_nCurbits);
// 		}
// 	else	{
// 		g_nAccum = code;
// 	
// 		}
// 	
// 
// 	g_nCurbits += g_nbits;	
// 
// 
// 	while( g_nCurbits >= 8 ) 
// 		{
// 		char_out( (unsigned int)(g_nAccum & 0xff) );
// 		g_nAccum >>= 8;
// 		g_nCurbits -= 8;
// 		}
// 	
// 	/*
// 	* If the next entry is going to be too big for the code size,
// 	* then increase it, if possible.
// 	*/
// 	
// 	if ( g_nDictionaryPos > g_nMaxcode || g_nclear_flg ) 
// 		{
// 		if( g_nclear_flg ) 
// 			{
// 			g_nMaxcode = (short)MAXCODE(g_nbits = g_init_bits);
// 			g_nclear_flg = 0;
// 			} 
// 		else	{
// 			g_nbits ++;
// 			if ( g_nbits == MAXBITSCODES )
// 				g_nMaxcode = (code_int)1 << MAXBITSCODES;		/* should NEVER generate this code */
// 			else
// 				g_nMaxcode = (short)MAXCODE(g_nbits);
// 			}	
// 		}
// 	
// 	if( code == g_nEOFCode ) 
// 		{
// 		// At EOF, write the rest of the buffer
// 	
//  		//��ʱ����,��ͼ���ĳЩGifͼƬ��windowsͼƬ��������ŻῨ�����
// 		if(g_nEncCount < 2)
//  			{			
//  			//PrintTrace("g_nCurbits=%d,g_nAccum=%d,g_nEncCount=%d",g_nCurbits,g_nAccum,g_nEncCount);
//  			g_nCurbits	= 0;
//  			g_nEncCount	= 0;
//  			}
// 		
// 		while( g_nCurbits > 0 ) 
// 			{
// 			int nCodec = g_nAccum & 0xff;
// 
// 			g_nAccum >>= 8;
// 			g_nCurbits -= 8;
// 
// 			//if(nCodec == 0) break;
// 			char_out(nCodec);
// 			}
// 			
// 		flush_char();			
// 		}
// 
// }

BOOL SaveBmp32File(BYTE * pData,UINT nWidth,UINT nHeight,TCHAR * szFileName,WORD wBits)
{	
	DWORD	dwDIBSize = 0;
	DWORD	dwBmBitSize = 0;
	DWORD	dwWritten;
	INT	nLineSize;	
	
		
	BITMAPFILEHEADER   bmfHdr = { 0 };        
	BITMAPINFOHEADER   bmiHdr = { 0 };
	
	HANDLE hFile = NULL;
	
	if( NULL == pData || nWidth <= 0 || nHeight <= 0 || NULL == szFileName )
		{
		return FALSE;
		}
	
	nLineSize = wBits*nWidth;
	
	nLineSize = FOUR_ALIGN(nLineSize);
	
	//dwBmBitSize = ((nWidth * wBitCount +31) & ~31) /8 * nHeight;
	dwDIBSize=sizeof(BITMAPFILEHEADER)+ sizeof(BITMAPINFOHEADER) + dwBmBitSize; 
	
	//����λͼ��Ϣͷ�ṹ
	bmiHdr.biSize            = sizeof(BITMAPINFOHEADER);
	bmiHdr.biWidth           = nWidth;
	bmiHdr.biHeight          = nHeight;
	bmiHdr.biPlanes          = 1;
	bmiHdr.biBitCount        = wBits*8;
	bmiHdr.biCompression     = BI_RGB;
	bmiHdr.biSizeImage       = nLineSize*nHeight;
	bmiHdr.biXPelsPerMeter   = 0;
	bmiHdr.biYPelsPerMeter   = 0;
	bmiHdr.biClrUsed         = 0;
	bmiHdr.biClrImportant    = 0;
	
	//����λͼ�ļ�    
	hFile = CreateFile(szFileName, GENERIC_WRITE, 
		0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	
	if (hFile == INVALID_HANDLE_VALUE)
		{
		return FALSE;
		}
	
	// ����λͼ�ļ�ͷ
	bmfHdr.bfType = 0x4D42;  // "BM"	 
	bmfHdr.bfSize = dwDIBSize;
	bmfHdr.bfReserved1 = 0;
	bmfHdr.bfReserved2 = 0;
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER);
	
	// д��λͼ�ļ�ͷ
	WriteFile(hFile, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(hFile, (LPSTR)&bmiHdr, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
	
	// д��λͼ�ļ���������
	WriteFile(hFile, (BYTE *)pData , nLineSize*nHeight,&dwWritten, NULL);
	
	CloseHandle(hFile);
	
	return TRUE;
}
	
// //����ѹ��
// int	CompressLZW(int init_bits, LPBYTE lpPixel,INT nPixelNum ,HFILE hOutFile)
// {
// 	code_int	nDictionaryLen;
// 	long		fcode;
// 	long		c;
// 	long		nPrefix;	
// 	long		hshift;
// 	long		disp;
// 	long		i;
// 	int		nCount = 0;
// 	
// 
// 	g_init_bits	= init_bits;
// 	g_hOutFile	= hOutFile;	
// 	g_nTotalCount	= 0;
// 	
// 	// Set up the necessary values
// 	g_nAccum = g_nCurbits = g_nclear_flg = 0;
// 	g_nMaxcode	= (short)MAXCODE(g_nbits = g_init_bits);
// 	nDictionaryLen	= MAX_CODE_LEN;//(code_int)1 << MAXBITSCODES;
// 	
// 	g_nClearCode	 = (1 << (init_bits - 1));
// 	g_nEOFCode		 = g_nClearCode + 1;
// 	g_nDictionaryPos = (short)(g_nClearCode + 2);
// 	
// 	g_nEncCount	= 0;
// 	nPrefix		= lpPixel[0];	
// 	hshift		= 0;
// 
// 	for (fcode = (long)HSIZE;  fcode < 65536L; fcode *= 2L )
// 		++hshift;
// 
// 	hshift = 8 - hshift;       /* set hash code range bound */
// 	clear_hash(HSIZE);        /* clear hash table */
// 	output((code_int)g_nClearCode);
// 	
// 	for(INT j=1;j<nPixelNum;j++)	
// 		{    
// 		c = lpPixel[j];
// 		
// 		fcode = (long) (((long) c << MAXBITSCODES) + nPrefix);
// 		i = (((code_int)c << hshift) ^ nPrefix);    /* xor hashing */
// 		
// 		//if(i>=HSIZE) break;			 
// 		if(Hasg_htabOf (i) == fcode) 
// 			{
// 			nPrefix = g_codetabOf(i);
// 			continue;
// 			} 
// 		else if((long)Hasg_htabOf(i) < 0 )      /* empty slot */
// 			{
// 			goto nomatch;
// 			}
// 
// 		disp = HSIZE - i;           /* secondary hash (after G. Knott) */
// 		if(i == 0 ) disp = 1;
// 
// probe:
// 		if((i -= disp) < 0)
// 			{
// 			i += HSIZE;
// 			}
// 		if(Hasg_htabOf(i) == fcode) 
// 			{
// 			nPrefix = g_codetabOf(i); 
// 			continue; 
// 			}
// 
// 		if((long)Hasg_htabOf(i) > 0)	
// 			{
// 			goto probe;
// 			}
// nomatch:
// 		output((code_int)nPrefix);
// 		nPrefix = c;
// 		if(g_nDictionaryPos < nDictionaryLen) 
// 			{  
// 			g_codetabOf(i) = g_nDictionaryPos++; /* code -> hasg_htable */
// 			Hasg_htabOf(i) = fcode;
// 			} 
// 		else	{
// 			clear_hash(HSIZE);
// 			g_nDictionaryPos=(short)(g_nClearCode+2);
// 			g_nclear_flg=1;
// 			output((code_int)g_nClearCode);
// 
// 			nCount ++;		
// 			}
// 		}
// 	
// 	//PrintTrace("nPrefix=%d,CurPos=%d,%d,nCount=%d,g_nEncCount=%d",nPrefix,g_nDictionaryPos,nDictionaryLen,nCount,g_nEncCount);
// 	//if(g_nEncCount < 32)
// 	//	g_nEncCount = 0;
// 	
// 	// Put out the final code.
// 	output( (code_int)nPrefix);
// 	output( (code_int)g_nEOFCode);
// 
// 	return nPrefix;
// }

LPBYTE GetDiffImage(LPGIFENC_INFO lpGifObj,LPBYTE lpSrcBuf,LPRECT pDiffRect) 
{
	INT	nImageSize	= 0;
	LPBYTE	lpDiffBuf	= NULL;
	RECT	rcFull		= {0};

	
	SetRect(&rcFull,0,0,lpGifObj->nWidth, lpGifObj->nHeight-1);	
	nImageSize = FOUR_ALIGN(lpGifObj->nWidth*3)*lpGifObj->nHeight;
	
	if(lpGifObj->pLastBuf == NULL)	
		{
		lpGifObj->pLastBuf = (LPBYTE)LocalAlloc(LPTR,nImageSize);
		
		CopyMemory(lpGifObj->pLastBuf,lpSrcBuf,nImageSize);
		}
	else	{
		RECT rcDiff;
		BITMAP b1, b2;
		
		ConstructBitmap(&b1, lpGifObj->nWidth, lpGifObj->nHeight, 24, lpGifObj->pLastBuf);
		ConstructBitmap(&b2, lpGifObj->nWidth, lpGifObj->nHeight, 24, lpSrcBuf);
		
		rcDiff = DiffRect(&b1, &b2);
		
		//PrintTrace("left=%d,top=%d,right=%d,bottom=%d", rcDiff.left,rcDiff.top,rcDiff.right,rcDiff.bottom);

		if(EqualRect(&rcDiff,&rcFull) == TRUE)
			{
			//��ȫ��ͬ,ֱ���滻
			CopyMemory(lpGifObj->pLastBuf,lpSrcBuf,nImageSize);
			}
		else if(rcDiff.right ==0 || rcDiff.bottom ==0)
			{
			//��ȫ��ͬ,ֻѹ������
			#define SHORT_LINE	4
			INT	nLineSize = FOUR_ALIGN(lpGifObj->nWidth*3);
			
	
			lpDiffBuf = (LPBYTE)LocalAlloc(LPTR,nLineSize*SHORT_LINE);
			CopyMemory(lpDiffBuf,lpSrcBuf,nLineSize*SHORT_LINE);
			
			SetRect(pDiffRect,0,0,lpGifObj->nWidth,SHORT_LINE);
			}
		else	{					
			//���ֲ�ͬ����ȡ��ͬ�Ĳ���
			INT	nWidth,nHeight;
			INT	nSrcLineSize = FOUR_ALIGN(lpGifObj->nWidth*3);
			INT	nDstLineSize = 0;
			INT	nDstLine = 0;
			

			*pDiffRect = rcDiff;
			nWidth  = rcDiff.right - rcDiff.left;
			nHeight = rcDiff.bottom- rcDiff.top;
					
			
			nDstLineSize =  FOUR_ALIGN(nWidth*3);			
			lpDiffBuf = (LPBYTE)LocalAlloc(LPTR,nWidth*nHeight*4);
						
			for(INT i=rcDiff.top;i<rcDiff.bottom;i++)
				{
				CopyMemory(lpDiffBuf+nDstLine*nDstLineSize,
					   lpSrcBuf+i*nSrcLineSize+rcDiff.left*3,
					   nDstLineSize);

				nDstLine ++;				
				}
			
			CopyMemory(lpGifObj->pLastBuf,lpSrcBuf,nImageSize);
			}	
		}

	return	lpDiffBuf;
}

///////////////////////////////////////////////////////////////////////
//
// ������       : LoadBmpHandle
// ��������     : ����һ��bmp�����Dib����
// ����         : HBITMAP hBitmap	λͼ���
// ����         : nBits			����λ��(32,24)
// ����ֵ       : DIB���ݻ���
//
///////////////////////////////////////////////////////////////////////
LPBYTE	LoadBitmap(HBITMAP hBitmap,INT nBits,PINT pWidth,PINT pHeight)
{
	HDC			hDC		= NULL;
	WORD			wBitCount	= nBits;   //��ǰ��ʾ�ֱ�����ÿ��������ռ�ֽ���
	BITMAP			Bitmap		= {0};        
	BITMAPINFO		bmpInfo		= {0};
	DWORD			dwBmBitsSize	= 0;
	LPBYTE			lpDibBuf	= NULL;
	
	
	if(hBitmap == NULL) return NULL;
	
	hDC	=	CreateDC("DISPLAY", NULL, NULL, NULL);
	
	//����λͼ��Ϣͷ�ṹ
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&Bitmap);
	bmpInfo.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth		= Bitmap.bmWidth;
	bmpInfo.bmiHeader.biHeight		= Bitmap.bmHeight;
	bmpInfo.bmiHeader.biPlanes		= 1;
	bmpInfo.bmiHeader.biBitCount		= wBitCount;
	bmpInfo.bmiHeader.biCompression		= BI_RGB;
	bmpInfo.bmiHeader.biSizeImage		= 0;
	bmpInfo.bmiHeader.biXPelsPerMeter       = 0;
	bmpInfo.bmiHeader.biYPelsPerMeter       = 0;
	bmpInfo.bmiHeader.biClrUsed		= 0;
	bmpInfo.bmiHeader.biClrImportant        = 0;
	
	*pWidth			= Bitmap.bmWidth;
	*pHeight		= Bitmap.bmHeight;
	
	dwBmBitsSize	= ((Bitmap.bmWidth*wBitCount+31)/32)*4*Bitmap.bmHeight;
	
	//Ϊλͼ���ݷ����ڴ�
	lpDibBuf  = (LPBYTE)LocalAlloc(LPTR,dwBmBitsSize);
	
	// ��ȡ�õ�ɫ�����µ�����ֵ
	GetDIBits(hDC, hBitmap, 0, (UINT)Bitmap.bmHeight,lpDibBuf,&bmpInfo, DIB_RGB_COLORS);
	
	DeleteDC(hDC);
	
	return lpDibBuf;
}

//��ָ�����α������ͼ����
BOOL APIENTRY DrawCursorOnDib(LPBYTE lpSrcDIb ,INT nX,INT nY,INT nWidth,INT nHeight,INT nBits,HCURSOR hCursor)
{	
	ICONINFO	szIcon		= {0};
	LPBYTE		lpCursorMask	= NULL;
	LPBYTE		lpCursorColor	= NULL;
	INT		nCursorWidth	= 0;
	INT		nCursorHeight	= 0;	
	INT		nSrcDibLinesize	= 0;	
	INT		nCursorLinesize	= 0;	
	INT		nCursorXoff	= 0;
	INT		nCursorYoff	= 0;
	INT		nCursorLinePos	= 0;
	INT		nPixBytes	= nBits/8;
	BOOL		bRet		= FALSE;
	
	
	if(GetIconInfo(hCursor,&szIcon) == FALSE || lpSrcDIb == NULL)
		{
		return FALSE;	
		}
	
	nCursorXoff = szIcon.xHotspot+nX;
	nCursorYoff = szIcon.yHotspot+nY;
	
	//�õ��α��DIB����
	lpCursorMask	= LoadBitmap(szIcon.hbmMask,nBits,&nCursorWidth,&nCursorHeight);
	lpCursorColor   = LoadBitmap(szIcon.hbmColor,nBits,&nCursorWidth,&nCursorHeight);
	
	nSrcDibLinesize = FOUR_ALIGN(nWidth*nPixBytes);
	nCursorLinesize = FOUR_ALIGN(nCursorWidth*nPixBytes);
	if(lpCursorColor == NULL)
		{
		//����ɫ��ʵ����ɫ����һ��ͼ��	
		lpCursorColor =  lpCursorMask;
		
		lpCursorMask  = lpCursorColor+(nCursorHeight/2*nCursorLinesize);
		nCursorHeight /=2;
		}
	
	nCursorLinePos = szIcon.yHotspot;
	
	//���α�����ͼ����.�Ƚ������ԭͼ�������㣬���ʵ���α�ͼ���ԭͼ�������
	for(INT i=nCursorYoff;i<(nCursorYoff+nCursorHeight);i++)
		{
		LPBYTE	lpSrcLine	 = lpSrcDIb+(nHeight-i-1)*nSrcDibLinesize+nCursorXoff*nPixBytes;
		
		LPBYTE	lpMaskLine	 = lpCursorMask  + (nCursorHeight-nCursorLinePos-1)*nCursorLinesize+szIcon.xHotspot*nPixBytes;
		LPBYTE	lpColorLine	 = lpCursorColor + (nCursorHeight-nCursorLinePos-1)*nCursorLinesize+szIcon.xHotspot*nPixBytes;
		
		nCursorLinePos ++;
		if(nCursorLinePos >= nCursorHeight) break;
		
		for(INT j=0;j<nCursorWidth-szIcon.xHotspot;j++)
			{
			if(j+nCursorXoff >= nWidth) break;
			
			lpSrcLine[0] &= lpMaskLine[0];
			lpSrcLine[1] &= lpMaskLine[1];
			lpSrcLine[2] &= lpMaskLine[2];	
			
			lpSrcLine[0] |= lpColorLine[0];
			lpSrcLine[1] |= lpColorLine[1];
			lpSrcLine[2] |= lpColorLine[2];
			
			
			lpSrcLine	+= nPixBytes;
			lpMaskLine	+= nPixBytes;
			lpColorLine	+= nPixBytes;
			}
		}
	
	
	//�ͷ�	
	LocalFree(lpCursorColor);
	if(szIcon.hbmColor)
		{		
		LocalFree(lpCursorMask);
		}
	
	return TRUE;
}
extern "C" _declspec(dllexport) HANDLE  APIENTRY InitGifEncObj(LPCSTR lpGifFile,INT nWidth,INT nHeight)
{
	LPGIFENC_INFO	lpGifObj	= NULL;
	HFILE		hGifFile	= HFILE_ERROR;	
	GIFHEARD	heard		= {0};
	
	hGifFile = _lcreat(lpGifFile,0);

	if(hGifFile == HFILE_ERROR)
		{
		return NULL;
		}

#ifdef  DEBUG_OUT	
	INT nFrame = 50;
	
	hRgbFile = _lcreat("E:\\Rgbsrc.dat",0);
	
	_lwrite(hRgbFile,(LPCSTR)&nWidth,4);
	_lwrite(hRgbFile,(LPCSTR)&nHeight,4);
	_lwrite(hRgbFile,(LPCSTR)&nFrame,4);
#endif 

	//GIF�ļ�ͷ��89a��ʽ
	memcpy(heard.Signature,"GIF89a",6);
	heard.ScreenWidth = nWidth;
	heard.ScreenHeight = nHeight;
	//heard.GlobalFlagByte = HASPalette|((bitsPixel-1)<<4)|0|(bitsPixel-1);ȫ�ֵ�ɫ��
	heard.GlobalFlagByte = ((8-1)<<4);
	heard.BackGroundColor = 0;
	heard.AspectRatio = 0;
	
	_lwrite(hGifFile,(LPCSTR)&heard,13);//��Ҫ��sizeof(GIFHEARD)�����ǰѱ�������Ϊ1�ֽڶ���

	
	//Ϊ����IEѭ�����ţ���������Ӧ�ó���飨���ֻ��һ��ͼƬ����ʡ�Ըÿ飩
	ApplicationExtension appData;
	appData.extensionIntroducer = 0x21;
	appData.applicationLabel = 0xFF;
	appData.blockSize = 11;
	memcpy(appData.applicationId,"NETSCAPE",8);
	memcpy(appData.appAuthCode,"2.0",3);
	appData.cAppData[0] = 3;
	appData.cAppData[1] = 1;
	appData.cAppData[2] = 0;
	appData.cAppData[3] = 0;
	appData.blockTerminator = 0;
	
	//file.Write(&appData,sizeof(ApplicationExtension));
	_lwrite(hGifFile,(LPCSTR)&appData,sizeof(ApplicationExtension));

	lpGifObj = (LPGIFENC_INFO)LocalAlloc(LPTR,sizeof(GIFENC_INFO));
	lpGifObj->hFileObj	= hGifFile;
	lpGifObj->nWidth	= nWidth;
	lpGifObj->nHeight	= nHeight;

	//ÿ��ͼƬ���1000000������
	//lpGifObj->pEncData	= (LPWORD)LocalAlloc(LPTR,ENC_BUF_LEN); 
	lpGifObj->pColorIndex	= (LPBYTE)LocalAlloc(LPTR,nWidth*nHeight); 

	return  (HANDLE)lpGifObj;
}

extern "C" _declspec(dllexport) BOOL  APIENTRY WriteGifFrame(HANDLE hGifObj,LPBYTE lpRgb24,WORD nDelay,short nTransparentColorIndex)
{
	LPGIFENC_INFO		lpGifObj	= (LPGIFENC_INFO)hGifObj;
	GraphicController	control;	//���ƿ�
	RECT			rcDiff		= {0};	
	BOOL			bDiff		= FALSE;
	INT			nRealWidth	= lpGifObj->nWidth;
	INT			nRealHeight	= lpGifObj->nHeight;
	LPBYTE			lpRealBuf	= NULL;
	BYTE			bitsPixel	= COLORS_BITS;
	BYTE			bEndFlage	= 0;
	INT			nTransColorInx	= 0;		


	if(lpGifObj == NULL)
		{
		return FALSE;
		}

#ifdef  DEBUG_OUT
	INT nLineLne =lpGifObj->nWidth*3;
	
	nLineLne = FOUR_ALIGN(nLineLne);
	_lwrite(hRgbFile,(LPCSTR)lpRgb24,nLineLne*lpGifObj->nHeight);	
#endif 

#ifdef USE_DRAWCURSOR	
	HCURSOR hSor = LoadCursor(NULL,IDC_ARROW);
	DrawCursorOnDib(lpRgb24,10,10,nRealWidth,nRealHeight,24,hSor);
	DeleteObject(hSor);	
#endif	

	InvertedLine(lpRgb24,nRealWidth,nRealHeight);

#ifdef USE_DIFFPROC
	lpRealBuf = GetDiffImage(lpGifObj,lpRgb24,&rcDiff);
#endif 
	if(lpRealBuf)
		{		
		nRealWidth  = rcDiff.right  - rcDiff.left;
		nRealHeight = rcDiff.bottom - rcDiff.top;
		bDiff	    = TRUE;				
		}
	else	{
		lpRealBuf = lpRgb24;
		SetRect(&rcDiff,0,0,lpGifObj->nWidth, lpGifObj->nHeight);
		}


	//����ͼ�񣬲����ֲ���ɫ��
	nTransColorInx = GetPalette(lpGifObj->szLocalclut,lpRealBuf,lpGifObj->pColorIndex,3,nRealWidth,nRealHeight);
	if(bDiff == TRUE)
		{
		LocalFree(lpRealBuf);
		}

	nTransparentColorIndex = nTransColorInx;
		
	control.extensionIntroducer	= 0x21;
	control.graphicControlLabel	= 0xF9;
	control.blockSize		= 4;
	control.packedField		= (nTransparentColorIndex==-1? 4:5);
	control.nDelayTime		= nDelay;//�ӳ�ʱ��
	control.transparentColorIndex	= (BYTE)(nTransparentColorIndex==-1? 0:nTransparentColorIndex);
	control.blockTerminator		= 0;
	
	//file.Write(&control,sizeof(GraphicController));
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&control,sizeof(GraphicController));
	
	//ͼƬ����ͷ
	GIFDATAHEARD dataHeard;
	dataHeard.imageLabel	= 0x2c;
	dataHeard.imageLeft	= rcDiff.left;
	dataHeard.imageTop	= rcDiff.top;
	dataHeard.imageWidth	= nRealWidth;
	dataHeard.imageHeight	= nRealHeight;
	dataHeard.localFlagByte = HASPalette|(COLORS_BITS-1);
	
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.imageLabel,1);
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.imageLeft,2);
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.imageTop,2);
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.imageWidth,2);
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.imageHeight,2);
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&dataHeard.localFlagByte,1);

	//�����ɫ��
	_lwrite(lpGifObj->hFileObj,(LPCSTR)lpGifObj->szLocalclut, (1<<COLORS_BITS)*3);
	
	//ÿ����ռ��λ��
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&bitsPixel,1);
	
	//���ݱ���
	GifEncObj  GifEnc;

	GifEnc.CompressLZW(COLORS_BITS+1,lpGifObj->pColorIndex,nRealWidth*nRealHeight,lpGifObj->hFileObj);
	
	//д�������־
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&bEndFlage,1);

	return TRUE;
}

extern "C" _declspec(dllexport) void APIENTRY FreeGifEncObj(HANDLE hGifObj)
{
	LPGIFENC_INFO	lpGifObj = (LPGIFENC_INFO)hGifObj;
	BYTE		bytes	 = 0x3B;
	

	if(lpGifObj == NULL)
		{
		return ;
		}

	//д���ļ�������־	
	_lwrite(lpGifObj->hFileObj,(LPCSTR)&bytes,1);
	if(lpGifObj->pLastBuf)
		{
		LocalFree(lpGifObj->pLastBuf);	
		}

	LocalFree(lpGifObj->pColorIndex);	
	_lclose(lpGifObj->hFileObj);
	LocalFree(lpGifObj);

#ifdef  DEBUG_OUT
	_lclose(hRgbFile);	
#endif
	//file.Write(&bytes,1);
}