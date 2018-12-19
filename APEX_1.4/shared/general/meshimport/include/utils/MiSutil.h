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



#ifndef MI_SUTIL_H
#define MI_SUTIL_H

namespace mimp
{

char *         stristr(const char *str,const char *key);       // case insensitive str str
bool           isstristr(const char *str,const char *key);     // bool true/false based on case insenstive strstr
MiU32   GetHEX(const char *foo,const char **next=0);
MiU8  GetHEX1(const char *foo,const char **next=0);
MiU16 GetHEX2(const char *foo,const char **next=0);
MiU32   GetHEX4(const char *foo,const char **next=0);
MiF32          GetFloatValue(const char *str,const char **next=0);
MiI32            GetIntValue(const char *str,const char **next=0);
const char *   SkipWhitespace(const char *str);
bool           IsWhitespace(char c);
const char *   FloatString(MiF32 v,bool binary=false);
const char *   GetTrueFalse(MiU32 state);
bool           CharToWide(const char *source,wchar_t *dest,MiI32 maxlen);
bool           WideToChar(const wchar_t *source,char *dest,MiI32 maxlen);
const char **  GetArgs(char *str,MiI32 &count); // destructable parser, stomps EOS markers on the input string!
MiI32            GetUserArgs(const char *us,const char *key,const char **args);
bool           GetUserSetting(const char *us,const char *key,MiI32 &v);
bool           GetUserSetting(const char *us,const char *key,const char * &v);
const char *   GetRootName(const char *fname); // strip off everything but the 'root' file name.
bool           IsTrueFalse(const char *c);
bool           IsDirectory(const char *fname,char *path,char *basename,char *postfix);
bool           hasSpace(const char *str); // true if the string contains a space
const char *   lastDot(const char *src);
const char *   lastSlash(const char *src); // last forward or backward slash character, null if none found.
const char *   lastChar(const char *src,char c);
const char  	*fstring(MiF32 v);
const char *   formatNumber(MiI32 number); // JWR  format this integer into a fancy comma delimited string
bool           fqnMatch(const char *n1,const char *n2); // returns true if two fully specified file names are 'the same' but ignores case sensitivty and treats either a forward or backslash as the same character.
bool           getBool(const char *str);
bool           needsQuote(const char *str); // if this string needs quotes around it (spaces, commas, #, etc)

#define MAX_FQN 512 // maximum size of an FQN string
void           normalizeFQN(const wchar_t *source,wchar_t *dest);
void           normalizeFQN(const char *source,char *dest);
bool           endsWith(const char *str,const char *ends,bool caseSensitive);

};

#endif // SUTIL_H
