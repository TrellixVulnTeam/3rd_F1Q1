// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Winmm.lib")

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#ifdef _DEBUG
#pragma comment(lib, "zlib1d.lib")
#else
#pragma comment(lib, "zlib1.lib")
#endif

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
