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



#ifndef MI_FILE_BUFFER_H
#define MI_FILE_BUFFER_H

#include "Ps.h"
#include "MiFileBuf.h"
#include "PsUserAllocated.h"

namespace mimp
{
//Use this class if you want to use your own allocator
class MiFileBufferBase : public MiFileBuf
{
public:
	MiFileBufferBase(const char *fileName,OpenMode mode)
	{
		mOpenMode = mode;
		mFph = NULL;
		mFileLength = 0;
		mSeekRead   = 0;
		mSeekWrite  = 0;
		mSeekCurrent = 0;
		switch ( mode )
		{
			case OPEN_READ_ONLY:
				mimp::shdfnd::fopen_s(&mFph,fileName,"rb");
				break;
			case OPEN_WRITE_ONLY:
				mimp::shdfnd::fopen_s(&mFph,fileName,"wb");
				break;
			case OPEN_READ_WRITE_NEW:
				mimp::shdfnd::fopen_s(&mFph,fileName,"wb+");
				break;
			case OPEN_READ_WRITE_EXISTING:
				mimp::shdfnd::fopen_s(&mFph,fileName,"rb+");
				break;
		}
		if ( mFph )
		{
			fseek(mFph,0L,SEEK_END);
			mFileLength = ftell(mFph);
			fseek(mFph,0L,SEEK_SET);
		}
		else
		{
			mOpenMode = OPEN_FILE_NOT_FOUND;
		}
    }

	virtual						~MiFileBufferBase()
	{
		close();
	}

	virtual void close()
	{
		if( mFph )
		{
			fclose(mFph);
			mFph = 0;
		}
	}

	virtual SeekType isSeekable(void) const
	{
		return mSeekType;
	}

	virtual		MiU32			read(void* buffer, MiU32 size)	
	{
		MiU32 ret = 0;
		if ( mFph )
		{
			setSeekRead();
			ret = (MiU32)::fread(buffer,1,size,mFph);
			mSeekRead+=ret;
			mSeekCurrent+=ret;
		}
		return ret;
	}

	virtual		MiU32			peek(void* buffer, MiU32 size)
	{
		MiU32 ret = 0;
		if ( mFph )
		{
			MiU32 loc = tellRead();
			setSeekRead();
			ret = (MiU32)::fread(buffer,1,size,mFph);
			mSeekCurrent+=ret;
			seekRead(loc);
		}
		return ret;
	}

	virtual		MiU32		write(const void* buffer, MiU32 size)
	{
		MiU32 ret = 0;
		if ( mFph )
		{
			setSeekWrite();
			ret = (MiU32)::fwrite(buffer,1,size,mFph);
			mSeekWrite+=ret;
			mSeekCurrent+=ret;
			if ( mSeekWrite > mFileLength )
			{
				mFileLength = mSeekWrite;
			}
		}
		return ret;
	}

	virtual MiU32 tellRead(void) const
	{
		return mSeekRead;
	}

	virtual MiU32 tellWrite(void) const
	{
		return mSeekWrite;
	}

	virtual MiU32 seekRead(MiU32 loc) 
	{
		mSeekRead = loc;
		if ( mSeekRead > mFileLength )
		{
			mSeekRead = mFileLength;
		}
		return mSeekRead;
	}

	virtual MiU32 seekWrite(MiU32 loc)
	{
		mSeekWrite = loc;
		if ( mSeekWrite > mFileLength )
		{
			mSeekWrite = mFileLength;
		}
		return mSeekWrite;
	}

	virtual void flush(void)
	{
		if ( mFph )
		{
			::fflush(mFph);
		}
	}

	virtual OpenMode	getOpenMode(void) const
	{
		return mOpenMode;
	}

	virtual MiU32 getFileLength(void) const
	{
		return mFileLength;
	}

private:
	// Moves the actual file pointer to the current read location
	void setSeekRead(void) 
	{
		if ( mSeekRead != mSeekCurrent && mFph )
		{
			if ( mSeekRead >= mFileLength )
			{
				::fseek(mFph,0L,SEEK_END);
			}
			else
			{
				::fseek(mFph,mSeekRead,SEEK_SET);
			}
			mSeekCurrent = mSeekRead = ::ftell(mFph);
		}
	}
	// Moves the actual file pointer to the current write location
	void setSeekWrite(void)
	{
		if ( mSeekWrite != mSeekCurrent && mFph )
		{
			if ( mSeekWrite >= mFileLength )
			{
				::fseek(mFph,0L,SEEK_END);
			}
			else
			{
				::fseek(mFph,mSeekWrite,SEEK_SET);
			}
			mSeekCurrent = mSeekWrite = ::ftell(mFph);
		}
	}


	FILE		*mFph;
	MiU32		mSeekRead;
	MiU32		mSeekWrite;
	MiU32		mSeekCurrent;
	MiU32		mFileLength;
	SeekType	mSeekType;
	OpenMode	mOpenMode;
};

//Use this class if you want to use PhysX memory allocator
class MiFileBuffer: public MiFileBufferBase, public MeshImportAllocated
{
public:
	MiFileBuffer(const char *fileName,OpenMode mode): MiFileBufferBase(fileName, mode) {}
};

}

#endif // MI_FILE_BUFFER_H
