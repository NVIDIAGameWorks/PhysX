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



#include "MiFastXml.h"
#include <stdio.h>
#include <string.h>
#include <new>

#define DEBUG_LOG 0

namespace mimp
{

#define MIN_CLOSE_COUNT 2
#define DEFAULT_READ_BUFFER_SIZE (16*1024)

#define DEBUG_ASSERT(x) //MI_ASSERT(x)
#define DEBUG_ALWAYS_ASSERT() DEBUG_ASSERT(0)

#define U(c) ((unsigned char)(c))

class MyFastXml : public FastXml
{
public:
	enum CharType
	{
		CT_DATA,
		CT_EOF,
		CT_SOFT,
		CT_END_OF_ELEMENT, // either a forward slash or a greater than symbol
		CT_END_OF_LINE,
	};

	MyFastXml(Callback *c)
	{
		mStreamFromMemory = true;
		mCallback = c;
		memset(mTypes, CT_DATA, sizeof(mTypes));
		mTypes[0] = CT_EOF;
		mTypes[U(' ')] = mTypes[U('\t')] = CT_SOFT;
		mTypes[U('/')] = mTypes[U('>')] = mTypes[U('?')] = CT_END_OF_ELEMENT;
		mTypes[U('\n')] = mTypes[U('\r')] = CT_END_OF_LINE;
		mError = 0;
		mStackIndex = 0;
		mFileBuf = NULL;
		mReadBufferEnd = NULL;
		mReadBuffer = NULL;
		mReadBufferSize = DEFAULT_READ_BUFFER_SIZE;
		mOpenCount = 0;
		mLastReadLoc = 0;
		for (MiU32 i=0; i<(MAX_STACK+1); i++)
		{
			mStack[i] = NULL;
			mStackAllocated[i] = false;
		}
#if DEBUG_LOG
		mFph = fopen("xml_log.txt","wb");
#endif
	}

	virtual ~MyFastXml(void)
	{
		releaseMemory();
#if DEBUG_LOG
		if ( mFph )
		{
			fclose(mFph);
		}
#endif
	}

#if DEBUG_LOG
	void indent(void)
	{
		if ( mFph )
		{
			for (MiU32 i=0; i<mStackIndex; i++)
			{
				fprintf(mFph,"\t");
				fflush(mFph);
			}
		}
	}
#endif

	char *processClose(char c, const char *element, char *scan, MiI32 argc, const char **argv, FastXml::Callback *iface,bool &isError)
	{
		isError = true; // by default, if we return null it's due to an error.
		if ( c == '/' || c == '?' )
		{
			char *slash = (char *)strchr(element, c);
			if( slash )
				*slash = 0;

			if( c == '?' && strcmp(element, "xml") == 0 )
			{
				if( !iface->processXmlDeclaration(argc/2, argv, 0, mLineNo) )
					return NULL;
			}
			else
			{
#if DEBUG_LOG
				if ( mFph )
				{
					indent();
					fprintf(mFph,"<%s ",element);
					for (MiI32 i=0; i<argc/2; i++)
					{
						fprintf(mFph," %s=\"%s\"", argv[i*2], argv[i*2+1] );
					}
					fprintf(mFph,">\r\n");
					fflush(mFph);
				}
#endif

				if ( !iface->processElement(element, argc, argv, 0, mLineNo) )
				{
					mError = "User aborted the parsing process";
					return NULL;
				}

				pushElement(element);

				const char *close = popElement();
#if DEBUG_LOG
				indent();
				fprintf(mFph,"</%s>\r\n", close );
				fflush(mFph);
#endif	
				if( !iface->processClose(close,mStackIndex,isError) )
				{
					return NULL;
				}
			}

			if ( !slash )
				++scan;
		}
		else
		{
			scan = skipNextData(scan);
			char *data = scan; // this is the data portion of the element, only copies memory if we encounter line feeds
			char *dest_data = 0;
			while ( *scan && *scan != '<' )
			{
				if ( mTypes[U(*scan)] == CT_END_OF_LINE )
				{
					if ( *scan == '\r' ) mLineNo++;
					dest_data = scan;
					*dest_data++ = ' '; // replace the linefeed with a space...
					scan = skipNextData(scan);
					while ( *scan && *scan != '<' )
					{
						if ( mTypes[U(*scan)] == CT_END_OF_LINE )
						{
							if ( *scan == '\r' ) mLineNo++;
							*dest_data++ = ' '; // replace the linefeed with a space...
							scan = skipNextData(scan);
						}
						else
						{
							*dest_data++ = *scan++;
						}
					}
					break;
				}
				else
					++scan;
			}

			if ( *scan == '<' )
			{
				if ( scan[1] != '/' )
				{
					MI_ASSERT(mOpenCount>0);
					mOpenCount--;
				}
				if ( dest_data )
				{
					*dest_data = 0;
				}
				else
				{
					*scan = 0;
				}

				scan++; // skip it..

				if ( *data == 0 ) data = 0;

#if DEBUG_LOG
				if ( mFph )
				{
					indent();
					fprintf(mFph,"<%s ",element);
					for (MiI32 i=0; i<argc/2; i++)
					{
						fprintf(mFph," %s=\"%s\"", argv[i*2], argv[i*2+1] );
					}
					fprintf(mFph,">\r\n");
					if ( data )
					{
						indent();
						fprintf(mFph,"%s\r\n", data );
					}
					fflush(mFph);
				}
#endif
				if ( !iface->processElement(element, argc, argv, data, mLineNo) )
				{
					mError = "User aborted the parsing process";
					return 0;
				}

				pushElement(element);

				// check for the comment use case...
				if ( scan[0] == '!' && scan[1] == '-' && scan[2] == '-' )
				{
					scan+=3;
					while ( *scan && *scan == ' ' )
						++scan;

					char *comment = scan;
					char *comment_end = strstr(scan, "-->");
					if ( comment_end )
					{
						*comment_end = 0;
						scan = comment_end+3;
						if( !iface->processComment(comment) )
						{
							mError = "User aborted the parsing process";
							return 0;
						}
					}
				}
				else if ( *scan == '/' )
				{
					scan = processClose(scan, iface, isError);
					if( scan == NULL ) 
					{
						return NULL;
					}
				}
			}
			else
			{
				mError = "Data portion of an element wasn't terminated properly";
				return NULL;
			}
		}

		if ( mOpenCount < MIN_CLOSE_COUNT )
		{
			scan = readData(scan);
		}

		return scan;
	}

	char *processClose(char *scan, FastXml::Callback *iface,bool &isError)
	{
		const char *start = popElement(), *close = start;
		if( scan[1] != '>')
		{
			scan++;
			close = scan;
			while ( *scan && *scan != '>' ) scan++;
			*scan = 0;
		}

		if( 0 != strcmp(start, close) )
		{
			mError = "Open and closing tags do not match";
			return 0;
		}
#if DEBUG_LOG
		indent();
		fprintf(mFph,"</%s>\r\n", close );
		fflush(mFph);
#endif	
		if( !iface->processClose(close,mStackIndex,isError) )
		{
			// we need to set the read pointer!
			MiU32 offset = (MiU32)(mReadBufferEnd-scan)-1;
			MiU32 readLoc = mLastReadLoc-offset;
			mFileBuf->seekRead(readLoc); 
			return NULL;
		}
		++scan;

		return scan;
	}

	virtual bool processXml(MiFileBuf &fileBuf,bool streamFromMemory)
	{
		releaseMemory();
		mFileBuf = &fileBuf;
		mStreamFromMemory = streamFromMemory;
		return processXml(mCallback);
	}

	// if we have finished processing the data we had pending..
	char * readData(char *scan)
	{
		for (MiU32 i=0; i<(mStackIndex+1); i++)
		{
			if ( !mStackAllocated[i] )
			{
				const char *text = mStack[i];
				if ( text )
				{
					MiU32 tlen = (MiU32)strlen(text);
					mStack[i] = (const char *)mCallback->fastxml_malloc(tlen+1);
					memcpy((void *)mStack[i],text,tlen+1);
					mStackAllocated[i] = true;

				}
			}
		}

		if ( !mStreamFromMemory )
		{
			if ( scan == NULL )
			{
				MiU32 seekLoc = mFileBuf->tellRead();
				mReadBufferSize = (mFileBuf->getFileLength()-seekLoc);
			}
			else
			{
				return scan;
			}
		}

		if ( mReadBuffer == NULL )
		{
			mReadBuffer = (char *)mCallback->fastxml_malloc(mReadBufferSize+1);
		}
		MiU32 offset = 0;
		MiU32 readLen = mReadBufferSize;

		if ( scan )
		{
			offset = (MiU32)(scan - mReadBuffer );
			MiU32 copyLen = mReadBufferSize-offset;
			if ( copyLen )
			{
				MI_ASSERT(scan >= mReadBuffer);
				memmove(mReadBuffer,scan,copyLen);
				mReadBuffer[copyLen] = 0;
				readLen = mReadBufferSize - copyLen;
			}
			offset = copyLen;
		}

		MiU32 readCount = mFileBuf->read(&mReadBuffer[offset],readLen);

		while ( readCount > 0 )
		{

			mReadBuffer[readCount+offset] = 0; // end of string terminator...
			mReadBufferEnd = &mReadBuffer[readCount+offset];

			const char *scan = &mReadBuffer[offset];
			while ( *scan )
			{
				if ( *scan == '<' && scan[1] != '/' )
				{
					mOpenCount++;
				}
				scan++;
			}

			if ( mOpenCount < MIN_CLOSE_COUNT )
			{
				MiU32 oldSize = (MiU32)(mReadBufferEnd-mReadBuffer);
				mReadBufferSize = mReadBufferSize*2;
				char *oldReadBuffer = mReadBuffer;
				mReadBuffer = (char *)mCallback->fastxml_malloc(mReadBufferSize+1);
				memcpy(mReadBuffer,oldReadBuffer,oldSize);
				mCallback->fastxml_free(oldReadBuffer);
				offset = oldSize;
				MiU32 readSize = mReadBufferSize - oldSize;
				readCount = mFileBuf->read(&mReadBuffer[offset],readSize);
				if ( readCount == 0 )
					break;
			}
			else
			{
				break;
			}
		}
		mLastReadLoc = mFileBuf->tellRead();

		return mReadBuffer;
	}

	bool processXml(FastXml::Callback *iface)
	{
		bool ret = true;

		const int MAX_ATTRIBUTE = 2048; // can't imagine having more than 2,048 attributes in a single element right?

		mLineNo = 1;

		char *element, *scan = readData(0);

		while( *scan )
		{

			scan = skipNextData(scan);

			if( *scan == 0 ) break;

			if( *scan == '<' )
			{

				if ( scan[1] != '/' )
				{
					MI_ASSERT(mOpenCount>0);
					mOpenCount--;
				}
				scan++;

				if( *scan == '?' ) //Allow xml declarations
				{
					scan++;
				}
				else if ( scan[0] == '!' && scan[1] == '-' && scan[2] == '-' )
				{
					scan+=3;
					while ( *scan && *scan == ' ' )
						scan++;
					char *comment = scan, *comment_end = strstr(scan, "-->");
					if ( comment_end )
					{
						*comment_end = 0;
						scan = comment_end+3;
						if( !iface->processComment(comment) )
						{
							mError = "User aborted the parsing process";
							DEBUG_ALWAYS_ASSERT();
							return false;
						}
					}
					continue;
				}
				else if ( scan[0] == '!' ) //Allow doctype
				{
					scan++;

					//DOCTYPE syntax differs from usual XML so we parse it here

					//Read DOCTYPE
					const char *tag = "DOCTYPE";
					if( !strstr(scan, tag) )
					{
						mError = "Invalid DOCTYPE";
						DEBUG_ALWAYS_ASSERT();
						return false;
					}

					scan += strlen(tag);

					//Skip whites
					while(  CT_SOFT == mTypes[U(*scan)] )
						++scan;

					//Read rootElement
					const char *rootElement = scan;
					while( CT_DATA == mTypes[U(*scan)] )
						++scan;

					char *endRootElement = scan;

					//TODO: read remaining fields (fpi, uri, etc.)
					while( CT_END_OF_ELEMENT != mTypes[U(*scan++)] );

					*endRootElement = 0;

					if( !iface->processDoctype(rootElement, 0, 0, 0) )
					{
						mError = "User aborted the parsing process";
						DEBUG_ALWAYS_ASSERT();
						return false;
					}

					continue; //Restart loop
				}
			}


			if( *scan == '/' )
			{
				bool isError;
				scan = processClose(scan, iface, isError);
				if( !scan )
				{
					if ( isError )
					{
						DEBUG_ALWAYS_ASSERT();
						mError = "User aborted the parsing process";
					}
					return !isError;
				}
			}
			else
			{
				if( *scan == '?' )
					scan++;
				element = scan;
				MiI32 argc = 0;
				const char *argv[MAX_ATTRIBUTE];
				bool close;
				scan = nextSoftOrClose(scan, close);
				if( close )
				{
					char c = *(scan-1);
					if ( c != '?' && c != '/' )
					{
						c = '>';
					}
					*scan++ = 0;
					bool isError;
					scan = processClose(c, element, scan, argc, argv, iface, isError);
					if ( !scan )
					{
						if ( isError )
						{
							DEBUG_ALWAYS_ASSERT();
							mError = "User aborted the parsing process";
						}
						return !isError;
					}
				}
				else
				{
					if ( *scan == 0 )
					{
						return ret;
					}

					*scan = 0; // place a zero byte to indicate the end of the element name...
					scan++;

					while ( *scan )
					{
						scan = skipNextData(scan); // advance past any soft seperators (tab or space)

						if ( mTypes[U(*scan)] == CT_END_OF_ELEMENT )
						{
							char c = *scan++;
							if( '?' == c )
							{
								if( '>' != *scan ) //?>
								{
									DEBUG_ALWAYS_ASSERT();
									return false;
								}

								scan++;
							}
							bool isError;
							scan = processClose(c, element, scan, argc, argv, iface, isError);
							if ( !scan )
							{
								if ( isError )
								{
									DEBUG_ALWAYS_ASSERT();
									mError = "User aborted the parsing process";
								}
								return !isError;
							}
							break;
						}
						else
						{
							if( argc >= MAX_ATTRIBUTE )
							{
								DEBUG_ALWAYS_ASSERT();
								mError = "encountered too many attributes";
								return false;
							}
							argv[argc] = scan;
							scan = nextSep(scan);  // scan up to a space, or an equal
							if( *scan )
							{
								if( *scan != '=' )
								{
									*scan = 0;
									scan++;
									while ( *scan && *scan != '=' ) scan++;
									if ( *scan == '=' ) scan++;
								}
								else
								{
									*scan=0;
									scan++;
								}

								if( *scan ) // if not eof...
								{
									scan = skipNextData(scan);
									if( *scan == '"' )
									{
										scan++;
										argc++;
										argv[argc] = scan;
										argc++;
										while ( *scan && *scan != 34 ) scan++;
										if( *scan == '"' )
										{
											*scan = 0;
											scan++;
										}
										else
										{
											DEBUG_ALWAYS_ASSERT();
											mError = "Failed to find closing quote for attribute";
											return false;
										}
									}
									else
									{
										//mError = "Expected quote to begin attribute";
										//return false;
										// PH: let's try to have a more graceful fallback
										argc--;
										while(*scan != '/' && *scan != '>' && *scan != 0)
											scan++;
									}
								}
							} //if( *scan )
						} //if ( mTypes[*scan]
					} //if( close )
				} //if( *scan == '/'
			} //while( *scan )
		}

		if( mStackIndex )
		{
			DEBUG_ALWAYS_ASSERT();
			mError = "Invalid file format";
			return false;
		}

		return ret;
	}

	const char *getError(MiI32 &lineno)
	{
		const char *ret = mError;
		lineno = mLineNo;
		mError = 0;
		return ret;
	}

	virtual void release(void)
	{
		Callback *c = mCallback;	// get the user allocator interface
		MyFastXml *f = this;		// cast the this pointer
		f->~MyFastXml();			// explicitely invoke the destructor for this class
		c->fastxml_free(f);			// now free up the memory associated with it.
	}

private:

	MI_INLINE void releaseMemory(void)
	{
		mFileBuf = NULL;
		mCallback->fastxml_free(mReadBuffer);
		mReadBuffer = NULL;
		mLastReadLoc = 0;
		mStackIndex = 0;
		mReadBufferEnd = NULL;
		mOpenCount = 0;
		mError = NULL;
		for (MiU32 i=0; i<(mStackIndex+1); i++)
		{
			if ( mStackAllocated[i] )
			{
				mCallback->fastxml_free((void *)mStack[i]);
				mStackAllocated[i] = false;
			}
			mStack[i] = NULL;
		}
	}

	MI_INLINE char *nextSoft(char *scan)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT ) scan++;
		return scan;
	}

	MI_INLINE char *nextSoftOrClose(char *scan, bool &close)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT && *scan != '>' ) scan++;
		close = *scan == '>';
		return scan;
	}

	MI_INLINE char *nextSep(char *scan)
	{
		while ( *scan && mTypes[U(*scan)] != CT_SOFT && *scan != '=' ) scan++;
		return scan;
	}

	MI_INLINE char *skipNextData(char *scan)
	{
		// while we have data, and we encounter soft seperators or line feeds...
		while ( (*scan && mTypes[U(*scan)] == CT_SOFT) || mTypes[U(*scan)] == CT_END_OF_LINE )
		{
			if ( *scan == '\n' ) mLineNo++;
			scan++;
		}
		return scan;
	}

	void pushElement(const char *element)
	{
		MI_ASSERT( mStackIndex < MAX_STACK );
		if( mStackIndex < MAX_STACK )
		{
			if ( mStackAllocated[mStackIndex] )
			{
				mCallback->fastxml_free((void *)mStack[mStackIndex]);
				mStackAllocated[mStackIndex] = false;
			}
			mStack[mStackIndex++] = element;
		}
	}

	const char *popElement(void)
	{
		MI_ASSERT(mStackIndex>0);
		if ( mStackAllocated[mStackIndex] )
		{
			mCallback->fastxml_free((void*)mStack[mStackIndex]);
			mStackAllocated[mStackIndex] = false;
		}
		mStack[mStackIndex] = NULL;
		return mStackIndex ? mStack[--mStackIndex] : NULL;
	}

	static const unsigned MAX_STACK = 2048;

	char mTypes[256];

	MiFileBuf *mFileBuf;

	char			*mReadBuffer;
	char			*mReadBufferEnd;

	MiU32	mOpenCount;
	MiU32	mReadBufferSize;
	MiU32	mLastReadLoc;

	MiI32 mLineNo;
	const char *mError;
	MiU32 mStackIndex;
	const char *mStack[MAX_STACK+1];
	bool		mStreamFromMemory;
	bool		mStackAllocated[MAX_STACK+1];
	Callback	*mCallback;
#if DEBUG_LOG
	FILE	*mFph;
#endif
};

const char *getAttribute(const char *attr, MiI32 argc, const char **argv)
{
	MiI32 count = argc/2;
	for(MiI32 i = 0; i < count; ++i)
	{
		const char *key = argv[i*2], *value = argv[i*2+1];
		if( strcmp(key, attr) == 0 )
			return value;
	}

	return 0;
}

FastXml * createFastXml(FastXml::Callback *iface)
{
	MyFastXml *m = (MyFastXml *)iface->fastxml_malloc(sizeof(MyFastXml));
	if ( m )
	{
		new ( m ) MyFastXml(iface);
	}
	return static_cast< FastXml *>(m);
}

}	// end of namespace
