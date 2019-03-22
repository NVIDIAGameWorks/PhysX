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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#include "foundation/PxPreprocessor.h"

#if PX_UWP

#include "foundation/PxAssert.h"

#include "windows/PsWindowsInclude.h"
#include "PsThread.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;

// an exception for setting the thread name in microsoft debuggers
#define NS_MS_VC_EXCEPTION 0x406D1388

namespace physx
{
namespace shdfnd
{

namespace
{

// struct for naming a thread in the debugger
#pragma pack(push, 8)

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;     // Must be 0x1000.
	LPCSTR szName;    // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;

#pragma pack(pop)

class _ThreadImpl
{
  public:
	enum State
	{
		NotStarted,
		Started,
		Stopped
	};

	PX_FORCE_INLINE _ThreadImpl(ThreadImpl::ExecuteFn fn_, void* arg_);

	IAsyncAction ^ asyncAction;
	HANDLE signal;
	LONG quitNow; // Should be 32bit aligned on SMP systems.
	State state;
	DWORD threadID;

	ThreadImpl::ExecuteFn fn;
	void* arg;
};

PX_FORCE_INLINE _ThreadImpl::_ThreadImpl(ThreadImpl::ExecuteFn fn_, void* arg_)
: asyncAction(nullptr), quitNow(0), state(_ThreadImpl::NotStarted), threadID(0xffffFFFF), fn(fn_), arg(arg_)
{
	signal = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
}

_ThreadImpl* getThread(ThreadImpl* impl)
{
	return reinterpret_cast<_ThreadImpl*>(impl);
}

DWORD WINAPI PxThreadStart(LPVOID arg)
{	
	_ThreadImpl* impl = getThread((ThreadImpl*)arg);

	// run either the passed in function or execute from the derived class (Runnable).
	if(impl->fn)
		(*impl->fn)(impl->arg);
	else if(impl->arg)
		((Runnable*)impl->arg)->execute();
	return 0;
}
}

static const uint32_t gSize = sizeof(_ThreadImpl);
uint32_t ThreadImpl::getSize()
{
	return gSize;
}

ThreadImpl::Id ThreadImpl::getId()
{
	return static_cast<Id>(GetCurrentThreadId());
}

ThreadImpl::ThreadImpl()
{
	PX_PLACEMENT_NEW(getThread(this), _ThreadImpl)(NULL, NULL);
}

ThreadImpl::ThreadImpl(ExecuteFn fn, void* arg, const char*)
{
	PX_PLACEMENT_NEW(getThread(this), _ThreadImpl)(fn, arg);
	start(0, NULL);
}

ThreadImpl::~ThreadImpl()
{
	if(getThread(this)->state == _ThreadImpl::Started)
		kill();
	if(getThread(this)->asyncAction)
		getThread(this)->asyncAction->Close();
	CloseHandle(getThread(this)->signal);
}

void ThreadImpl::start(uint32_t /*stackSize*/, Runnable* runnable)
{
	if(getThread(this)->state != _ThreadImpl::NotStarted)
		return;
	getThread(this)->state = _ThreadImpl::Started;

	if(runnable && !getThread(this)->arg && !getThread(this)->fn)
		getThread(this)->arg = runnable;

	// run lambda
	auto workItemDelegate = [this](IAsyncAction ^ workItem)
	{
		PxThreadStart((LPVOID) this); // function to run async
	};

	// onComplete lambda
	auto completionDelegate = [this](IAsyncAction ^ action, AsyncStatus /*status*/)
	{
		switch(action->Status)
		{
		case AsyncStatus::Started:
			break;
		case AsyncStatus::Completed:
		case AsyncStatus::Canceled:
		case AsyncStatus::Error:
			SetEvent(getThread(this)->signal);
			break;
		}
	};

	// thread pool work item, can run on any thread
	auto workItemHandler = ref new WorkItemHandler(workItemDelegate, CallbackContext::Any);
	// thread status handler (signal complete), can run on any thread
	auto completionHandler = ref new AsyncActionCompletedHandler(completionDelegate, Platform::CallbackContext::Any);

	// run with normal priority, time sliced
	getThread(this)->asyncAction =
	    ThreadPool::RunAsync(workItemHandler, WorkItemPriority::Normal, WorkItemOptions::TimeSliced);
	getThread(this)->asyncAction->Completed = completionHandler;
}

void ThreadImpl::signalQuit()
{
	InterlockedIncrement(&(getThread(this)->quitNow));
}

bool ThreadImpl::waitForQuit()
{
	if(getThread(this)->state == _ThreadImpl::NotStarted)
		return false;

	WaitForSingleObjectEx(getThread(this)->signal, INFINITE, false);

	return true;
}

bool ThreadImpl::quitIsSignalled()
{
	return InterlockedCompareExchange(&(getThread(this)->quitNow), 0, 0) != 0;
}

void ThreadImpl::quit()
{
	getThread(this)->state = _ThreadImpl::Stopped;
}

void ThreadImpl::kill()
{
	if(getThread(this)->state == _ThreadImpl::Started && getThread(this)->asyncAction)
	{
		getThread(this)->asyncAction->Cancel();
		InterlockedIncrement(&(getThread(this)->quitNow));
	}
	getThread(this)->state = _ThreadImpl::Stopped;
}

void ThreadImpl::sleep(uint32_t ms)
{
	// find something better than this:
	if(ms == 0)
		yield();
	else
	{
		HANDLE handle = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
		WaitForSingleObjectEx(handle, ms, false);
		CloseHandle(handle);
	}
}

void ThreadImpl::yield()
{
	YieldProcessor();
}

uint32_t ThreadImpl::setAffinityMask(uint32_t /*mask*/)
{
	return 0;
}

void ThreadImpl::setName(const char* name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = getThread(this)->threadID;
	info.dwFlags = 0;

	// C++ Exceptions are disabled for this project, but SEH is not (and cannot be)
	// http://stackoverflow.com/questions/943087/what-exactly-will-happen-if-i-disable-c-exceptions-in-a-project
	__try
	{
		RaiseException(NS_MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// this runs if not attached to a debugger (thus not really naming the thread)
	}
}

void ThreadImpl::setPriority(ThreadPriority::Enum /*prio*/)
{
	// AFAIK this can only be done at creation in Metro Apps
}

ThreadPriority::Enum ThreadImpl::getPriority(Id /*threadId*/)
{
	// see setPriority
	return ThreadPriority::eNORMAL;
}

uint32_t TlsAlloc()
{
	DWORD rv = ::TlsAlloc();
	PX_ASSERT(rv != TLS_OUT_OF_INDEXES);
	return (uint32_t)rv;
}

void TlsFree(uint32_t index)
{
	::TlsFree(index);
}

void* TlsGet(uint32_t index)
{
	return ::TlsGetValue(index);
}

size_t TlsGetValue(uint32_t index)
{
	return reinterpret_cast<size_t>(::TlsGetValue(index));
}

uint32_t TlsSet(uint32_t index, void* value)
{
	return uint32_t(::TlsSetValue(index, value));
}

uint32_t TlsSetValue(uint32_t index, size_t value)
{
	return uint32_t(::TlsSetValue(index, reinterpret_cast<void*>(value)));
}


uint32_t ThreadImpl::getDefaultStackSize()
{
	return 1048576;
};

uint32_t ThreadImpl::getNbPhysicalCores()
{
	SYSTEM_INFO info;
	GetNativeSystemInfo(&info);
	return info.dwNumberOfProcessors;
}

} // namespace shdfnd
} // namespace physx

#endif // PX_UWP
