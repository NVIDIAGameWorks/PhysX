//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "MiInparser.h"
#include "MiSutil.h"

namespace mimp
{

static char ToLower(char c)
{
	if ( c >= 'A' && c <= 'Z' ) c+=32;
	return c;
}

char *stristr(const char *str,const char *key)       // case insensitive str str
{
	MI_ASSERT( strlen(str) < 2048 );
	MI_ASSERT( strlen(key) < 2048 );
	char istr[2048];
	char ikey[2048];
	strncpy(istr,str,2048);
	strncpy(ikey,key,2048);
	MESH_IMPORT_STRING::strlwr(istr);
	MESH_IMPORT_STRING::strlwr(ikey);

	char *foo = strstr(istr,ikey);
	if ( foo )
	{
		MiU32 loc = (MiU32)(foo - istr);
		foo = (char *)str+loc;
	}

	return foo;
}

bool        isstristr(const char *str,const char *key)     // bool true/false based on case insenstive strstr
{
	bool ret = false;
	const char *found = strstr(str,key);
	if ( found ) ret = true;
	return ret;
}

MiU32 GetHex(MiU8 c)
{
	MiU32 v = 0;
	c = (MiU8)ToLower((char)c);
	if ( c >= '0' && c <= '9' )
		v = MiU32(c-'0');
	else
	{
		if ( c >= 'a' && c <= 'f' )
		{
			v = MiU32(10 + c-'a');
		}
	}
	return v;
}

MiU8 GetHEX1(const char *foo,const char **endptr)
{
	MiU32 ret = 0;

	ret = (GetHex((MiU8)foo[0])<<4) | GetHex((MiU8)foo[1]);

	if ( endptr )
	{
		*endptr = foo+2;
	}

	return (MiU8) ret;
}


MiU16 GetHEX2(const char *foo,const char **endptr)
{
	MiU32 ret = 0;

	ret = (GetHex((MiU8)foo[0])<<12) | (GetHex((MiU8)foo[1])<<8) | (GetHex((MiU8)foo[2])<<4) | GetHex((MiU8)foo[3]);

	if ( endptr )
	{
		*endptr = foo+4;
	}

	return (MiU16) ret;
}

MiU32 GetHEX4(const char *foo,const char **endptr)
{
	MiU32 ret = 0;

	for (MiI32 i=0; i<8; i++)
	{
		ret = (ret<<4) | GetHex((MiU8)foo[i]);
	}

	if ( endptr )
	{
		*endptr = foo+8;
	}

	return ret;
}

MiU32 GetHEX(const char *foo,const char **endptr)
{
	MiU32 ret = 0;

	while ( *foo )
	{
		MiU8 c = (MiU8)ToLower( *foo );
		MiU32 v = 0;
		if ( c >= '0' && c <= '9' )
			v = MiU32(c-'0');
		else
		{
			if ( c >= 'a' && c <= 'f' )
			{
				v = MiU32(10 + c-'a');
			}
			else
				break;
		}
		ret = (ret<<4)|v;
		foo++;
	}

	if ( endptr ) *endptr = foo;

	return ret;
}


bool         IsWhitespace(char c)
{
	if ( c == ' ' || c == 9 || c == 13 || c == 10 || c == ',' ) return true;
	return false;
}


const char * SkipWhitespace(const char *str)
{
	while ( *str && IsWhitespace(*str) ) str++;
	return str;
}

#define MAXNUM 32

MiF32        GetFloatValue(const char *str,const char **next)
{
	MiF32 ret = 0;

	if ( next ) *next = 0;

	str = SkipWhitespace(str);

	char dest[MAXNUM];
	char *dst = dest;
	const char *hex = 0;

	for (MiI32 i=0; i<(MAXNUM-1); i++)
	{
		char c = *str;
		if ( c == 0 || IsWhitespace(c) )
		{
			if ( next ) *next = str;
			break;
		}
		else if ( c == '$' )
		{
			hex = str+1;
		}
		*dst++ = ToLower(c);
		str++;
	}

	*dst = 0;

	if ( hex )
	{
		MiU32 iv = GetHEX(hex,0);
		MiF32 *v = (MiF32 *)&iv;
		ret = *v;
	}
	else if ( dest[0] == 'f' )
	{
		if ( strcmp(dest,"fltmax") == 0 || strcmp(dest,"fmax") == 0 )
		{
			ret = FLT_MAX;
		}
		else if ( strcmp(dest,"fltmin") == 0 || strcmp(dest,"fmin") == 0 )
		{
			ret = FLT_MIN;
		}
	}
	else if ( dest[0] == 't' ) // t or 'true' is treated as the value '1'.
	{
		ret = 1;
	}
	else
	{
		ret = (MiF32)atof(dest);
	}
	return ret;
}

MiI32          GetIntValue(const char *str,const char **next)
{
	MiI32 ret = 0;

	if ( next ) *next = 0;

	str = SkipWhitespace(str);

	char dest[MAXNUM];
	char *dst = dest;

	for (MiI32 i=0; i<(MAXNUM-1); i++)
	{
		char c = *str;
		if ( c == 0 || IsWhitespace(c) )
		{
			if ( next ) *next = str;
			break;
		}
		*dst++ = c;
		str++;
	}

	*dst = 0;

	ret = atoi(dest);

	return ret;
}


bool CharToWide(const char *source,wchar_t *dest,MiI32 maxlen)
{
	bool ret = false;

	ret = true;
	mbstowcs(dest, source, (size_t)maxlen );

	return ret;
}

bool WideToChar(const wchar_t *source,char *dest,MiI32 maxlen)
{
	bool ret = false;

	ret = true;
	wcstombs(dest, source, (size_t)maxlen );

	return ret;
}



const char * GetTrueFalse(MiU32 state)
{
	if ( state ) return "true";
	return "false";
};


const char * FloatString(MiF32 v,bool binary)
{
	static char data[64*16];
	static MiI32  index=0;

	char *ret = &data[index*64];
	index++;
	if (index == 16 ) index = 0;

	if ( !MESH_IMPORT_INTRINSICS::isFinite(v) )
  {
    MI_ALWAYS_ASSERT();
    strcpy(ret,"0"); // not a valid number!
  }
/***
	else if ( v == FLT_MAX )
	{
		strcpy(ret,"fltmax");
	}
	else if ( v == FLT_MIN )
	{
		strcpy(ret,"fltmin");
	}
***/
	else if ( v == 1 )
	{
		strcpy(ret,"1");
	}
	else if ( v == 0 )
	{
		strcpy(ret,"0");
	}
	else if ( v == - 1 )
	{
		strcpy(ret,"-1");
	}
	else
	{
		if ( binary )
		{
			MiU32 *iv = (MiU32 *) &v;
			MESH_IMPORT_STRING::snprintf(ret,64,"%.4f$%x", v, *iv );
		}
		else
		{
			MESH_IMPORT_STRING::snprintf(ret,64,"%.9f", v );
			const char *dot = strstr(ret,".");
			if ( dot )
			{
				MiI32 len = (MiI32)strlen(ret);
				char *foo = &ret[len-1];
				while ( *foo == '0' ) foo--;
				if ( *foo == '.' )
					*foo = 0;
				else
					foo[1] = 0;
			}
		}
	}

	return ret;
}


char * NextSep(char *str,char &c)
{
	while ( *str && *str != ',' && *str != ')' )
	{
		str++;
	}
	c = *str;
	return str;
}

MiI32 GetUserArgs(const char *us,const char *key,const char **args)
{
	MiI32 ret = 0;
	static char arglist[2048];
	strcpy(arglist,us);

	char keyword[512];
	MESH_IMPORT_STRING::snprintf(keyword,512,"%s(", key );
	char *found = strstr(arglist,keyword);
	if ( found )
	{
		found = strstr(found,"(");
		found++;
		args[ret] = found;
		ret++;
    static bool bstate = true;
		while ( bstate )
		{
			char c;
			found = NextSep(found,c);
			if ( found )
			{
				*found = 0;
				if ( c == ',' )
				{
					found++;
					args[ret] = found;
					ret++;
				}
				else
				{
					break;
				}
			}
		}
	}
	return ret;
}

bool GetUserSetting(const char *us,const char *key,MiI32 &v)
{
	bool ret = false;

	const char *argv[256];

	MiI32 argc = GetUserArgs(us,key,argv);
	if ( argc )
	{
		v = atoi( argv[0] );
		ret = true;
	}
	return ret;
}

bool GetUserSetting(const char *us,const char *key,const char * &v)
{
	bool ret = false;

	const char *argv[256];
	MiI32 argc = GetUserArgs(us,key,argv);
	if ( argc )
	{
		v = argv[0];
		ret = true;
	}
	return ret;
}

const char **  GetArgs(char *str,MiI32 &count) // destructable parser, stomps EOS markers on the input string!
{
	InPlaceParser ipp;

	return ipp.GetArglist(str,count);
}

const char * GetRootName(const char *fname)
{
	static char scratch[512];

	const char *source = fname;

	const char *start  = fname;

  while ( *source )
  {
  	if ( *source == '/' || *source == '\\' )
  	{
  		start = source+1;
  	}
  	source++;
  }

	strcpy(scratch,start);

  char *dot = strrchr( scratch, '.' );
 
  if ( dot )
  {
  	*dot = 0;
  }

	return scratch;
}

bool IsTrueFalse(const char *c)
{
	bool ret = false;

	if ( MESH_IMPORT_STRING::stricmp(c,"true") == 0 || MESH_IMPORT_STRING::stricmp(c,"1") == 0 ) ret = true;

  return ret;
}


bool IsDirectory(const char *fname,char *path,char *basename,char *postfix)
{
	bool ret = false;

	strcpy(path,fname);
	strcpy(basename,fname);

	char *foo = path;
	char *last = 0;

	while ( *foo )
	{
		if ( *foo == '\\' || *foo == '/' ) last = foo;
		foo++;
	}

	if ( last )
	{
		strcpy(basename,last+1);
		*last = 0;
		ret = true;
	}

	const char *scan = fname;

  static bool bstate = true;
	while ( bstate )
	{
		const char *dot = strstr(scan,".");
		if ( dot == 0 )
				break;
		scan = dot+1;
	}

	strcpy(postfix,scan);
	MESH_IMPORT_STRING::strlwr(postfix);

	return ret;
}

bool hasSpace(const char *str) // true if the string contains a space
{
	bool ret = false;

  while ( *str )
  {
  	char c = *str++;
  	if ( c == 32 || c == 9 )
  	{
  		ret = true;
  		break;
  	}
  }
  return ret;
}


const char * lastDot(const char *src)
{
  const char *ret = 0;

  const char *dot = strchr(src,'.');
  while ( dot )
  {
    ret = dot;
    dot = strchr(dot+1,'.');
  }
  return ret;
}


const char *   lastChar(const char *src,char c)
{
  const char *ret = 0;

  const char *dot = (const char *)strchr(src,c);
  while ( dot )
  {
    ret = dot;
    dot = (const char *)strchr(dot+1,c);
  }
  return ret;
}


const char *         lastSlash(const char *src) // last forward or backward slash character, null if none found.
{
  const char *ret = 0;

  const char *dot = strchr(src,'\\');
  if  ( dot == 0 )
    dot = strchr(src,'/');
  while ( dot )
  {
    ret = dot;
    dot = strchr(ret+1,'\\');
    if ( dot == 0 )
      dot = strchr(ret+1,'/');
  }
  return ret;
}


const char	*fstring(MiF32 v)
{
	static char	data[64	*16];
	static MiI32 index = 0;

	char *ret	=	&data[index	*64];
	index++;
	if (index	== 16)
	{
		index	=	0;
	}

	if (v	== FLT_MIN)
	{
		return "-INF";
	}
	// collada notation	for	FLT_MIN	and	FLT_MAX
	if (v	== FLT_MAX)
	{
		return "INF";
	}

	if (v	== 1)
	{
		strcpy(ret,	"1");
	}
	else if	(v ==	0)
	{
		strcpy(ret,	"0");
	}
	else if	(v ==	 - 1)
	{
		strcpy(ret,	"-1");
	}
	else
	{
		MESH_IMPORT_STRING::snprintf(ret,16, "%.9f", v);
		const	char *dot	=	strstr(ret,	".");
		if (dot)
		{
			MiI32	len	=	(MiI32)strlen(ret);
			char *foo	=	&ret[len - 1];
			while	(*foo	== '0')
			{
				foo--;
			}
			if (*foo ==	'.')
			{
				*foo = 0;
			}
			else
			{
				foo[1] = 0;
			}
		}
	}

	return ret;
}


#define MAXNUMERIC 32  // JWR  support up to 16 32 character long numeric formated strings
#define MAXFNUM    16

static	char  gFormat[MAXNUMERIC*MAXFNUM];
static MiI32    gIndex=0;

const char * formatNumber(MiI32 number) // JWR  format this integer into a fancy comma delimited string
{
	char * dest = &gFormat[gIndex*MAXNUMERIC];
	gIndex++;
	if ( gIndex == MAXFNUM ) gIndex = 0;

	char scratch[512];

#if defined (LINUX_GENERIC) || defined(LINUX) || defined(__CELLOS_LV2__) || defined(__APPLE__) || defined(ANDROID) || PX_PS4 || PX_LINUX_FAMILY || PX_SWITCH
	snprintf(scratch, 10, "%d", number);
#else
	itoa(number,scratch,10);
#endif

	char *str = dest;
	MiU32 len = (MiU32)strlen(scratch);
	for (MiU32 i=0; i<len; i++)
	{
		MiI32 place = ((MiI32)len-1)-(MiI32)i;
		*str++ = scratch[i];
		if ( place && (place%3) == 0 ) *str++ = ',';
	}
	*str = 0;

	return dest;
}


bool fqnMatch(const char *n1,const char *n2) // returns true if two fully specified file names are 'the same' but ignores case sensitivty and treats either a forward or backslash as the same character.
{
  bool ret = true;

  while ( *n1 )
  {
    char c1 = *n1++;
    char c2 = *n2++;
    if ( c1 >= 'A' && c1 <= 'Z' ) c1+=32;
    if ( c2 >= 'A' && c2 <= 'Z' ) c2+=32;
    if ( c1 == '\\' ) c1 = '/';
    if ( c2 == '\\' ) c2 = '/';
    if ( c1 != c2 )
    {
      ret = false;
      break;
    }
  }
  if ( ret )
  {
    if ( *n2 ) ret = false;
  }

  return ret;

}


bool           getBool(const char *str)
{
  bool ret = false;

  if ( MESH_IMPORT_STRING::stricmp(str,"true") == 0 || strcmp(str,"1") == 0 || MESH_IMPORT_STRING::stricmp(str,"yes") == 0 ) ret = true;

  return ret;
}


bool needsQuote(const char *str) // if this string needs quotes around it (spaces, commas, #, etc)
{
  bool ret = false;

  if ( str )
  {
    while ( *str )
    {
      char c = *str++;
      if ( c == ',' || c == '#' || c == 32 || c == 9 )
      {
        ret = true;
        break;
      }
    }
  }
  return ret;
}

void  normalizeFQN(const wchar_t *source,wchar_t *dest)
{
  char scratch[512];
  WideToChar(source,scratch,512);
  char temp[512];
  normalizeFQN(scratch,temp);
  CharToWide(temp,dest,512);
}

void  normalizeFQN(const char *source,char *dest)
{
  if ( source && strlen(source ) )
  {
    while ( *source )
    {
      char c = *source++;
      if ( c == '\\' ) c = '/';
      if ( c >= 'A' && c <= 'Z' ) c+=32;
      *dest++ = c;
    }
    *dest = 0;
  }
  else
  {
    *dest = 0;
  }
}



bool           endsWith(const char *str,const char *ends,bool caseSensitive)
{
  bool ret = false;

  MiI32 l1 = (MiI32) strlen(str);
  MiI32 l2 = (MiI32) strlen(ends);
  if ( l1 >= l2 )
  {
    MiI32 diff = l1-l2;
    const char *echeck = &str[diff];
    if ( caseSensitive )
    {
		if ( strcmp(echeck,ends) == 0 )
      {
        ret = true;
      }
    }
    else
    {
		if ( MESH_IMPORT_STRING::stricmp(echeck,ends) == 0 )
      {
        ret = true;
      }
    }
  }
  return ret;
}

};
