// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// TODO: reference additional headers your program requires here
#include <stdio.h>
#include <windows.h>

#define JPEG_CJPEG_DJPEG
#define JPEG_INTERNALS
#define JPEG_CJPEG_DJPEG
#define PPM_SUPPORTED
#define TARGA_SUPPORTED
#define RLE_SUPPORTED
#define BMP_SUPPORTED
#define GIF_SUPPORTED

#include "cderror.h"
#include "cdjpeg.h"
#include "jchuff.h"
#include "jconfig.h"
#include "jdct.h"
#include "jdhuff.h"
#include "jerror.h"
#include "jinclude.h"
#include "jpegint.h"
#include "jlossls.h"
#include "jlossy.h"
#include "jmemsys.h"
#include "jmorecfg.h"
#include "jpeglib.h"
#include "jversion.h"
#include "transupp.h"
#include "rdswitch.h"
#include "jcodec.h"
#include "jcdiffct.h"
#include "jddiffct.h"
