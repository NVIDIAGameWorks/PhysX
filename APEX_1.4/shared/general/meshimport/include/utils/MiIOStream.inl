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


/*!
\file
\brief MiIOStream inline implementation
*/

MI_INLINE MiIOStream& MiIOStream::operator<<(bool v)
{
	if ( mBinary )
	{
		mStream.storeByte((MiU8)v);
	}
	else
	{
		char scratch[6];
		storeString( MiAsc::valueToStr(v, scratch, 6) );
	}
	return *this;
};


MI_INLINE MiIOStream& MiIOStream::operator<<(char c)
{
	mStream.storeByte((MiU8)c);
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiU8 c)
{
	if ( mBinary )
	{
		mStream.storeByte((MiU8)c);
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(c, scratch, MiAsc::IntStrLen) );
	}

	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiI8 c)
{
	if ( mBinary )
	{
		mStream.storeByte((MiU8)c);
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(c, scratch, MiAsc::IntStrLen) );
	}

	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(const char *c)
{
	if ( mBinary )
	{
		c = c ? c : ""; // it it is a null pointer, assign it to an empty string.
		MiU32 len = (MiU32)strlen(c);
		MI_ASSERT( len < (MAX_STREAM_STRING-1));
		if ( len > (MAX_STREAM_STRING-1) )
		{
			len = MAX_STREAM_STRING-1;
		}
		mStream.storeDword(len);
		if ( len )
			mStream.write(c,len);
	}
	else
	{
		storeString(c);
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiU64 v)
{
	if ( mBinary )
	{
		mStream.storeDouble( (MiF64) v );
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiI64 v)
{
	if ( mBinary )
	{
		mStream.storeDouble( (MiF64) v );
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiF64 v)
{
	if ( mBinary )
	{
		mStream.storeDouble( (MiF64) v );
	}
	else
	{
		char scratch[MiAsc::MiF64StrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::MiF64StrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiF32 v)
{
	if ( mBinary )
	{
		mStream.storeFloat(v);
	}
	else
	{
		char scratch[MiAsc::MiF32StrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::MiF32StrLen) );

	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiU32 v)
{
	if ( mBinary )
	{
		mStream.storeDword(v);
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiI32 v)
{
	if ( mBinary )
	{
		mStream.storeDword( (MiU32) v );
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiU16 v)
{
	if ( mBinary )
	{
		mStream.storeWord(v);
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};

MI_INLINE MiIOStream& MiIOStream::operator<<(MiI16 v)
{
	if ( mBinary )
	{
		mStream.storeWord( (MiU16) v );
	}
	else
	{
		char scratch[MiAsc::IntStrLen];
		storeString( MiAsc::valueToStr(v, scratch, MiAsc::IntStrLen) );
	}
	return *this;
};


MI_INLINE MiIOStream& MiIOStream::operator>>(MiU32 &v)
{
	if ( mBinary )
	{
		v = mStream.readDword();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(char &v)
{
	if ( mBinary )
	{
		v = (char)mStream.readByte();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiU8 &v)
{
	if ( mBinary )
	{
		v = mStream.readByte();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiI8 &v)
{
	if ( mBinary )
	{
		v = (MiI8)mStream.readByte();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiI64 &v)
{
	if ( mBinary )
	{
		v = mStream.readDword();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiU64 &v)
{
	if ( mBinary )
	{
		v = (MiU64)mStream.readDouble();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiF64 &v)
{
	if ( mBinary )
	{
		v = mStream.readDouble();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiF32 &v)
{
	if ( mBinary )
	{
		v = mStream.readFloat();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiI32 &v)
{
	if ( mBinary )
	{
		v = (MiI32)mStream.readDword();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiU16 &v)
{
	if ( mBinary )
	{
		v = mStream.readWord();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(MiI16 &v)
{
	if ( mBinary )
	{
		v = (MiI16)mStream.readWord();
	}
	return *this;
}

MI_INLINE MiIOStream& MiIOStream::operator>>(bool &v)
{
	MiU8 iv;
	iv = mStream.readByte();
	v = iv ? true : false;
	return *this;
};

#define IOSTREAM_COMMA_SEPARATOR if(!mBinary) *this << ' ';

MI_INLINE MiIOStream& MiIOStream::operator>>(const char *&str)
{
	str = NULL; // by default no string streamed...
	if ( mBinary )
	{
		MiU32 len=0;
		*this >> len;
		MI_ASSERT( len < (MAX_STREAM_STRING-1) );
		if ( len < (MAX_STREAM_STRING-1) )
		{
			mStream.read(mReadString,len);
			mReadString[len] = 0;
			str = mReadString;
		}
	}
	return *this;
}


MI_INLINE void  MiIOStream::storeString(const char *c,bool zeroTerminate)
{
	while ( *c )
	{
		mStream.storeByte((MiU8)*c);
		c++;
	}
	if ( zeroTerminate )
	{
		mStream.storeByte(0);
	}
}
