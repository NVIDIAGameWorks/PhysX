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

// Helper for profiling with NVIDIA Nsight tools.
//
// If Nsight is installed, find nvToolsExt headers and libs path in 
// environment variable NVTOOLSEXT_PATH.

// Define NVTX_ENABLE before including this header to enable nvToolsExt markers and ranges.
//#define NVTX_ENABLE



// Emulate stdint.h header.
#ifndef __stdint_h__
#define __stdint_h__
typedef            char    int8_t;
typedef unsigned   char   uint8_t;
typedef            short  int16_t;
typedef unsigned   short uint16_t;
typedef            long   int32_t;
typedef unsigned   long  uint32_t;
typedef          __int64  int64_t;
typedef unsigned __int64 uint64_t;
#endif // file guard



#ifdef NVTX_ENABLE

#define NVTX_STDINT_TYPES_ALREADY_DEFINED
#include "nvToolsExt.h"

#ifdef _WIN64
#pragma comment( lib, "nvToolsExt64_1.lib")
#else
#pragma comment( lib, "nvToolsExt32_1.lib")
#endif

#define NVTX_MarkEx nvtxMarkEx
#define NVTX_MarkA nvtxMarkA
#define NVTX_MarkW nvtxMarkW
#define NVTX_RangeStartEx nvtxRangeStartEx
#define NVTX_RangeStartA nvtxRangeStartA
#define NVTX_RangeStartW nvtxRangeStartW
#define NVTX_RangeEnd nvtxRangeEnd
#define NVTX_RangePushEx nvtxRangePushEx
#define NVTX_RangePushA nvtxRangePushA
#define NVTX_RangePushW nvtxRangePushW
#define NVTX_RangePop nvtxRangePop
#define NVTX_NameOsThreadA nvtxNameOsThreadA
#define NVTX_NameOsThreadW nvtxNameOsThreadW

#else

struct nvtxEventAttributes_t {};
typedef uint64_t nvtxRangeId_t;

#define NVTX_MarkEx __noop
#define NVTX_MarkA __noop
#define NVTX_MarkW __noop
#define NVTX_RangeStartEx __noop
#define NVTX_RangeStartA __noop
#define NVTX_RangeStartW __noop
#define NVTX_RangeEnd __noop
#define NVTX_RangePushEx __noop
#define NVTX_RangePushA __noop
#define NVTX_RangePushW __noop
#define NVTX_RangePop __noop
#define NVTX_NameOsThreadA __noop
#define NVTX_NameOsThreadW __noop


#endif

// C++ function templates to enable NvToolsExt functions
namespace nvtx
{
#ifdef NVTX_ENABLE

class Attributes
{
public:
  Attributes() {clear();}
  Attributes& category(uint32_t category) {m_event.category = category;  return *this;}
  Attributes& color(uint32_t argb) {m_event.colorType = NVTX_COLOR_ARGB; m_event.color = argb; return *this;}
  Attributes& payload(uint64_t value) {m_event.payloadType = NVTX_PAYLOAD_TYPE_UNSIGNED_INT64; m_event.payload.ullValue = value; return *this;}
  Attributes& payload(int64_t value) {m_event.payloadType = NVTX_PAYLOAD_TYPE_INT64; m_event.payload.llValue = value; return *this;}
  Attributes& payload(double value) {m_event.payloadType = NVTX_PAYLOAD_TYPE_DOUBLE; m_event.payload.dValue = value;return *this;}
  Attributes& message(const char* message) {m_event.messageType = NVTX_MESSAGE_TYPE_ASCII; m_event.message.ascii = message; return *this;}
  Attributes& message(const wchar_t* message) {m_event.messageType = NVTX_MESSAGE_TYPE_UNICODE; m_event.message.unicode = message; return *this;}
  Attributes& clear() {memset(&m_event, 0, NVTX_EVENT_ATTRIB_STRUCT_SIZE); m_event.version = NVTX_VERSION; m_event.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE; return *this;}
  const nvtxEventAttributes_t* out() const {return &m_event;}
private:
  nvtxEventAttributes_t m_event;
};


class ScopedRange
{
public:
  ScopedRange(const char* message)		{ nvtxRangePushA(message); }
  ScopedRange(const wchar_t* message)  { nvtxRangePushW(message); }
  ScopedRange(const nvtxEventAttributes_t* attributes)  { nvtxRangePushEx(attributes); }
  ScopedRange(const nvtx::Attributes& attributes)		 { nvtxRangePushEx(attributes.out()); }
  ~ScopedRange() { nvtxRangePop(); }
};

inline void Mark(const nvtx::Attributes& attrib) { nvtxMarkEx(attrib.out()); }
inline void Mark(const nvtxEventAttributes_t* eventAttrib) { nvtxMarkEx(eventAttrib); }
inline void Mark(const char* message) { nvtxMarkA(message); }
inline void Mark(const wchar_t* message) { nvtxMarkW(message); }
inline nvtxRangeId_t RangeStart(const nvtx::Attributes& attrib) { return nvtxRangeStartEx(attrib.out()); }
inline nvtxRangeId_t RangeStart(const nvtxEventAttributes_t* eventAttrib) { return nvtxRangeStartEx(eventAttrib); }
inline nvtxRangeId_t RangeStart(const char* message) { return nvtxRangeStartA(message); }
inline nvtxRangeId_t RangeStart(const wchar_t* message) { return nvtxRangeStartW(message); }
inline void RangeEnd(nvtxRangeId_t id) { nvtxRangeEnd(id); }
inline int RangePush(const nvtx::Attributes& attrib) { return nvtxRangePushEx(attrib.out()); }
inline int RangePush(const nvtxEventAttributes_t* eventAttrib) { return nvtxRangePushEx(eventAttrib); }
inline int RangePush(const char* message) { return nvtxRangePushA(message); }
inline int RangePush(const wchar_t* message) { return nvtxRangePushW(message); }
inline void RangePop() { nvtxRangePop(); }
inline void NameCategory(uint32_t category, const char* name) { nvtxNameCategoryA(category, name); }
inline void NameCategory(uint32_t category, const wchar_t* name) { nvtxNameCategoryW(category, name); }
inline void NameOsThread(uint32_t threadId, const char* name) { nvtxNameOsThreadA(threadId, name); }
inline void NameOsThread(uint32_t threadId, const wchar_t* name) { nvtxNameOsThreadW(threadId, name); }
inline void NameCurrentThread(const char* name) { nvtxNameOsThreadA(::GetCurrentThreadId(), name); }
inline void NameCurrentThread(const wchar_t* name) { nvtxNameOsThreadW(::GetCurrentThreadId(), name); }

#else

class Attributes
{
public:
  Attributes() {}
  Attributes& category(uint32_t category) { return *this; }
  Attributes& color(uint32_t argb) { return *this; }
  Attributes& payload(uint64_t value) { return *this; }
  Attributes& payload(int64_t value) { return *this; }
  Attributes& payload(double value) { return *this; }
  Attributes& message(const char* message) { return *this; }
  Attributes& message(const wchar_t* message) { return *this; }
  Attributes& clear() { return *this; }
  const nvtxEventAttributes_t* out() { return 0; }
};

class ScopedRange
{
public:
  ScopedRange(const char* message) { (void)message; }
  ScopedRange(const wchar_t* message) { (void)message; }
  ScopedRange(const nvtxEventAttributes_t* attributes) { (void)attributes; }
  ScopedRange(const Attributes& attributes) { (void)attributes; }
  ~ScopedRange() {}
};

inline void Mark(const nvtx::Attributes& attrib) { (void)attrib; }
inline void Mark(const nvtxEventAttributes_t* eventAttrib) { (void)eventAttrib; }
inline void Mark(const char* message) { (void)message; }
inline void Mark(const wchar_t* message) { (void)message; }
inline nvtxRangeId_t RangeStart(const nvtx::Attributes& attrib) { (void)attrib; return 0; }
inline nvtxRangeId_t RangeStart(const nvtxEventAttributes_t* eventAttrib) { (void)eventAttrib; return 0; }
inline nvtxRangeId_t RangeStart(const char* message) { (void)message; return 0; }
inline nvtxRangeId_t RangeStart(const wchar_t* message) { (void)message; return 0; }
inline void RangeEnd(nvtxRangeId_t id) { (void)id; }
inline int RangePush(const nvtx::Attributes& attrib) { (void)attrib; return -1; }
inline int RangePush(const nvtxEventAttributes_t* eventAttrib) { (void)eventAttrib; return -1; }
inline int RangePush(const char* message) { (void)message; return -1;}
inline int RangePush(const wchar_t* message) { (void)message; return -1; }
inline int RangePop() { return -1; }
inline void NameCategory(uint32_t category, const char* name) { (void)category; (void)name; }
inline void NameCategory(uint32_t category, const wchar_t* name) { (void)category; (void)name; }
inline void NameOsThread(uint32_t threadId, const char* name) { (void)threadId; (void)name; }
inline void NameOsThread(uint32_t threadId, const wchar_t* name) { (void)threadId; (void)name; }
inline void NameCurrentThread(const char* name) { (void)name; }
inline void NameCurrentThread(const wchar_t* name) { (void)name; }

#endif
} //nvtx
