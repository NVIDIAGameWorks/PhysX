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



#ifndef MI_FILE_BUF_H
#define MI_FILE_BUF_H


/** \addtogroup foundation
  @{
*/

namespace mimp
{

MI_PUSH_PACK_DEFAULT

/**
\brief Callback class for data serialization.

The user needs to supply an MiFileBuf implementation to a number of methods to allow the SDK to read or write
chunks of binary data. This allows flexibility for the source/destination of the data. For example the MiFileBuf
could store data in a file, memory buffer or custom file format.

\note It is the users responsibility to ensure that the data is written to the appropriate offset.

*/
class MiFileBuf
{
public:

	enum EndianMode
	{
		ENDIAN_NONE		= 0, // do no conversion for endian mode
		ENDIAN_BIG		= 1, // always read/write data as natively big endian (Power PC, etc.)
		ENDIAN_LITTLE	= 2, // always read/write data as natively little endian (Intel, etc.) Default Behavior!
	};

	MiFileBuf(EndianMode mode=ENDIAN_LITTLE)
	{
		setEndianMode(mode);
	}

	virtual ~MiFileBuf(void)
	{

	}

	/**
	\brief Declares a constant to seek to the end of the stream.
	*
	* Does not support streams longer than 32 bits
	*/
	static const MiU32 STREAM_SEEK_END=0xFFFFFFFF;

	enum OpenMode
	{
		OPEN_FILE_NOT_FOUND,
		OPEN_READ_ONLY,					// open file buffer stream for read only access
		OPEN_WRITE_ONLY,				// open file buffer stream for write only access
		OPEN_READ_WRITE_NEW,			// open a new file for both read/write access
		OPEN_READ_WRITE_EXISTING		// open an existing file for both read/write access
	};

	virtual OpenMode	getOpenMode(void) const  = 0;

	bool isOpen(void) const
	{
		return getOpenMode()!=OPEN_FILE_NOT_FOUND;
	}

	enum SeekType
	{
		SEEKABLE_NO			= 0,
		SEEKABLE_READ		= 0x1,
		SEEKABLE_WRITE		= 0x2,
		SEEKABLE_READWRITE 	= 0x3
	};

	virtual SeekType isSeekable(void) const = 0;

	void	setEndianMode(EndianMode e)
	{
		mEndianMode = e;
		if ( (e==ENDIAN_BIG && !isBigEndian() ) ||
			 (e==ENDIAN_LITTLE && isBigEndian() ) )
		{
			mEndianSwap = true;
		}
 		else
		{
			mEndianSwap = false;
		}
	}

	EndianMode	getEndianMode(void) const
	{
		return mEndianMode;
	}

	virtual MiU32 getFileLength(void) const = 0;

	/**
	\brief Seeks the stream to a particular location for reading
	*
	* If the location passed exceeds the length of the stream, then it will seek to the end.
	* Returns the location it ended up at (useful if you seek to the end) to get the file position
	*/
	virtual MiU32	seekRead(MiU32 loc) = 0;

	/**
	\brief Seeks the stream to a particular location for writing
	*
	* If the location passed exceeds the length of the stream, then it will seek to the end.
	* Returns the location it ended up at (useful if you seek to the end) to get the file position
	*/
	virtual MiU32	seekWrite(MiU32 loc) = 0;

	/**
	\brief Reads from the stream into a buffer.

	\param[out] mem  The buffer to read the stream into.
	\param[in]  len  The number of bytes to stream into the buffer

	\return Returns the actual number of bytes read.  If not equal to the length requested, then reached end of stream.
	*/
	virtual MiU32	read(void *mem,MiU32 len) = 0;


	/**
	\brief Reads from the stream into a buffer but does not advance the read location.

	\param[out] mem  The buffer to read the stream into.
	\param[in]  len  The number of bytes to stream into the buffer

	\return Returns the actual number of bytes read.  If not equal to the length requested, then reached end of stream.
	*/
	virtual MiU32	peek(void *mem,MiU32 len) = 0;

	/**
	\brief Writes a buffer of memory to the stream

	\param[in] mem The address of a buffer of memory to send to the stream.
	\param[in] len  The number of bytes to send to the stream.

	\return Returns the actual number of bytes sent to the stream.  If not equal to the length specific, then the stream is full or unable to write for some reason.
	*/
	virtual MiU32	write(const void *mem,MiU32 len) = 0;

	/**
	\brief Reports the current stream location read aqccess.

	\return Returns the current stream read location.
	*/
	virtual MiU32	tellRead(void) const = 0;

	/**
	\brief Reports the current stream location for write access.

	\return Returns the current stream write location.
	*/
	virtual MiU32	tellWrite(void) const = 0;

	/**
	\brief	Causes any temporarily cached data to be flushed to the stream.
	*/
	virtual	void	flush(void) = 0;

	/**
	\brief	Close the stream.
	*/
	virtual void close(void) {}

	void release(void)
	{
		delete this;
	}

    static MI_INLINE bool isBigEndian()
     {
       MiI32 i = 1;
        return *((char*)&i)==0;
    }

    MI_INLINE void swap2Bytes(void* _data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[1]; data[1] = one_byte;
    }

    MI_INLINE void swap4Bytes(void* _data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[3]; data[3] = one_byte;
		one_byte = data[1]; data[1] = data[2]; data[2] = one_byte;
    }

    MI_INLINE void swap8Bytes(void *_data) const
    {
		char *data = (char *)_data;
		char one_byte;
		one_byte = data[0]; data[0] = data[7]; data[7] = one_byte;
		one_byte = data[1]; data[1] = data[6]; data[6] = one_byte;
		one_byte = data[2]; data[2] = data[5]; data[5] = one_byte;
		one_byte = data[3]; data[3] = data[4]; data[4] = one_byte;
    }


	MI_INLINE void storeDword(MiU32 v)
	{
		if ( mEndianSwap )
		    swap4Bytes(&v);

		write(&v,sizeof(v));
	}

	MI_INLINE void storeFloat(MiF32 v)
	{
		if ( mEndianSwap )
			swap4Bytes(&v);
		write(&v,sizeof(v));
	}

	MI_INLINE void storeDouble(MiF64 v)
	{
		if ( mEndianSwap )
			swap8Bytes(&v);
		write(&v,sizeof(v));
	}

	MI_INLINE  void storeByte(MiU8 b)
	{
		write(&b,sizeof(b));
	}

	MI_INLINE void storeWord(MiU16 w)
	{
		if ( mEndianSwap )
			swap2Bytes(&w);
		write(&w,sizeof(w));
	}

	MiU8 readByte(void) 
	{
		MiU8 v=0;
		read(&v,sizeof(v));
		return v;
	}

	MiU16 readWord(void) 
	{
		MiU16 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap2Bytes(&v);
		return v;
	}

	MiU32 readDword(void) 
	{
		MiU32 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap4Bytes(&v);
		return v;
	}

	MiF32 readFloat(void) 
	{
		MiF32 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap4Bytes(&v);
		return v;
	}

	MiF64 readDouble(void) 
	{
		MiF64 v=0;
		read(&v,sizeof(v));
		if ( mEndianSwap )
		    swap8Bytes(&v);
		return v;
	}

private:
	bool		mEndianSwap;	// whether or not the endian should be swapped on the current platform
	EndianMode	mEndianMode;  	// the current endian mode behavior for the stream
};

MI_POP_PACK


}; // end of namespace

#endif // MI_FILE_BUF_H
