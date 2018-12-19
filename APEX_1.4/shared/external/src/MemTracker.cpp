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



#include "MemTracker.h"

#if PX_WINDOWS_FAMILY // only compile this source code for windows!

#define USE_HASH_MAP 1

// VC10 hash maps are abominably slow with iterator debug level == 0
//   Note: All STL containers are used internally, so their should not be any resultant conflict
#if (PX_VC == 10) && USE_HASH_MAP
#define OVERRIDE_ITERATOR_DEBUG_LEVEL 1
#else 
#define OVERRIDE_ITERATOR_DEBUG_LEVEL 0
#endif

#if OVERRIDE_ITERATOR_DEBUG_LEVEL
#undef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#define _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH 1
#endif

#include "htmltable.h"
#include "PxSimpleTypes.h"
#include "PxAssert.h"
#include <stdio.h>
#include <string>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <map>
#include <vector>
#include <windows.h>

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <hash_map>

#pragma warning(disable:4100 4996 4267 4239)

namespace MEM_TRACKER
{

class ContextType
{
public:
	ContextType(void)
	{
		mContext = NULL;
		mType = NULL;
		mAllocCount = 0;
		mAllocSize = 0;
	}
	ContextType(const char *context,const char *type,size_t size)
	{
		mContext = context;
		mType	= type;
		char scratch[512];
		sprintf_s(scratch,512,"PlatformAnalyzer/ContextType/%s/%s/Count(AllocCount)", mContext, mType );
		mAllocCountSpec = scratch;
		sprintf_s(scratch,512,"PlatformAnalyzer/ContextType/%s/%s/Size(AllocSize)", mContext, mType );
		mAllocSizeSpec = scratch;
		mAllocCount = 1;
		mAllocSize = size;
	}

	const	char	*mContext;
	const	char	*mType;
	uint32_t	mAllocCount;
	size_t			mAllocSize;
	std::string		mAllocCountSpec;
	std::string		mAllocSizeSpec;
};

class ByContextIndex
{
public:
	ByContextIndex(const char *context,const char *type)
	{
		mContext = context;
		mType = type;
	}

	const	char	*mContext;
	const	char	*mType;
};

class ByContextHasher
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;

	size_t operator()(const ByContextIndex &index) const
	{
		size_t hash1 = (size_t)index.mContext;
		size_t hash2 = (size_t)index.mType;
		size_t hash = hash1 ^ hash2;
		return hash;
	}

	bool operator()(const ByContextIndex &s1,const ByContextIndex &s2) const
	{
		if ( s1.mContext < s2.mContext ) return true;
		if ( s1.mContext > s2.mContext ) return false;
		return s1.mType < s2.mType;
	}
};

#if USE_HASH_MAP
typedef stdext::hash_map< ByContextIndex, ContextType, ByContextHasher > ByContextTypeHash;
#else
typedef std::map< ByContextIndex, ContextType, ByContextHasher > ByContextTypeHash;
#endif

static void getDateTime(char *date_time)
{
	time_t ltime;
	struct tm *today;
	_tzset();
	time( &ltime );
	today = localtime( &ltime );
	strftime( date_time, 128,"%A-%B-%d-%Y-%I-%M-%S-%p", today );
}

size_t getPow2(size_t s,size_t &p)
{
	size_t ret = 0;
	while ( s > p )
	{
		p = p<<1;
		ret++;
	}
	return ret;
}

class MemTrack
{
public:
  MemoryType  mType;
  size_t       mSize;
  size_t       mThreadId;
  const char *mContext;
  const char *mClassName;
  const char *mFileName;
  uint32_t       mLineNo;
  size_t       mAllocCount;
};

class ByType
{
public:
	ByType(size_t size)
	{
		mSize = size;
		mCount = 1;
	}
	ByType(void) { };
	size_t mSize;
	size_t mCount;
};

class BySource
{
public:
  BySource(void) { };

  BySource(const MemTrack &t)
  {
	mClassName = t.mClassName;
	mFileName  = t.mFileName;
	mLineNo    = t.mLineNo;
	mSize      = t.mSize;
	mLineNo    = t.mLineNo;
	mCount = 0;
  }

  const char *mClassName;
  const char *mFileName;
  size_t       mLineNo;
  size_t       mSize;
  size_t       mCount;
};

#if USE_HASH_MAP
typedef stdext::hash_map< size_t, ByType > ByTypeHash;
#else
typedef std::map< size_t, ByType > ByTypeHash;
#endif

class BySourceIndex
{
public:
	BySourceIndex(const char *fileName,uint32_t lineno)
	{
		mFileName = fileName;
		mLineNo = lineno;
	}

	const	char	*mFileName;
	uint32_t			mLineNo;
};

class BySourceHasher
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 8;

	size_t operator()(const BySourceIndex &index) const
	{
		size_t hash = (size_t)index.mFileName;
		hash = hash^index.mLineNo;
		return hash;
	}

	bool operator()(const BySourceIndex &s1,const BySourceIndex &s2) const
	{
		if ( s1.mFileName < s2.mFileName ) return true;
		if ( s1.mFileName > s2.mFileName ) return false;
		return s1.mLineNo < s2.mLineNo;
	}
};

#if USE_HASH_MAP
typedef stdext::hash_map< BySourceIndex, BySource, BySourceHasher > BySourceHash;
typedef stdext::hash_map< size_t , MemTrack > MemTrackHash;
#else
typedef std::map< BySourceIndex, BySource, BySourceHasher > BySourceHash;
typedef std::map< size_t , MemTrack > MemTrackHash;
#endif

typedef std::vector< MemTrack > MemTrackVector;

class ReportContext 
{
public:
  ReportContext(const char *context)
  {
	mContext = context;
  }
  void add(const MemTrack &t)
  {
	  assert(t.mAllocCount > 0 );
	mMemTracks.push_back(t);
  }

  const char * getContext(void) const { return mContext; };

  // first we generate a report based on type.
  void generateReport(nvidia::HtmlDocument *document,nvidia::HtmlTable *contextTable)
  {
	  unsigned int totalMemory = 0;
	  unsigned int totalAllocations = 0;
	  unsigned int typeCount = 0;
	  unsigned int sourceCount = 0;

	  {



		ByTypeHash bt;
		MemTrackVector::iterator i;
		for (i=mMemTracks.begin(); i!=mMemTracks.end(); i++)
		{
		  const MemTrack &t = (*i);
		  size_t hash = (size_t)t.mClassName;
		  ByTypeHash::iterator found = bt.find(hash);
		  if ( found == bt.end() )
		  {
			  ByType b(t.mSize);
			  bt[hash] = b;
		  }
		  else
		  {
			  ByType &b = (ByType &)(*found).second;
			  b.mSize+=t.mSize;
			  b.mCount++;
		  }
		}

		{
			typeCount = bt.size();

		  char scratch[512];
		  char date_time[512];
		  getDateTime(date_time);
		  sprintf(scratch,"Memory Usage by Class Name or Memory Type for Context: %s : %s", mContext, date_time );
		  nvidia::HtmlTable *table = document->createHtmlTable(scratch);
		  //                    1           2            3
		  table->addHeader("Memory/Type,Memory/Size,Allocation/Count");
		  table->addSort("Sorted By Memory Size",2,false,3,false);
		  for (ByTypeHash::iterator iter = bt.begin(); iter !=bt.end(); ++iter)
		  {
			ByType b = (*iter).second;
			table->addColumn( (const char *)(*iter).first );
			table->addColumn( (uint32_t)b.mSize );
			table->addColumn( (uint32_t)b.mCount );
			table->nextRow();
		  }
		  table->computeTotals();
		}
	  }

	  {

		BySourceHash bt;
		MemTrackVector::iterator i;
		for (i=mMemTracks.begin(); i!=mMemTracks.end(); i++)
		{
		  const MemTrack &t = (*i);
		  BySource b(t);
		  BySourceIndex index(b.mFileName,b.mLineNo);
		  BySourceHash::iterator found = bt.find(index);
		  if ( found == bt.end() )
		  {
			  b.mCount = 1;
			  bt[index] = b;
		  }
		  else
		  {
			  BySource &bs = (BySource &)(*found).second;
			  bs.mSize+=t.mSize;
			  bs.mCount++;
		  }
		}


		{
			sourceCount = bt.size();

		  char scratch[512];
		  char date_time[512];
		  getDateTime(date_time);
		  sprintf(scratch,"Memory Usage by Source File and Line Number for Context: %s : %s", mContext, date_time );
		  nvidia::HtmlTable *table = document->createHtmlTable(scratch);
		  //                    1           2            3           4            5
		  table->addHeader("Source/File,Line/Number,Memory/Type,Memory/Size,Allocation/Count");
		  table->addSort("Sorted By Memory Size",4,false,5,false);
		  table->excludeTotals(2);
		  for (BySourceHash::iterator i=bt.begin(); i!=bt.end(); ++i)
		  {
			BySource b = (*i).second;
			table->addColumn( b.mFileName );
			table->addColumn( (uint32_t)b.mLineNo );
			table->addColumn( b.mClassName );
			table->addColumn( (uint32_t)b.mSize );
			assert( b.mCount > 0 );
			table->addColumn( (uint32_t)b.mCount );
			table->nextRow();
			totalMemory+=b.mSize;
			totalAllocations+=b.mCount;
		  }
		  table->computeTotals();
		}
	  }


	  // Power of two, sizes 1-256
	{
		char scratch[512];
		char date_time[512];
		getDateTime(date_time);
		sprintf(scratch,"Power of Two Memory 1 to 256 bytes for Context: %s : %s", mContext, date_time );
		nvidia::HtmlTable *table = document->createHtmlTable(scratch);
		//                    1         2            3           4
		table->addHeader("Mem/Size,Total/Alloc,Alloc/Count,Fragment/Amount");
		table->addSort("Sorted By Memory Size",2,false,3,false);
		table->excludeTotals(1);

		// 0   1   2   3   4    5
		// 8, 16, 32, 64, 128, 256
		struct Power2
		{
			Power2(void)
			{
				memSize = 0;
				memTotal = 0;
				memCount = 0;
				fragmentTotal = 0;
			}
			unsigned int memSize;
			unsigned int memTotal;
			unsigned int memCount;
			unsigned int fragmentTotal;
		};

		Power2 p[6];

		MemTrackVector::iterator i;
		for (i=mMemTracks.begin(); i!=mMemTracks.end(); i++)
		{
			const MemTrack &t = (*i);
			if ( t.mSize <= 256 )
			{
				size_t p2=8;
				size_t index = getPow2(t.mSize,p2);
				p[index].memSize = p2;
				p[index].memTotal+=t.mSize;
				p[index].memCount++;
				p[index].fragmentTotal+=(p2-t.mSize);
			}
		}
		for (size_t i=0; i<6; i++)
		{
			Power2 &t = p[i];
			if ( t.memCount > 0 )
			{
				table->addColumn(t.memSize);
				table->addColumn(t.memTotal);
				table->addColumn(t.memCount);
				table->addColumn(t.fragmentTotal);
				table->nextRow();
			}
		}
		table->computeTotals();
	}

	// power of two allocations > 256 bytes
	{
		char scratch[512];
		char date_time[512];
		getDateTime(date_time);
		sprintf(scratch,"Power of Two Memory Greater than 256 bytes for Context: %s : %s", mContext, date_time );
		nvidia::HtmlTable *table = document->createHtmlTable(scratch);
		//                    1         2            3           4
		table->addHeader("Mem/Size,Total/Alloc,Alloc/Count,Fragment/Amount");
		table->addSort("Sorted By Memory Size",2,false,3,false);
		table->excludeTotals(1);

		struct Power2
		{
			Power2(void)
			{
				memSize = 0;
				memTotal = 0;
				memCount = 0;
				fragmentTotal = 0;
			}
			unsigned int memSize;
			unsigned int memTotal;
			unsigned int memCount;
			unsigned int fragmentTotal;
		};

		Power2 p[32];

		MemTrackVector::iterator i;
		for (i=mMemTracks.begin(); i!=mMemTracks.end(); i++)
		{
			const MemTrack &t = (*i);
			if ( t.mSize > 256 )
			{
				size_t p2=512;
				size_t index = getPow2(t.mSize,p2);
				p[index].memSize = p2;
				p[index].memTotal+=t.mSize;
				p[index].memCount++;
				p[index].fragmentTotal+=(p2-t.mSize);
			}
		}
		for (size_t i=0; i<32; i++)
		{
			Power2 &t = p[i];
			if ( t.memCount > 0 )
			{
				table->addColumn(t.memSize);
				table->addColumn(t.memTotal);
				table->addColumn(t.memCount);
				table->addColumn(t.fragmentTotal);
				table->nextRow();
			}
		}
		table->computeTotals();
	}


	// context summary..
	if ( contextTable )
	{
		contextTable->addColumn( mContext );
		contextTable->addColumn( totalMemory );
		contextTable->addColumn( totalAllocations );
		contextTable->addColumn( typeCount );
		contextTable->addColumn(sourceCount );
		contextTable->nextRow();
	}

  }

private:
  const char       *mContext;
  MemTrackVector    mMemTracks;
};

#if USE_HASH_MAP
typedef stdext::hash_map< size_t , ReportContext * > ReportContextHash;
#else
typedef std::map< size_t , ReportContext * > ReportContextHash;
#endif

class MyMemTracker : public MemTracker
{
public:

	MyMemTracker(void)
	{
		mSingleThreaded = false;
	  mFrameNo = 1;
	  mAllocCount = 0;
	  mAllocSize = 0;
	  mAllocFrameCount = 0;
	  mFreeFrameCount = 0;
	  mAllocFrameSize = 0;
	  mFreeFrameSize = 0;
	  mDocument = 0;
	  mFrameSummary = 0;
	  mDetailed = 0;
	 // nvidia::HtmlTableInterface *h = nvidia::getHtmlTableInterface();
	 // if ( h )
	 // {
		//mDocument = h->createHtmlDocument("MemTracker");
	 // }

	}

	virtual ~MyMemTracker(void)
	{
	  if ( mDocument )
	  {
		nvidia::HtmlTableInterface *h = nvidia::getHtmlTableInterface();
		h->releaseHtmlDocument(mDocument);
	  }
	}

  virtual void trackAlloc(size_t threadId,void *mem,size_t size,MemoryType type,const char *context,const char *className,const char *fileName,uint32_t lineno)
  {
	if ( mem )
	{
		addContextType(context,className,size);

	  mAllocCount++;
	  mAllocSize+=size;

	  mAllocFrameCount++;
	  mAllocFrameSize+=size;

	  size_t hash = (size_t) mem;
	  MemTrack t;
	  t.mType = type;
	  t.mSize = size;
	  t.mThreadId  = threadId;
	  t.mContext   = context;
	  t.mClassName = className;
	  t.mFileName  = fileName;
	  t.mLineNo    = lineno;
	  t.mAllocCount = 1;
	  MemTrackHash::iterator found = mMemory.find(hash);
	  if ( found != mMemory.end()  )
	  {
		  PX_ALWAYS_ASSERT();
		  const MemTrack &previous = (*found).second;
		  printf("Prev: %s\r\n", previous.mClassName );
		  PX_UNUSED(previous);
	  }
	  // track which allocation number this one was.
	  {
		BySource b(t);
		b.mCount = 1;
		BySourceIndex index(b.mFileName,b.mLineNo);
		BySourceHash::iterator found = mSourceHash.find(index);
		if ( found == mSourceHash.end() )
		{
			mSourceHash[index] = b;
		}
		else
		{
			BySource bs = (*found).second;
			bs.mCount++;
			t.mAllocCount = bs.mCount;
			mSourceHash[index] = bs;
		}
	  }
	  if ( mDetailed )
	  {
		mDetailed->addColumnHex( (size_t)mem );
		switch ( type )
		{
		  case MT_NEW:
			  mDetailed->addColumn("NEW");
			  break;
		  case MT_NEW_ARRAY:
			  mDetailed->addColumn("NEW_ARRAY");
			  break;
		  case MT_MALLOC:
			  mDetailed->addColumn("MALLOC");
			  break;
		  case MT_GLOBAL_NEW:
			  mDetailed->addColumn("GLOBAL_NEW");
			  break;
		  case MT_GLOBAL_NEW_ARRAY:
			  mDetailed->addColumn("GLOBAL_NEW_ARRAY");
			  break;
		  default:
			  PX_ALWAYS_ASSERT();
			  mDetailed->addColumn("ERROR");
			  break;
		}
		mDetailed->addColumn((uint32_t)size);
		mDetailed->addColumn((uint32_t)t.mAllocCount);
		mDetailed->addColumnHex(threadId);
		mDetailed->addColumn(context);
		mDetailed->addColumn(className);
		mDetailed->addColumn(fileName);
		mDetailed->addColumn((uint32_t)lineno);
		mDetailed->nextRow();
	  }

	  mMemory[hash] = t;

	  PX_ASSERT( mAllocCount == mMemory.size() );
	}
  }

  virtual void trackRealloc(size_t threadId,
	  void *oldMem,
	  void *newMem,
	  size_t newSize,
	  const char *context,
	  const char *className,
	  const char *fileName,
	  uint32_t lineno)
  {
	  TrackInfo info;
	  bool found = trackInfo(oldMem,info);
	  PX_ASSERT(found);
	  if ( found )
	  {
		PX_ASSERT( info.mType == MT_MALLOC );
		trackFree(threadId,oldMem,MT_FREE,context,fileName,lineno);
		trackAlloc(threadId,newMem,newSize,info.mType,context,className,fileName,lineno);
	  }
  }

  virtual const char * typeString(MemoryType type)
  {
	  const char *ret = "unknown";
	  switch ( type )
	  {
	  case MT_NEW:
		  ret = "new operator";
		  break;
	  case MT_NEW_ARRAY:
		  ret = "new[] array operator";
		  break;
	  case MT_MALLOC:
		  ret = "malloc";
		  break;
	  case MT_FREE:
		  ret = "free";
		  break;
	  case MT_DELETE:
		  ret = "delete operator";
		  break;
	  case MT_DELETE_ARRAY:
		  ret = "delete[] array operator";
		  break;
	  case MT_GLOBAL_NEW:
		  ret = "global new";
		  break;
	  case MT_GLOBAL_NEW_ARRAY:
		  ret = "global new[] array";
		  break;
	  case MT_GLOBAL_DELETE:
		  ret = "global delete";
		  break;
	  case MT_GLOBAL_DELETE_ARRAY:
		  ret = "global delete array";
		  break;
	  }
	  return ret;
  }


  virtual const char * trackValidateFree(size_t threadId,void *mem,MemoryType type,const char *context,const char *fileName,uint32_t lineno)
  {
	  const char *ret = NULL;
	  if ( mem )
	  {
		  char scratch[1024];
		  scratch[0] = 0;

		  size_t hash = (size_t) mem;
		  MemTrackHash::iterator found = mMemory.find(hash);
		  if ( found == mMemory.end() )
		  {
			  sprintf_s(scratch,1024,"Error! Tried to free memory never tracked.  Source: %s : Line: %d\r\n", fileName, lineno);
			  PX_ALWAYS_ASSERT();
		  }
		  else
		  {
			  MemTrack &t = (MemTrack &)(*found).second;

			  switch ( type )
			  {
			  case MT_DELETE:
				  if ( t.mType != MT_NEW )
				  {
					  sprintf_s(scratch,1024,"Error: Allocated with %s but deallocated with the delete operator\r\n", typeString(t.mType));
				  }
				  break;
			  case MT_DELETE_ARRAY:
				  if ( t.mType != MT_NEW_ARRAY )
				  {
					  sprintf_s(scratch,1024,"Error: Allocated with %s but deallocated with the delete array operator.\r\n", typeString(t.mType));
				  }
				  break;
			  case MT_FREE:
				  if ( t.mType != MT_MALLOC )
				  {
					sprintf_s(scratch,1024,"Error: Allocated with %s but deallocated with malloc.\r\n", typeString(t.mType));
				  }
				  break;
			  case MT_GLOBAL_DELETE:
				  if ( t.mType != MT_GLOBAL_NEW )
				  {
					sprintf_s(scratch,1024,"Error: Allocated with %s but deallocated with global delete.\r\n", typeString(t.mType));
				  }
				  break;
			  case MT_GLOBAL_DELETE_ARRAY:
				  if ( t.mType != MT_GLOBAL_NEW_ARRAY )
				  {
					sprintf_s(scratch,1024,"Error: Allocated with %s but deallocated with global delete array.\r\n", typeString(t.mType));
				  }
				  break;
			  default:
				  sprintf_s(scratch,1024,"Invalid memory type encountered.  Data corrupted!?\r\n");
				  break;
			  }
			  if ( t.mThreadId != threadId && mSingleThreaded )
			  {
				  sprintf_s(scratch,1024,"Memory de-allocated from a different thread than it was allocated from!\r\n");
			  }
			  if ( !context )
			  {
				  sprintf_s(scratch,1024,"Null context!\r\n");
			  }
			  if ( !t.mContext )
			  {
				  sprintf_s(scratch,1024,"Original memory block allocated with a null context, or the data is corrupted!\r\n");
			  }
			  if ( context != t.mContext )
			  {
				  sprintf_s(scratch,1024,"Memory is de-allocated from a different context.  Allocated from (%s) deallocated from (%s)\r\n", context, t.mContext );
			  }
		  }
		  if ( scratch[0] )
		  {
			  mErrorMessage = scratch;
			  ret = mErrorMessage.c_str();
		  }
	  }
	  return ret;
  }

  void addContextType(const char *context,const char *type,size_t size)
  {
	  ByContextIndex index(context,type);
	  ByContextTypeHash::iterator found = mContextType.find(index);
	  if ( found != mContextType.end() )
	  {
		  ContextType &c = (ContextType &)(*found).second;
		  PX_ASSERT( c.mContext == context );
		  PX_ASSERT( c.mType == type );
		  c.mAllocCount++;
		  c.mAllocSize+=size;
	  }
	  else
	  {
		  ContextType c(context,type,size);
		  mContextType[index] = c;
	  }
#if 0
	  int foundCount=0;
	  {
		  for (ByContextTypeHash::iterator i=mContextType.begin(); i!=mContextType.end(); ++i)
		  {
			  const ByContextIndex &index = (*i).first;
			  if ( index.mContext == context && index.mType == type )
			  {
				  foundCount++;
			  }
		  }
	  }
	  PX_ASSERT( foundCount == 1 );
#endif
  }

  void removeContextType(const char *context,const char *type,size_t size)
  {
	  ByContextIndex index(context,type);
	  ByContextTypeHash::iterator found = mContextType.find(index);
	  if ( found  != mContextType.end() )
	  {
		  ContextType &c = (ContextType &)(*found).second;
		  c.mAllocCount--;
		  c.mAllocSize-=size;
	  }
	  else
	  {
		  PX_ALWAYS_ASSERT();
	  }


  }


  virtual void trackFree(size_t threadId,void *mem,MemoryType type,const char *context,const char *fileName,uint32_t lineno)
  {
	if ( mem )
	{
	  size_t hash = (size_t) mem;
	  MemTrackHash::iterator found = mMemory.find(hash);
	  if ( found == mMemory.end() )
	  {
		  PX_ALWAYS_ASSERT();
	  }
	  else
	  {
		  const MemTrack &t = (*found).second;
		  removeContextType(t.mContext,t.mClassName,t.mSize);
		  if ( mDetailed )
		  {
			mDetailed->addColumnHex( (size_t) mem );
			switch ( type )
			{
			  case MT_FREE:
				  mDetailed->addColumn("FREE");
				  break;
			  case MT_DELETE:
				  mDetailed->addColumn("DELETE");
				  break;
			  case MT_DELETE_ARRAY:
				  mDetailed->addColumn("DELETE_ARRAY");
				  break;
			  case MT_GLOBAL_DELETE:
				  mDetailed->addColumn("GLOBAL_DELETE");
				  break;
			  case MT_GLOBAL_DELETE_ARRAY:
				  mDetailed->addColumn("GLOBAL_DELETE_ARRAY");
				  break;
			  default:
				  printf("Invalid memory allocation type! %s : %d\r\n", fileName, lineno );
				  mDetailed->addColumn("INVALID ALLOC TYPE");
				  break;
			}
			mDetailed->addColumn((uint32_t)t.mSize);
			mDetailed->addColumn((uint32_t)t.mAllocCount);
			mDetailed->addColumnHex(threadId);
			mDetailed->addColumn(context);
			mDetailed->addColumn(t.mClassName);
			mDetailed->addColumn(fileName);
			mDetailed->addColumn((uint32_t)lineno);
			mDetailed->nextRow();
		  }

		  mAllocCount--;
		  mAllocSize-=t.mSize;

		  mFreeFrameCount++;
		  mFreeFrameSize+=t.mSize;

		  mMemory.erase(hash);

		  PX_ASSERT( mAllocCount == mMemory.size() );
	  }
	}
  }

  void trackFrame(void)
  {
	mFrameNo++;
	if ( mAllocFrameCount || mFreeFrameCount )
	{
	  if ( mFrameSummary )
	  {
		mFrameSummary->addColumn((uint32_t)mFrameNo);
		mFrameSummary->addColumn((uint32_t)mAllocCount);
		mFrameSummary->addColumn((uint32_t)mAllocSize);
		mFrameSummary->addColumn((uint32_t)mAllocFrameCount);
		mFrameSummary->addColumn((uint32_t)mAllocFrameSize);
		mFrameSummary->addColumn((uint32_t)mFreeFrameCount);
		mFrameSummary->addColumn((uint32_t)mFreeFrameSize);
		mFrameSummary->addColumn((uint32_t)(mAllocFrameCount-mFreeFrameCount));
		mFrameSummary->addColumn((uint32_t)(mAllocFrameSize-mFreeFrameSize));
		mFrameSummary->nextRow();
	  }

	  mAllocFrameCount = 0;
	  mAllocFrameSize  = 0;
	  mFreeFrameCount = 0;
	  mFreeFrameSize = 0;

	  if ( mDetailed )
	  {
		char scratch[512];
		sprintf(scratch,"New Frame %d", (int)mFrameNo );
		mDetailed->addColumn(scratch);
		mDetailed->addColumn((uint32_t)mAllocSize);
		mDetailed->nextRow();
	  }


	}
  }

  virtual void releaseReportMemory(void *mem)
  {
	  ::free(mem);
  }

  void *generateReport(MemoryReportFormat format,const char *fname,uint32_t &saveLen,bool reportAllLeaks)
  {
	  void *ret = NULL;
	  saveLen = 0;

	//
	nvidia::HtmlTable *contextTable = NULL;
	{
		char scratch[512];
		char date_time[512];
		getDateTime(date_time);
		sprintf(scratch,"Summary Report for All Contexts : %s", date_time);
		contextTable = mDocument->createHtmlTable(scratch);
		//                    1              2           3
		contextTable->addHeader("Memory/Context,Total/Memory,Alloc/Count,Type/Count,Source/Count");
		contextTable->addSort("Sorted By Total Memory",2,true,3,true);

	}
	ReportContextHash rchash;
	{
	ReportContext *current = 0;
	  
	  for (MemTrackHash::iterator i=mMemory.begin(); i!=mMemory.end(); ++i)
	  {
		  MemTrack t = (*i).second;
		  if ( (current == 0) || current->getContext() != t.mContext )
		  {
			  size_t hash = (size_t)t.mContext;
			  ReportContextHash::iterator found = rchash.find(hash);
			  if ( found == rchash.end() )
			  {
				  current = new ReportContext(t.mContext);
				  rchash[hash] = current;
			  }
			  else
			  {
				  current = (*found).second;
			  }
		  }
		  current->add(t);
	  }
	  //
	  if ( reportAllLeaks )
	  {
		char scratch[512];
		char date_time[512];
		getDateTime(date_time);
		sprintf(scratch,"Memory Leaks in order of allocation : %s", date_time);

		nvidia::HtmlTable *table = mDocument->createHtmlTable(scratch);
		//                    1              2           3          4       5      6            7         8
		table->addHeader("Memory/Address,Alloc/Count,Alloc/Size,ThreadId,Context,Class/Type,Source/File,Lineno");
		table->addSort("Sorted By Source File and Line Number",7,true,8,true);

		table->excludeTotals(2);
		table->excludeTotals(8);
		
		for (MemTrackHash::iterator i=mMemory.begin();  i!=mMemory.end(); ++i)
		{
			MemTrack t = (*i).second;
			table->addColumnHex( (size_t)(*i).first ); // memory address
			table->addColumn((uint32_t) t.mAllocCount ); // allocation count.
			table->addColumn((uint32_t) t.mSize );       // size of allocation.
			table->addColumnHex( t.mThreadId ); // thread id
			table->addColumn( t.mContext );    // context
			table->addColumn( t.mClassName );  //
			table->addColumn( t.mFileName );
			table->addColumn((uint32_t) t.mLineNo );
			table->nextRow();
		  }
		  table->computeTotals();
		}
	  }
	  //
	  {
		  
		  for (ReportContextHash::iterator i=rchash.begin(); i!=rchash.end(); ++i)
		  {
			  ReportContext *rc = (*i).second;
			  rc->generateReport(mDocument,contextTable);
			  delete rc;
		  }
	  }
	//
	if ( mFrameSummary )
	{
	  mFrameSummary->excludeTotals(1);
	  mFrameSummary->excludeTotals(2);
	  mFrameSummary->excludeTotals(3);
	  mFrameSummary->excludeTotals(8);
	  mFrameSummary->excludeTotals(9);
	  mFrameSummary->computeTotals();
	}

	contextTable->computeTotals();

	nvidia::HtmlSaveType saveType = nvidia::HST_SIMPLE_HTML;
	switch ( format )
	{
		case MRF_SIMPLE_HTML:       // just a very simple HTML document containing the tables.
			saveType = nvidia::HST_SIMPLE_HTML;
			break;
		case MRF_CSV:               // Saves the Tables out as comma seperated value text
			saveType = nvidia::HST_CSV;
			break;
		case MRF_TEXT:              // Saves the tables out in human readable text format.
			saveType = nvidia::HST_TEXT;
			break;
		case MRF_TEXT_EXTENDED:     // Saves the tables out in human readable text format, but uses the MS-DOS style extended ASCII character set for the borders.
			saveType = nvidia::HST_TEXT_EXTENDED;
			break;
	}

	size_t len;
	const char *data = mDocument->saveDocument(len,saveType);
	if ( data )
	{
		ret = ::malloc(len);
		memcpy(ret,data,len);
		saveLen = len;
		mDocument->releaseDocumentMemory(data);
	}
	return ret;
  }


	virtual void usage(void)
	{
		printf("On Frame Number: %d has performed %d memory allocations for a total %d bytes of memory.\r\n", (int)mFrameNo, (int)mAllocCount, (int)mAllocSize );
	}


	virtual size_t detectLeaks(size_t &acount)
	{
		acount = mAllocCount;
		return mAllocSize;
	}


  virtual void setLogLevel(bool logEveryAllocation,bool logEveryFrame,bool verifySingleThreaded)
  {
	  mSingleThreaded = verifySingleThreaded;
	mLogEveryAllocation = logEveryAllocation;
	mLogEveryFrame      = logEveryFrame;
	if ( mDocument )
	{
	  if ( mLogEveryFrame && mFrameSummary == 0 )
	  {
		  char date_time[512];
		  getDateTime(date_time);
		  char scratch[1024];
		  sprintf(scratch,"Per Frame Memory Usage Summary : %s ", date_time);

		  mFrameSummary = mDocument->createHtmlTable(scratch);
		//                             1           2                  3               4                  5               6               7             8               9
		mFrameSummary->addHeader("Frame/Number,Total/Alloc Count,Total/Alloc Mem,Frame/Alloc Count,Frame/Alloc Mem,Frame/Free Count,Frame/Free Mem,Frame/Delta Count,Frame/Deleta Mem");
	  }
	  if ( mLogEveryAllocation && mDetailed == 0 )
	  {
		  char date_time[512];
		  getDateTime(date_time);
		  char scratch[2048];
		  sprintf(scratch,"Detailed Memory Usage Report : %s ",date_time);
		  mDetailed = mDocument->createHtmlTable(scratch);
		//                      1      2     3      4           5     6             7         8
		mDetailed->addHeader("Memory,Event,Size,Alloc Count,ThreadId,Context,Class or Type,Source File,Line Number");
		mDetailed->setOrder(1000); // make sure it displays this last!
	  }
	}
  }


  virtual bool trackInfo(const void *mem,TrackInfo &info)
	{
		bool ret = false;

		if ( mem )
		{
		  size_t hash = (size_t) mem;
		  MemTrackHash::iterator found = mMemory.find(hash);
		  if ( found == mMemory.end() )
		  {
			  printf("Error! Tried to get information for memory never tracked.\r\n");
		  }
		  else
		  {
			  MemTrack &t = (MemTrack &)(*found).second;
			  info.mMemory 		= mem;
			  info.mType		= t.mType;
			  info.mSize   		= t.mSize;
			  info.mContext 	= t.mContext;
			  info.mClassName 	= t.mClassName;
			  info.mFileName 	= t.mFileName;
			  info.mLineNo 		= t.mLineNo;
			  info.mAllocCount 	= t.mAllocCount;
			  ret = true;
		  }
		}
		return ret;
	}


private:

  bool           mLogEveryAllocation;
  bool           mLogEveryFrame;

  size_t         mFrameNo;
  MemTrackHash  mMemory;

  size_t         mAllocFrameCount;
  size_t         mFreeFrameCount;

  size_t         mAllocFrameSize;
  size_t         mFreeFrameSize;

  size_t         mAllocCount;
  size_t         mAllocSize;
  bool	mSingleThreaded;

  nvidia::HtmlTable    *mFrameSummary;
  nvidia::HtmlTable    *mDetailed;
  nvidia::HtmlDocument *mDocument;
  BySourceHash           mSourceHash;
  ByContextTypeHash		mContextType;

  std::string	mErrorMessage;
};

MemTracker *createMemTracker(void)
{
	MyMemTracker *m = new MyMemTracker;
	return static_cast< MemTracker *>(m);
}

void        releaseMemTracker(MemTracker *mt)
{
	MyMemTracker *m = static_cast< MyMemTracker *>(mt);
	delete m;
}
};

#endif