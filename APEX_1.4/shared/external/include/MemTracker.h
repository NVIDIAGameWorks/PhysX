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



#ifndef MEM_TRACKER_H

#define MEM_TRACKER_H

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"

#if PX_WINDOWS_FAMILY // only compile this source code for windows!!

namespace MEM_TRACKER
{

/**
\brief The layout format for the memory report.
*/
enum MemoryReportFormat
{
	MRF_SIMPLE_HTML,       // just a very simple HTML document containing the tables.
	MRF_CSV,               // Saves the Tables out as comma seperated value text
	MRF_TEXT,              // Saves the tables out in human readable text format.
	MRF_TEXT_EXTENDED,     // Saves the tables out in human readable text format, but uses the MS-DOS style extended ASCII character set for the borders.
};

/**
\brief This enumeration indicates the type of memory allocation that is being performed.
*/
enum MemoryType
{
	MT_NEW,						// Captured new operator
	MT_NEW_ARRAY,				// Captured new array operator
	MT_MALLOC,					// Standard heap allocation
	MT_FREE,					// Standard heap free
	MT_DELETE,					// Captured delete operator
	MT_DELETE_ARRAY,			// Captured array delete
	MT_GLOBAL_NEW,				// Allocation via Global new
	MT_GLOBAL_NEW_ARRAY,		// Allocation via global new array
	MT_GLOBAL_DELETE,			// Deallocation via global delete
	MT_GLOBAL_DELETE_ARRAY,		// Deallocation via global delete array
};

/**
\brief This data structure is used to return the current state of a particular block of allocated memory.
*/
struct TrackInfo
{
	const void		*mMemory;		// Address of memory
	MemoryType		 mType;			// Type of allocation
	size_t			mSize;			// Size of the memory allocation
	const char		*mContext;		// The context of the memory allocation.
	const char		*mClassName;	// The class or type name of this allocation.
	const char		*mFileName;		// Source code file name where this allocation occured
	uint32_t	 mLineNo;		// Source code line number where this allocation occured
	size_t			mAllocCount; 	// Indicates which time this allocation occured at this particular source file and line number.
};


class MemTracker
{
public:

  virtual void trackAlloc(size_t threadId,
                          void *mem,
                          size_t size,
                          MemoryType type,
                          const char *context,
                          const char *className,
                          const char *fileName,
                          uint32_t lineno) = 0;

  virtual void trackRealloc(size_t threadId,
	                          void *oldMem,
 	                          void *newMem,
	                          size_t newSize,
	                          const char *context,
	                          const char *className,
	                          const char *fileName,
	                          uint32_t lineno) = 0;

  virtual void trackFree(size_t threadId,
                        void *mem,
                        MemoryType type,
                        const char *context,
                        const char *fileName,uint32_t lineno) = 0;

  virtual const char * trackValidateFree(size_t threadId,
	  void *mem,
	  MemoryType type,
	  const char *context,
	  const char *fileName,uint32_t lineno) = 0;


   virtual void trackFrame(void) = 0;


   virtual bool trackInfo(const void *mem,TrackInfo &info)  = 0;


   virtual void *generateReport(MemoryReportFormat format,const char *fname,uint32_t &saveLen,bool reportAllLeaks) = 0;
   virtual void releaseReportMemory(void *mem) = 0;

    virtual void usage(void) = 0;
    virtual size_t detectLeaks(size_t &acount) = 0;
    virtual void setLogLevel(bool logEveryAllocation,bool logEveyFrame,bool verifySingleThreaded) = 0;

};

MemTracker *createMemTracker(void);
void        releaseMemTracker(MemTracker *mt);

};

#endif

#endif
