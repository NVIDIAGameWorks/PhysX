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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

#include "NvParameterized.h"
#include "NvParameterizedTraits.h"

namespace NvParameterized
{

/*!
\brief Serializer::SerializePlatform and Serializer::DeserializedData inline implementation
*/

/**
\brief Check binary compatibility of compiler versions
*/
PX_INLINE bool DoCompilerVersMatch(SerializePlatform::CompilerType t, uint32_t v1, uint32_t v2)
{
	PX_UNUSED(t);
	if( SerializePlatform::ANY_VERSION == v1 || SerializePlatform::ANY_VERSION == v2 )
		return true;
	
	//In future we should distinguish compiler versions which have different ABI
	//but now we are optimistic

	return true;
}

/**
\brief Check binary compatibility of OS versions
*/
PX_INLINE bool DoOsVersMatch(SerializePlatform::OsType t, uint32_t v1, uint32_t v2)
{
	PX_UNUSED(t);
	if( SerializePlatform::ANY_VERSION == v1 || SerializePlatform::ANY_VERSION == v2 )
		return true;
	
	return true; //See comment for doCompilerVersMatch
}

PX_INLINE SerializePlatform::SerializePlatform()
	: archType(ARCH_LAST),
	compilerType(COMP_LAST),
	compilerVer(ANY_VERSION),
	osType(OS_LAST),
	osVer(ANY_VERSION)
{}

PX_INLINE SerializePlatform::SerializePlatform(ArchType archType_, CompilerType compType_, uint32_t compVer_, OsType osType_, uint32_t osVer_)
	: archType(archType_),
	compilerType(compType_),
	compilerVer(compVer_),
	osType(osType_),
	osVer(osVer_)
{}

PX_INLINE bool SerializePlatform::operator ==(const SerializePlatform &p) const
{
	return archType == p.archType
		&& compilerType == p.compilerType
		&& osType == p.osType
		&& DoCompilerVersMatch(compilerType, compilerVer, p.compilerVer)
		&& DoOsVersMatch(osType, osVer, p.osVer);
}

PX_INLINE bool SerializePlatform::operator !=(const SerializePlatform &p) const
{
	return !(*this == p);
}

template<typename T, int bufSize> PX_INLINE Serializer::DeserializedResults<T, bufSize>::DeserializedResults(): objs(0), nobjs(0), traits(0) {}

template<typename T, int bufSize> PX_INLINE Serializer::DeserializedResults<T, bufSize>::DeserializedResults(const Serializer::DeserializedResults<T, bufSize> &data)
{
	*this = data;
}

template<typename T, int bufSize> PX_INLINE Serializer::DeserializedResults<T, bufSize> &Serializer::DeserializedResults<T, bufSize>::operator =(const Serializer::DeserializedResults<T, bufSize> &rhs)
{
	if( this == &rhs )
		return *this;

	init(rhs.traits, rhs.objs, rhs.nobjs);
	return *this;
}

template<typename T, int bufSize> PX_INLINE void Serializer::DeserializedResults<T, bufSize>::clear()
{
	if ( objs && objs != buf ) //Memory was allocated?
	{
		PX_ASSERT(traits);
		traits->free(objs);
	}
}

template<typename T, int bufSize> PX_INLINE Serializer::DeserializedResults<T, bufSize>::~DeserializedResults()
{
	clear();
}

template<typename T, int bufSize> PX_INLINE void Serializer::DeserializedResults<T, bufSize>::init(Traits *traits_, T *objs_, uint32_t nobjs_)
{
	init(traits_, nobjs_);
	::memcpy(objs, objs_, nobjs * sizeof(T));
}

template<typename T, int bufSize> PX_INLINE void Serializer::DeserializedResults<T, bufSize>::init(Traits *traits_, uint32_t nobjs_)
{
	clear();

	traits = traits_;
	nobjs = nobjs_;

	//Allocate memory if buf is too small
	objs = nobjs <= bufSize
		? buf
		: (T *)traits->alloc(nobjs * sizeof(T));
}

template<typename T, int bufSize> PX_INLINE uint32_t Serializer::DeserializedResults<T, bufSize>::size() const
{
	return nobjs;
}

template<typename T, int bufSize> PX_INLINE T &Serializer::DeserializedResults<T, bufSize>::operator[](uint32_t i)
{
	PX_ASSERT( i < nobjs );
	return objs[i];
}

template<typename T, int bufSize> PX_INLINE const T &Serializer::DeserializedResults<T, bufSize>::operator[](uint32_t i) const
{
	PX_ASSERT( i < nobjs );
	return objs[i];
}

template<typename T, int bufSize> PX_INLINE void Serializer::DeserializedResults<T, bufSize>::getObjects(T *outObjs)
{
	::memcpy(outObjs, objs, nobjs * sizeof(T));
}

template<typename T, int bufSize> PX_INLINE void Serializer::DeserializedResults<T, bufSize>::releaseAll()
{
	for(uint32_t i = 0; i < nobjs; ++i)
	{
		if (objs[i]) 
		{
			objs[i]->destroy(); // FIXME What should we do with buf. And should we delete T* obj? 
		}
	}
}

} // namespace NvParameterized
