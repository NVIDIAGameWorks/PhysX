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


#ifndef GET_ARGS_H

#define GET_ARGS_H

#define MAX_ARGS 4096
#define MAX_ARG_STRING 16384

class GetArgs
{
public:

	const char **getArgs(const char *str,uint32_t &argc)
	{
		argc = 0;

		str = skipSpaces(str); // skip any leading spaces
		char *dest = mArgString;
		char *stop = &mArgString[MAX_ARG_STRING-1];

		while ( str && *str && dest < stop ) // if we have a valid string then we have at least one argument..
		{
			if ( *str == 34 ) // if it is a quoted string...
			{
				str++; // Skip the opening quote
				if ( *str == 34 ) // double quotes I guess we treat as an empty string
				{
					mArgv[argc] = "";
					argc++;
				}
				else
				{
					mArgv[argc] = dest; // store the beginning of the argument
					argc++;
					while ( *str && *str != 34 && dest < stop )
					{
						*dest++ = *str++;
					}
					*dest = 0; // null terminate the quoted argument.
					dest++;
				}
				if ( *str == 34 ) // skip closing quote
				{
					str++;
				}
				str = skipSpaces(str);
			}
			else
			{
				mArgv[argc] = dest;
				argc++;
				while ( *str && !isWhiteSpace(*str) && dest < stop )
				{
					*dest++ = *str++;
				}
				*dest = 0;
				dest++;
				str = skipSpaces(str);
			}
		}

		return mArgv;
	}

private:
	PX_INLINE bool isWhiteSpace(char c) const
	{
		if ( c == 32 || c == 9 ) return true;
		return false;
	}

	PX_INLINE const char *skipSpaces(const char *str) const
	{
		if ( str )
		{
			while ( *str == 32 || *str == 9 )
			{
				str++;
			}
		}
		return str;
	}

	const char	*mArgv[MAX_ARGS];
	char		mArgString[MAX_ARG_STRING];

};

#endif
