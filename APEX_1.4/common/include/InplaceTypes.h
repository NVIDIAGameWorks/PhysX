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



#ifndef __APEX_INPLACE_TYPES_H__
#define __APEX_INPLACE_TYPES_H__

#include "ApexUsingNamespace.h"
#include "PxVec3.h"
#include "PxVec4.h"
#include "PxBounds3.h"
#include "PxMat44.h"


namespace nvidia
{
namespace apex
{

#define INPLACE_TYPE_BUILD() "InplaceTypesBuilder.h"
#define INPLACE_TYPE_FIELD(_field_type_, _field_name_)                 ( (2, (_field_type_, _field_name_)) )
#define INPLACE_TYPE_FIELD_N(_field_type_, _field_name_, _field_size_) ( (3, (_field_type_, _field_name_, _field_size_)) )

#define APEX_OFFSETOF(type, member) offsetof(type, member)

#ifdef __CUDACC__
#define APEX_CUDA_CALLABLE __device__
#else
#define APEX_CUDA_CALLABLE
#endif


#ifdef __CUDACC__
#define INPLACE_TEMPL_ARGS_DEF template <bool GpuInplaceStorageTemplArg>
#define INPLACE_TEMPL_ARGS_VAL <GpuInplaceStorageTemplArg>
#define INPLACE_TEMPL_VA_ARGS_DEF(...) template <bool GpuInplaceStorageTemplArg, __VA_ARGS__>
#define INPLACE_TEMPL_VA_ARGS_VAL(...) <GpuInplaceStorageTemplArg, __VA_ARGS__>
#define INPLACE_STORAGE_SUB_ARGS_DEF const uint8_t* _arg_constMem_, texture<int, 1, cudaReadModeElementType> _arg_texRef_
#define INPLACE_STORAGE_ARGS_DEF InplaceHandleBase::StorageSelector<GpuInplaceStorageTemplArg> _arg_selector_, INPLACE_STORAGE_SUB_ARGS_DEF
#define INPLACE_STORAGE_ARGS_VAL _arg_selector_, _arg_constMem_, _arg_texRef_
#define CPU_INPLACE_STORAGE_ARGS_UNUSED
#else
#define INPLACE_TEMPL_ARGS_DEF template <typename CpuInplaceStorageTemplArg>
#define INPLACE_TEMPL_ARGS_VAL <CpuInplaceStorageTemplArg>
#define INPLACE_TEMPL_VA_ARGS_DEF(...) template <typename CpuInplaceStorageTemplArg, __VA_ARGS__>
#define INPLACE_TEMPL_VA_ARGS_VAL(...) <CpuInplaceStorageTemplArg, __VA_ARGS__>
#define INPLACE_STORAGE_ARGS_DEF const CpuInplaceStorageTemplArg& _arg_storage_
#define INPLACE_STORAGE_ARGS_VAL _arg_storage_
#define CPU_INPLACE_STORAGE_ARGS_UNUSED PX_UNUSED(_arg_storage_);
#endif


template <bool AutoFree>
struct InplaceTypeMemberTraits
{
	enum { AutoFreeValue = AutoFree };
};

typedef InplaceTypeMemberTraits<false> InplaceTypeMemberDefaultTraits;

template <typename T>
struct InplaceTypeTraits;

class InplaceTypeHelper
{
	template <int n>
	struct ArrayIterator
	{
		template <int _inplace_offset_, typename R, typename RA, typename T, int N, typename MT>
		APEX_CUDA_CALLABLE PX_INLINE static void reflectArray(R& r, RA ra, T (& arr)[N], MT mt)
		{
			InplaceTypeHelper::reflectType<(N - n)*sizeof(T) + _inplace_offset_>(r, ra, arr[ (N - n) ], mt);

			ArrayIterator<n - 1>::reflectArray<_inplace_offset_>(r, ra, arr, mt);
		}
	};

public:
	template <int _inplace_offset_, typename R, typename RA, typename T, typename MT>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, T& t, MT mt)
	{
		r.processType<_inplace_offset_>(ra, t, mt);
		InplaceTypeTraits<T>::reflectType<_inplace_offset_>(r, ra, t);
	}

	template <int _inplace_offset_, typename R, typename RA, typename T, int N, typename MT>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, T (& arr)[N], MT mt)
	{
		ArrayIterator<N>::reflectArray<_inplace_offset_>(r, ra, arr, mt);
	}

#ifdef __CUDACC__
	class FetchReflector4ConstMem
	{
	public:
		APEX_CUDA_CALLABLE PX_INLINE FetchReflector4ConstMem() {}

		template <int _inplace_offset_, typename T, typename MT>
		APEX_CUDA_CALLABLE PX_INLINE void processType(const uint8_t* constMem, T const& value, MT )
		{
			; //do nothing
		}

		template <int _inplace_offset_, typename T>
		APEX_CUDA_CALLABLE PX_INLINE void processPrimitiveType(const uint8_t* constMem, T& out)
		{
			out = *reinterpret_cast<const T*>(constMem + _inplace_offset_);
		}
	};
	template <typename T>
	APEX_CUDA_CALLABLE PX_INLINE static void fetchType(T& out, const uint8_t* constMem, uint32_t offset0)
	{
		FetchReflector4ConstMem r;
		InplaceTypeHelper::reflectType<0>(r, constMem + offset0, out, InplaceTypeMemberDefaultTraits());
	}

	class FetchReflector4TexRef
	{
		uint32_t texIdx0;

		APEX_CUDA_CALLABLE PX_INLINE void convertValue(int value, int32_t& out)
		{
			out = int32_t(value);
		}
		APEX_CUDA_CALLABLE PX_INLINE void convertValue(int value, uint32_t& out)
		{
			out = uint32_t(value);
		}
		APEX_CUDA_CALLABLE PX_INLINE void convertValue(int value, float& out)
		{
			out = __int_as_float(value);
		}
		template <typename T>
		APEX_CUDA_CALLABLE PX_INLINE void convertValue(int value, T*& out)
		{
			out = reinterpret_cast<T*>(value);
		}

		template <int _inplace_offset_, typename T>
		APEX_CUDA_CALLABLE PX_INLINE void processValue(texture<int, 1, cudaReadModeElementType> texRef, T& out)
		{
			PX_COMPILE_TIME_ASSERT((_inplace_offset_ & 3) == 0);
			//PX_COMPILE_TIME_ASSERT(sizeof(T) == 4); this fails to compile
			const int texIdx = (_inplace_offset_ >> 2);

			const int value = tex1Dfetch(texRef, texIdx0 + texIdx);
			convertValue(value, out);
		}

		template <int _inplace_offset_, typename T>
		APEX_CUDA_CALLABLE PX_INLINE void processValue64(texture<int, 1, cudaReadModeElementType> texRef, T& out)
		{
			PX_COMPILE_TIME_ASSERT((_inplace_offset_ & 7) == 0);
			PX_COMPILE_TIME_ASSERT(sizeof(T) == 8);
			const int texIdx = (_inplace_offset_ >> 2);
			union
			{
				struct
				{
					int value0;
					int value1;
				};
				long long value;
			} u;
			u.value0 = tex1Dfetch(texRef, texIdx0 + texIdx + 0);
			u.value1 = tex1Dfetch(texRef, texIdx0 + texIdx + 1);
			out = reinterpret_cast<T>(u.value);
		}

	public:
		APEX_CUDA_CALLABLE PX_INLINE FetchReflector4TexRef(uint32_t offset0) : texIdx0(offset0 >> 2) {}

		template <int _inplace_offset_, typename T, typename MT>
		APEX_CUDA_CALLABLE PX_INLINE void processType(texture<int, 1, cudaReadModeElementType> texRef, T const& value, MT )
		{
			; //do nothing
		}

		template <int _inplace_offset_, typename T>
		APEX_CUDA_CALLABLE PX_INLINE void processPrimitiveType(texture<int, 1, cudaReadModeElementType> texRef, T& out)
		{
			processValue<_inplace_offset_>(texRef, out);
		}

		template <int _inplace_offset_, typename T>
		APEX_CUDA_CALLABLE PX_INLINE void processPrimitiveType(texture<int, 1, cudaReadModeElementType> texRef, T*& out)
		{
#if PX_X64
			processValue64<_inplace_offset_>(texRef, out);
#else
			processValue<_inplace_offset_>(texRef, out);
#endif
		}
	};
	template <typename T>
	APEX_CUDA_CALLABLE PX_INLINE static void fetchType(T& out, texture<int, 1, cudaReadModeElementType> texRef, uint32_t offset0)
	{
		FetchReflector4TexRef r(offset0);
		InplaceTypeHelper::reflectType<0>(r, texRef, out, InplaceTypeMemberDefaultTraits());
	}
#endif
};

template <>
struct InplaceTypeHelper::ArrayIterator<0>
{
	template <int _inplace_offset_, typename R, typename RA, typename T, int N, typename MT>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectArray(R& , RA , T (& )[N], MT )
	{
		; // do nothing
	}
};

template <typename T>
struct InplaceTypeTraits
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, T& t)
	{
		t.reflectSelf<_inplace_offset_>(r, ra);
	}
};


class InplacePrimitive
{
protected:
	int _value;

	APEX_CUDA_CALLABLE PX_INLINE InplacePrimitive() {}

public:
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& r, RA ra)
	{
		//r.processPrimitiveType<APEX_OFFSETOF(InplacePrimitive, _value) + _inplace_offset_>(ra, _value);
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(InplacePrimitive, _value) + _inplace_offset_>(r, ra, _value, InplaceTypeMemberDefaultTraits());
	}
};

class InplaceBool : public InplacePrimitive
{
public:
	APEX_CUDA_CALLABLE PX_INLINE InplaceBool() {}
	APEX_CUDA_CALLABLE PX_INLINE InplaceBool(bool b) { _value = (b ? 1 : 0); }
	APEX_CUDA_CALLABLE PX_INLINE InplaceBool& operator= (bool b) { _value = (b ? 1 : 0); return *this; }
	APEX_CUDA_CALLABLE PX_INLINE operator bool () const { return (_value != 0); }
};

template <typename ET>
class InplaceEnum : public InplacePrimitive
{
public:
	APEX_CUDA_CALLABLE PX_INLINE InplaceEnum() {}
	APEX_CUDA_CALLABLE PX_INLINE InplaceEnum(ET value) { _value = value; }
	APEX_CUDA_CALLABLE PX_INLINE InplaceEnum<ET>& operator=(ET value) { _value = value; return *this; }
	APEX_CUDA_CALLABLE PX_INLINE operator ET () const { return ET(_value); }
};


template <typename T> struct InplaceTypeTraits<T*>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, T*& t) { r.processPrimitiveType<_inplace_offset_>(ra, t); }
};
template <> struct InplaceTypeTraits<int32_t>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, int32_t& t) { r.processPrimitiveType<_inplace_offset_>(ra, t); }
};
template <> struct InplaceTypeTraits<uint32_t>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, uint32_t& t) { r.processPrimitiveType<_inplace_offset_>(ra, t); }
};
template <> struct InplaceTypeTraits<float>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, float& t) { r.processPrimitiveType<_inplace_offset_>(ra, t); }
};
template <> struct InplaceTypeTraits<PxVec3>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxVec3& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec3, x) + _inplace_offset_>(r, ra, t.x, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec3, y) + _inplace_offset_>(r, ra, t.y, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec3, z) + _inplace_offset_>(r, ra, t.z, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxVec4>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxVec4& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec4, x) + _inplace_offset_>(r, ra, t.x, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec4, y) + _inplace_offset_>(r, ra, t.y, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec4, z) + _inplace_offset_>(r, ra, t.z, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxVec4, w) + _inplace_offset_>(r, ra, t.w, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxTransform>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxTransform& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxTransform, q) + _inplace_offset_>(r, ra, t.q, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxTransform, p) + _inplace_offset_>(r, ra, t.p, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxQuat>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxQuat& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxQuat, x) + _inplace_offset_>(r, ra, t.x, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxQuat, y) + _inplace_offset_>(r, ra, t.y, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxQuat, z) + _inplace_offset_>(r, ra, t.z, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxQuat, w) + _inplace_offset_>(r, ra, t.w, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxBounds3>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxBounds3& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxBounds3, minimum) + _inplace_offset_>(r, ra, t.minimum, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxBounds3, maximum) + _inplace_offset_>(r, ra, t.maximum, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxPlane>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxPlane& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxPlane, n) + _inplace_offset_>(r, ra, t.n, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxPlane, d) + _inplace_offset_>(r, ra, t.d, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxMat33>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxMat33& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat33, column0) + _inplace_offset_>(r, ra, t.column0, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat33, column1) + _inplace_offset_>(r, ra, t.column1, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat33, column2) + _inplace_offset_>(r, ra, t.column2, InplaceTypeMemberDefaultTraits());
	}
};
template <> struct InplaceTypeTraits<PxMat44>
{
	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE static void reflectType(R& r, RA ra, PxMat44& t)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat44, column0) + _inplace_offset_>(r, ra, t.column0, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat44, column1) + _inplace_offset_>(r, ra, t.column1, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat44, column2) + _inplace_offset_>(r, ra, t.column2, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(PxMat44, column3) + _inplace_offset_>(r, ra, t.column3, InplaceTypeMemberDefaultTraits());
	}
};


class InplaceHandleBase
{
protected:
	friend class InplaceStorage;
	friend class InplaceArrayBase;

	static const uint32_t NULL_VALUE = uint32_t(-1);
	uint32_t _value;

public:
	APEX_CUDA_CALLABLE PX_INLINE InplaceHandleBase()
	{
		_value = NULL_VALUE;
	}

	PX_INLINE void setNull()
	{
		_value = NULL_VALUE;
	}

	APEX_CUDA_CALLABLE PX_INLINE bool isNull() const
	{
		return _value == NULL_VALUE;
	}

#ifdef __CUDACC__
	template <bool B>
	struct StorageSelector
	{
		static const bool value = B;
	};

	template <typename T>
	APEX_CUDA_CALLABLE PX_INLINE void fetch(StorageSelector<false>, INPLACE_STORAGE_SUB_ARGS_DEF, T& out, uint32_t index = 0) const
	{
		//out = reinterpret_cast<const T*>(_arg_constMem_ + _value)[index];
		InplaceTypeHelper::fetchType<T>(out, _arg_constMem_, _value + sizeof(T) * index);
	}

	template <typename T>
	APEX_CUDA_CALLABLE PX_INLINE void fetch(StorageSelector<true>, INPLACE_STORAGE_SUB_ARGS_DEF, T& out, uint32_t index = 0) const
	{
		InplaceTypeHelper::fetchType<T>(out, _arg_texRef_, _value + sizeof(T) * index);
	}
#else
	template <typename S, typename T>
	PX_INLINE void fetch(const S& storage, T& out, uint32_t index = 0) const
	{
		storage.fetch(*this, out, index);
	}

	template <typename S, typename T>
	PX_INLINE void update(S& storage, const T& in, uint32_t index = 0) const
	{
		storage.update(*this, in, index);
	}

	template <typename S, typename T>
	PX_INLINE bool allocOrFetch(S& storage, T& out)
	{
		if (isNull())
		{
			storage.template alloc<T>(*this);
			return true;
		}
		else
		{
			storage.fetch(*this, out);
			return false;
		}
	}
#endif

	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& r, RA ra)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(InplaceHandleBase, _value) + _inplace_offset_>(r, ra, _value, InplaceTypeMemberDefaultTraits());
	}
};

template <typename T>
class InplaceHandle : public InplaceHandleBase
{
public:
	APEX_CUDA_CALLABLE PX_INLINE InplaceHandle() {}

	template <typename S>
	PX_INLINE bool alloc(S& storage)
	{
		return storage.alloc(*this);
	}
};


class InplaceArrayBase
{
protected:
	uint32_t _size;
	InplaceHandleBase _elems;

#ifndef __CUDACC__
	PX_INLINE InplaceArrayBase()
	{
		_size = 0;
	}
#endif

	template <int _inplace_offset_, typename R, typename RA, typename MT>
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& r, RA ra, MT mt)
	{
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(InplaceArrayBase, _size)  + _inplace_offset_>(r, ra, _size, InplaceTypeMemberDefaultTraits());
		InplaceTypeHelper::reflectType<APEX_OFFSETOF(InplaceArrayBase, _elems) + _inplace_offset_>(r, ra, _elems, mt);
	}
};


// if AutoFreeElems is true, then InplaceHandles in deleted elements are automaticly got free on array resize!
template <typename T, bool AutoFreeElems = false>
class InplaceArray : public InplaceArrayBase
{
public:
	APEX_CUDA_CALLABLE PX_INLINE uint32_t getSize() const
	{
		return _size;
	}

#ifndef __CUDACC__
	PX_INLINE InplaceArray()
	{
	}

	template <typename S>
	PX_INLINE void fetchElem(const S& storage, T& out, uint32_t index) const
	{
		storage.fetch(_elems, out, index);
	}
	template <typename S>
	PX_INLINE void updateElem(S& storage, const T& in, uint32_t index) const
	{
		storage.update(_elems, in, index);
	}
	template <typename S>
	PX_INLINE void updateRange(S& storage, const T* in, uint32_t count, uint32_t start = 0) const
	{
		storage.updateRange(_elems, in, count, start);
	}

	template <typename S>
	PX_INLINE bool resize(S& storage, uint32_t size)
	{
		if (storage.template realloc<T>(_elems, _size, size))
		{
			_size = size;
			return true;
		}
		return false;
	}

#else
	INPLACE_TEMPL_ARGS_DEF
	APEX_CUDA_CALLABLE PX_INLINE void fetchElem(INPLACE_STORAGE_ARGS_DEF, T& out, uint32_t index) const
	{
		_elems.fetch(INPLACE_STORAGE_ARGS_VAL, out, index);
	}
#endif

	template <int _inplace_offset_, typename R, typename RA>
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& r, RA ra)
	{
		InplaceArrayBase::reflectSelf<_inplace_offset_>(r, ra, InplaceTypeMemberTraits<AutoFreeElems>());
	}

};


}
} // end namespace nvidia::apex

#endif // __APEX_INPLACE_TYPES_H__
