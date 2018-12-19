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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef PX_PARAMETERS_H
#define PX_PARAMETERS_H

#include "PsMutex.h"
#include "PsAllocator.h"
#include "PxAssert.h"
#include "nvparameterized/NvParameterized.h"
#include "NvParametersTypes.h"

#define NV_PARAM_PLACEMENT_NEW(p, T)        new(p) T

namespace NvParameterized
{
	typedef physx::shdfnd::MutexT<physx::shdfnd::RawAllocator> MutexType;

	const char *typeToStr(DataType type);
	DataType strToType(const char *str);

	PX_INLINE static bool IsAligned(const void *p, uint32_t border)
	{
		return !(reinterpret_cast<size_t>(p) % border); //size_t == uintptr_t
	}

	PX_INLINE static bool IsAligned(uint32_t x, uint32_t border)
	{
		return !(x % border);
	}

    // Used for associating useful info with parameters (e.g. min, max, step, etc...)
    class HintImpl : public Hint
    {
    public:

        HintImpl();
        HintImpl(const char *name, uint64_t value);
        HintImpl(const char *name, double value);
        HintImpl(const char *name, const char *value);
        virtual ~HintImpl();

        void init(const char *name, uint64_t value, bool static_allocation = false);
        void init(const char *name, double value, bool static_allocation = false);
        void init(const char *name, const char *value, bool static_allocation = false);
        void cleanup();

        const char *name(void) const { return(mName); }
        DataType type(void) const { return(mType); }

		uint64_t asUInt(void) const;
		double asFloat(void) const;
		const char *asString(void) const;
		bool setAsUInt(uint64_t v);

     private:

        bool mStaticAllocation;
        char *mName;
        DataType mType;

        union
        {
            uint64_t mUIntValue;
            double mFloatValue;
            char *mStringValue;
        };
    };


    class DefinitionImpl : public Definition
    {
    public:

        DefinitionImpl(Traits &traits, bool staticAlloc = true);
        DefinitionImpl(const char *name, DataType t, const char *structName, Traits &traits, bool staticAlloc = true);
        virtual ~DefinitionImpl();

        // can be used instead of the constructors and destructor
        void init(const char *name, DataType t, const char *structName, bool static_allocation = false);
        void cleanup(void);
        void destroy();

        const char *name(void) const 
		{ 
			return(mName); 
		}
        const char *longName(void) const { return(mLongName); }
        const char *structName(void) const { return(mStructName); }
        DataType type(void) const { return(mType); }
		const char* typeString() const { return typeToStr(mType); }

        int32_t arrayDimension(void) const;
        int32_t arraySize(int32_t dimension = 0) const;
        bool arraySizeIsFixed(void) const;
        bool setArraySize(int32_t size); // -1 if the size is not fixed

        bool isIncludedRef(void) const;

        bool isLeaf(void) const { return(type() != TYPE_STRUCT && type() != TYPE_ARRAY); }

        const Definition *parent(void) const { return(mParent); }
        const Definition *root(void) const;

        // Only used with parameters of TYPE_STRUCT or TYPE_ARRAY
        int32_t numChildren(void) const;
        const Definition *child(int32_t index) const;
        const Definition *child(const char *name, int32_t &index) const; // only used with TYPE_STRUCT
        void setChildren(Definition **children, int32_t n);
        void addChild(Definition *child);

        int32_t numHints(void) const;
        const Hint *hint(int32_t index) const;
        const Hint *hint(const char *name) const;
        void setHints(const Hint **hints, int32_t n);
        void addHint(Hint *hint);

        int32_t numEnumVals(void) const;
		int32_t enumValIndex( const char * enum_val ) const;

        // The pointers returned by enumVal() are good for the lifetime of the DefinitionImpl object.
        const char *enumVal(int32_t index) const;
        void setEnumVals(const char **enum_vals, int32_t n);
        void addEnumVal(const char *enum_val);


		int32_t numRefVariants(void) const;
		int32_t refVariantValIndex( const char * ref_val ) const;

		// The pointers returned by refVariantVal() are good for the lifetime of the DefinitionImpl object.
		const char * refVariantVal(int32_t index) const;
        void setRefVariantVals(const char **ref_vals, int32_t n);
        void addRefVariantVal(const char *ref_val);

		uint32_t alignment(void) const;
		void setAlignment(uint32_t align);

		uint32_t padding(void) const;
		void setPadding(uint32_t align);

		void setDynamicHandleIndicesMap(const uint8_t *indices, uint32_t numIndices);
		const uint8_t * getDynamicHandleIndicesMap(uint32_t &outNumIndices) const;

		bool isSimpleType(bool simpleStructs, bool simpleStrings) const;

    private:

        enum { MAX_NAME_LEN = 256 };

        void setDefaults(void);

        bool mStaticAllocation;

        const char * mName;
        const char * mLongName;
        const char * mStructName;
        DataType mType;
        int32_t mArraySize;

        DefinitionImpl *mParent;

        int32_t mNumChildren;
        Definition **mChildren;

        int32_t mNumHints;
        HintImpl **mHints;

        int32_t mNumEnumVals;
        char **mEnumVals;

		int32_t mNumRefVariants;
        char **mRefVariantVals;

		Traits *mTraits;
		bool mLongNameAllocated;

		uint32_t mAlign;
		uint32_t mPad;

		uint32_t mNumDynamicHandleIndices;
		const uint8_t *mDynamicHandleIndices;
    };

	// Used by generated code
	struct ParamLookupNode
	{
		NvParameterized::DataType type;
		bool isDynamicArrayRoot;
		size_t offset;
		const size_t* children;
		int numChildren;
	};

	//Constructs temporary object and extract vptr
	template<typename T, size_t T_align>
	const char* getVptr()
	{
		char buf[T_align + sizeof(T)];

		// We want all isAllocated false
		memset(buf, false, sizeof(buf));

		// Align
		char* bufAligned = (char*)((size_t)(buf + T_align) & ~(T_align - 1));

		// Call "fast" constructor (buf and refCount != NULL)
		// which does not allocate members
		T* tmp = NV_PARAM_PLACEMENT_NEW(bufAligned, T)(0, reinterpret_cast<void*>(16), reinterpret_cast<int32_t*>(16));

		// vptr is usually stored at the beginning of object...
		const char* vptr = *reinterpret_cast<const char**>(tmp);

		//Cleanup
		tmp->~T();

		return vptr;
	}

class NvParameters : public NvParameterized::Interface
{
public:

    NvParameters(Traits *traits, void *buf = 0, int32_t *refCount = 0); //Long form is used for inplace objects
    virtual ~NvParameters();

	// placement delete
	virtual void destroy();
	virtual void initDefaults(void) { }
	virtual void initRandom(void);

	virtual const char * className(void) const { return mClassName; }
	virtual void setClassName(const char *name);

	virtual const char * name(void) const { return mName; }
	virtual void setName(const char *name);

	virtual uint32_t version(void) const { return 0; }
	virtual uint16_t getMajorVersion(void) const;
	virtual uint16_t getMinorVersion(void) const;

	virtual const uint32_t * checksum(uint32_t &bits) const
	{
		bits = 0;
		return 0;
	}

    int32_t numParameters(void);
    const Definition *parameterDefinition(int32_t index);

    const Definition *rootParameterDefinition(void);
	const Definition *rootParameterDefinition(void) const;

    // Given a long name like "mystruct.somearray[10].foo", it will return
    // a handle to that specific parameter.  The handle can then be used to
    // set/get values, as long as it's a handle to a leaf node.
    virtual ErrorType getParameterHandle(const char *long_name, Handle &handle) const;
	virtual ErrorType getParameterHandle(const char *long_name, Handle &handle);

	virtual void setSerializationCallback(SerializationCallback *cb, void *userData = NULL);
	virtual ErrorType callPreSerializeCallback() const;

	ErrorType initParamRef(const Handle &handle, const char *chosenRefStr = 0, bool doDestroyOld = false);

    // These functions wrap the raw(Get|Set)XXXXX() methods.  They deal with
    // error handling and casting.
    ErrorType getParamBool(const Handle &handle, bool &val) const;
    ErrorType setParamBool(const Handle &handle, bool val);
    ErrorType getParamBoolArray(const Handle &handle, bool *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamBoolArray(const Handle &handle, const bool *array, int32_t n, int32_t offset= 0);

    ErrorType getParamString(const Handle &handle, const char *&val) const;
    ErrorType setParamString(const Handle &handle, const char *val);
    ErrorType getParamStringArray(const Handle &handle, char **array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamStringArray(const Handle &handle, const char **array, int32_t n, int32_t offset= 0);

    ErrorType getParamEnum(const Handle &handle, const char *&val) const;
    ErrorType setParamEnum(const Handle &handle, const char *val);
    ErrorType getParamEnumArray(const Handle &handle, char **array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamEnumArray(const Handle &handle, const char **array, int32_t n, int32_t offset= 0);

	ErrorType getParamRef(const Handle &handle, NvParameterized::Interface *&val) const;
    ErrorType setParamRef(const Handle &handle, NvParameterized::Interface * val, bool doDestroyOld = false);
    ErrorType getParamRefArray(const Handle &handle, NvParameterized::Interface **array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamRefArray(const Handle &handle, /*const*/ NvParameterized::Interface **array, int32_t n, int32_t offset = 0, bool doDestroyOld = false);

	ErrorType getParamI8(const Handle &handle, int8_t &val) const;
    ErrorType setParamI8(const Handle &handle, int8_t val);
    ErrorType getParamI8Array(const Handle &handle, int8_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamI8Array(const Handle &handle, const int8_t *val, int32_t n, int32_t offset= 0);

    ErrorType getParamI16(const Handle &handle, int16_t &val) const;
    ErrorType setParamI16(const Handle &handle, int16_t val);
    ErrorType getParamI16Array(const Handle &handle, int16_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamI16Array(const Handle &handle, const int16_t *val, int32_t n, int32_t offset= 0);

    ErrorType getParamI32(const Handle &handle, int32_t &val) const;
    ErrorType setParamI32(const Handle &handle, int32_t val);
    ErrorType getParamI32Array(const Handle &handle, int32_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamI32Array(const Handle &handle, const int32_t *val, int32_t n, int32_t offset= 0);

    ErrorType getParamI64(const Handle &handle, int64_t &val) const;
    ErrorType setParamI64(const Handle &handle, int64_t val);
    ErrorType getParamI64Array(const Handle &handle, int64_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamI64Array(const Handle &handle, const int64_t *val, int32_t n, int32_t offset= 0);

    ErrorType getParamU8(const Handle &handle, uint8_t &val) const;
    ErrorType setParamU8(const Handle &handle, uint8_t val);
    ErrorType getParamU8Array(const Handle &handle, uint8_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamU8Array(const Handle &handle, const uint8_t *val, int32_t n, int32_t offset= 0);

    ErrorType getParamU16(const Handle &handle, uint16_t &val) const;
    ErrorType setParamU16(const Handle &handle, uint16_t val);
    ErrorType getParamU16Array(const Handle &handle, uint16_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamU16Array(const Handle &handle, const uint16_t *array, int32_t n, int32_t offset= 0);

    ErrorType getParamU32(const Handle &handle, uint32_t &val) const;
    ErrorType setParamU32(const Handle &handle, uint32_t val);
    ErrorType getParamU32Array(const Handle &handle, uint32_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamU32Array(const Handle &handle, const uint32_t *array, int32_t n, int32_t offset= 0);

    ErrorType getParamU64(const Handle &handle, uint64_t &val) const;
    ErrorType setParamU64(const Handle &handle, uint64_t val);
    ErrorType getParamU64Array(const Handle &handle, uint64_t *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamU64Array(const Handle &handle, const uint64_t *array, int32_t n, int32_t offset= 0);

    ErrorType getParamF32(const Handle &handle, float &val) const;
    ErrorType setParamF32(const Handle &handle, float val);
    ErrorType getParamF32Array(const Handle &handle, float *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamF32Array(const Handle &handle, const float *array, int32_t n, int32_t offset= 0);

    ErrorType getParamF64(const Handle &handle, double &val) const;
    ErrorType setParamF64(const Handle &handle, double val);
    ErrorType getParamF64Array(const Handle &handle, double *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamF64Array(const Handle &handle, const double *array, int32_t n, int32_t offset= 0);

    ErrorType setParamVec2(const Handle &handle, const physx::PxVec2 &val);
    ErrorType getParamVec2(const Handle &handle, physx::PxVec2 &val) const;
    ErrorType getParamVec2Array(const Handle &handle, physx::PxVec2 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamVec2Array(const Handle &handle, const physx::PxVec2 *array, int32_t n, int32_t offset= 0);

	ErrorType setParamVec3(const Handle &handle, const physx::PxVec3 &val);
    ErrorType getParamVec3(const Handle &handle, physx::PxVec3 &val) const;
    ErrorType getParamVec3Array(const Handle &handle, physx::PxVec3 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamVec3Array(const Handle &handle, const physx::PxVec3 *array, int32_t n, int32_t offset= 0);

    ErrorType setParamVec4(const Handle &handle, const physx::PxVec4 &val);
    ErrorType getParamVec4(const Handle &handle, physx::PxVec4 &val) const;
    ErrorType getParamVec4Array(const Handle &handle, physx::PxVec4 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamVec4Array(const Handle &handle, const physx::PxVec4 *array, int32_t n, int32_t offset= 0);

    ErrorType setParamQuat(const Handle &handle, const physx::PxQuat &val);
    ErrorType getParamQuat(const Handle &handle, physx::PxQuat &val) const;
    ErrorType getParamQuatArray(const Handle &handle, physx::PxQuat *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamQuatArray(const Handle &handle, const physx::PxQuat *array, int32_t n, int32_t offset= 0);

    ErrorType setParamMat33(const Handle &handle, const physx::PxMat33 &val);
    ErrorType getParamMat33(const Handle &handle, physx::PxMat33 &val) const;
    ErrorType getParamMat33Array(const Handle &handle, physx::PxMat33 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamMat33Array(const Handle &handle, const physx::PxMat33 *array, int32_t n, int32_t offset= 0);

	ErrorType setParamMat34Legacy(const Handle &handle, const float (&val)[12]);
	ErrorType getParamMat34Legacy(const Handle &handle, float (&val)[12]) const;
    ErrorType getParamMat34LegacyArray(const Handle &handle, float *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamMat34LegacyArray(const Handle &handle, const float *array, int32_t n, int32_t offset = 0);

	ErrorType setParamMat44(const Handle &handle, const physx::PxMat44 &val);
    ErrorType getParamMat44(const Handle &handle, physx::PxMat44 &val) const;
    ErrorType getParamMat44Array(const Handle &handle, physx::PxMat44 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamMat44Array(const Handle &handle, const physx::PxMat44 *array, int32_t n, int32_t offset= 0);

	ErrorType setParamBounds3(const Handle &handle, const physx::PxBounds3 &val);
    ErrorType getParamBounds3(const Handle &handle, physx::PxBounds3 &val) const;
    ErrorType getParamBounds3Array(const Handle &handle, physx::PxBounds3 *array, int32_t n, int32_t offset = 0) const;
    ErrorType setParamBounds3Array(const Handle &handle, const physx::PxBounds3 *array, int32_t n, int32_t offset= 0);

	ErrorType setParamTransform(const Handle &handle, const physx::PxTransform &val);
	ErrorType getParamTransform(const Handle &handle, physx::PxTransform &val) const;
	ErrorType getParamTransformArray(const Handle &handle, physx::PxTransform *array, int32_t n, int32_t offset = 0) const;
	ErrorType setParamTransformArray(const Handle &handle, const physx::PxTransform *array, int32_t n, int32_t offset= 0);

    ErrorType valueToStr(const Handle &array_handle, char *str, uint32_t n, const char *&ret);
    ErrorType strToValue(Handle &handle,const char *str,const char **endptr); // assigns this string to the value

    ErrorType resizeArray(const Handle &array_handle, int32_t new_size);
    ErrorType getArraySize(const Handle &array_handle, int32_t &size, int32_t dimension = 0) const;
	ErrorType swapArrayElements(const Handle &array_handle, uint32_t firstElement, uint32_t secondElement);

    bool equals(const NvParameterized::Interface &obj, Handle* handleOfInequality, uint32_t numHandlesOfInequality, bool doCompareNotSerialized = true) const;

    bool areParamsOK(Handle *invalidHandles = NULL, uint32_t numInvalidHandles = 0);

	ErrorType copy(const NvParameterized::Interface &other);

	ErrorType clone(NvParameterized::Interface *&obj) const;

    static physx::PxVec2 		getVec2(const char *str, const char **endptr);
    static physx::PxVec3 		getVec3(const char *str, const char **endptr);
    static physx::PxVec4 		getVec4(const char *str, const char **endptr);
    static physx::PxQuat 		getQuat(const char *str, const char **endptr);
    static physx::PxMat33		getMat33(const char *str, const char **endptr);
    static void					getMat34Legacy(const char *str, const char **endptr, float (&val)[12]);
    static physx::PxMat44 		getMat44(const char *str, const char **endptr);
    static physx::PxBounds3     getBounds3(const char *str, const char **endptr);
    static physx::PxTransform   getTransform(const char *str, const char **endptr);

	static physx::PxVec2		init(float x,float y);
	static physx::PxVec3		init(float x,float y,float z);
	static physx::PxVec4		initVec4(float x,float y,float z,float w);
	static physx::PxQuat		init(float x,float y,float z,float w);
	static physx::PxMat33		init(float _11,float _12,float _13,float _21,float _22,float _23,float _31,float _32,float _33);
	static physx::PxMat44		init(float _11,float _12,float _13,float _14,float _21,float _22,float _23,float _24,float _31,float _32,float _33,float _34,float _41,float _42,float _43,float _44);
	static physx::PxBounds3		init(float minx,float miny,float minz,float maxx,float maxy,float maxz);
	static physx::PxTransform	init(float x,float y,float z,float qx,float qy,float qz,float qw);

	static int32_t MultIntArray(const int32_t *array, int32_t n);
	static ErrorType resizeArray(Traits *parameterizedTraits,
                                 void *&buf,
                                 int32_t *array_sizes,
                                 int32_t dimension,
                                 int32_t resize_dim,
                                 int32_t new_size,
								 bool doFree,
                                 int32_t element_size,
                                 uint32_t element_align,
								 bool &isMemAllocated);
	static void recursiveCopy(const void *src,
							  const int32_t *src_sizes,
							  void *dst,
							  const int32_t *dst_sizes,
							  int32_t dimension,
							  int32_t element_size,
							  int32_t *indexes = NULL,
							  int32_t level = 0);

	virtual void getVarPtr(const Handle &, void *&ptr, size_t &offset) const
	{
		PX_ALWAYS_ASSERT();
		ptr = 0;
		offset = 0;
	}

	Traits * getTraits(void) const
	{
		return mParameterizedTraits;
	}

	virtual bool checkAlignments() const;

protected:

	void *getVarPtrHelper(const ParamLookupNode *rootNode, void *paramStruct, const Handle &handle, size_t &offset) const;

	static void destroy(NvParameters *obj, NvParameterized::Traits *traits, bool doDeallocateSelf, int32_t *refCount, void *buf);

    // All classes deriving from NvParameterized must overload this function.  It
    // returns the parameter definition tree.  The root node must be a struct
    // with an empty string for its name.
	virtual const Definition *getParameterDefinitionTree(void) {return NULL;}
	virtual const Definition *getParameterDefinitionTree(void) const {return NULL;}

	ErrorType releaseDownsizedParameters(const Handle &handle, int newSize, int oldSize);
	ErrorType initNewResizedParameters(const Handle &handle, int newSize, int oldSize);
	virtual ErrorType rawResizeArray(const Handle &handle, int new_size);
	virtual ErrorType rawGetArraySize(const Handle &array_handle, int &size, int dimension) const;
	virtual ErrorType rawSwapArrayElements(const Handle &array_handle, unsigned int firstElement, unsigned int secondElement);

    // The methods for the types that are supported by the class deriving from
    // NvParameterized must be overloaded.
    virtual ErrorType rawSetParamBool(const Handle &handle, bool val);
    virtual ErrorType rawGetParamBool(const Handle &handle, bool &val) const;
    virtual ErrorType rawGetParamBoolArray(const Handle &handle, bool *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamBoolArray(const Handle &handle, const bool *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamString(const Handle &handle, const char *&val) const;
    virtual ErrorType rawSetParamString(const Handle &handle, const char *val);
	virtual ErrorType rawGetParamStringArray(const Handle &handle, char **array, int32_t n, int32_t offset) const;
	virtual ErrorType rawSetParamStringArray(const Handle &handle, const char **array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamEnum(const Handle &handle, const char *&val) const;
    virtual ErrorType rawSetParamEnum(const Handle &handle, const char *val);
	virtual ErrorType rawGetParamEnumArray(const Handle &handle, char **array, int32_t n, int32_t offset) const;
	virtual ErrorType rawSetParamEnumArray(const Handle &handle, const char **array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamRef(const Handle &handle, NvParameterized::Interface *&val) const;
    virtual ErrorType rawSetParamRef(const Handle &handle, NvParameterized::Interface * val);
	virtual ErrorType rawGetParamRefArray(const Handle &handle, NvParameterized::Interface **array, int32_t n, int32_t offset) const;
	virtual ErrorType rawSetParamRefArray(const Handle &handle, /*const*/ NvParameterized::Interface **array, int32_t n, int32_t offset);

	virtual ErrorType rawGetParamI8(const Handle &handle, int8_t &val) const;
    virtual ErrorType rawSetParamI8(const Handle &handle, int8_t val);
    virtual ErrorType rawGetParamI8Array(const Handle &handle, int8_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamI8Array(const Handle &handle, const int8_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamI16(const Handle &handle, int16_t &val) const;
    virtual ErrorType rawSetParamI16(const Handle &handle, int16_t val);
    virtual ErrorType rawGetParamI16Array(const Handle &handle, int16_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamI16Array(const Handle &handle, const int16_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamI32(const Handle &handle, int32_t &val) const;
    virtual ErrorType rawSetParamI32(const Handle &handle, int32_t val);
    virtual ErrorType rawGetParamI32Array(const Handle &handle, int32_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamI32Array(const Handle &handle, const int32_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamI64(const Handle &handle, int64_t &val) const;
    virtual ErrorType rawSetParamI64(const Handle &handle, int64_t val);
    virtual ErrorType rawGetParamI64Array(const Handle &handle, int64_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamI64Array(const Handle &handle, const int64_t *array, int32_t n, int32_t offset);


    virtual ErrorType rawGetParamU8(const Handle &handle, uint8_t &val) const;
    virtual ErrorType rawSetParamU8(const Handle &handle, uint8_t val);
    virtual ErrorType rawGetParamU8Array(const Handle &handle, uint8_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamU8Array(const Handle &handle, const uint8_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamU16(const Handle &handle, uint16_t &val) const;
    virtual ErrorType rawSetParamU16(const Handle &handle, uint16_t val);
    virtual ErrorType rawGetParamU16Array(const Handle &handle, uint16_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamU16Array(const Handle &handle, const uint16_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamU32(const Handle &handle, uint32_t &val) const;
    virtual ErrorType rawSetParamU32(const Handle &handle, uint32_t val);
    virtual ErrorType rawGetParamU32Array(const Handle &handle, uint32_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamU32Array(const Handle &handle, const uint32_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamU64(const Handle &handle, uint64_t val);
    virtual ErrorType rawGetParamU64(const Handle &handle, uint64_t &val) const;
    virtual ErrorType rawGetParamU64Array(const Handle &handle, uint64_t *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamU64Array(const Handle &handle, const uint64_t *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamF32(const Handle &handle, float &val) const;
    virtual ErrorType rawSetParamF32(const Handle &handle, float val);
    virtual ErrorType rawGetParamF32Array(const Handle &handle, float *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamF32Array(const Handle &handle, const float *array, int32_t n, int32_t offset);

    virtual ErrorType rawGetParamF64(const Handle &handle, double &val) const;
    virtual ErrorType rawSetParamF64(const Handle &handle, double val);
    virtual ErrorType rawGetParamF64Array(const Handle &handle, double *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamF64Array(const Handle &handle, const double *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamVec2(const Handle &handle,physx::PxVec2 val);
    virtual ErrorType rawGetParamVec2(const Handle &handle,physx::PxVec2 &val) const;
    virtual ErrorType rawGetParamVec2Array(const Handle &handle,physx::PxVec2 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamVec2Array(const Handle &handle, const physx::PxVec2 *array, int32_t n, int32_t offset);

	virtual ErrorType rawSetParamVec3(const Handle &handle,physx::PxVec3 val);
    virtual ErrorType rawGetParamVec3(const Handle &handle,physx::PxVec3 &val) const;
    virtual ErrorType rawGetParamVec3Array(const Handle &handle,physx::PxVec3 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamVec3Array(const Handle &handle, const physx::PxVec3 *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamVec4(const Handle &handle,physx::PxVec4 val);
    virtual ErrorType rawGetParamVec4(const Handle &handle,physx::PxVec4 &val) const;
    virtual ErrorType rawGetParamVec4Array(const Handle &handle,physx::PxVec4 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamVec4Array(const Handle &handle, const physx::PxVec4 *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamQuat(const Handle &handle,physx::PxQuat val);
    virtual ErrorType rawGetParamQuat(const Handle &handle,physx::PxQuat &val) const;
    virtual ErrorType rawGetParamQuatArray(const Handle &handle,physx::PxQuat *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamQuatArray(const Handle &handle, const physx::PxQuat *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamMat33(const Handle &handle,physx::PxMat33 val);
    virtual ErrorType rawGetParamMat33(const Handle &handle,physx::PxMat33 &val) const;
    virtual ErrorType rawGetParamMat33Array(const Handle &handle,physx::PxMat33 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamMat33Array(const Handle &handle, const physx::PxMat33 *array, int32_t n, int32_t offset);

    virtual ErrorType rawSetParamMat44(const Handle &handle,physx::PxMat44 val);
    virtual ErrorType rawGetParamMat44(const Handle &handle,physx::PxMat44 &val) const;
    virtual ErrorType rawGetParamMat44Array(const Handle &handle,physx::PxMat44 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamMat44Array(const Handle &handle, const physx::PxMat44 *array, int32_t n, int32_t offset);

	virtual ErrorType rawSetParamMat34Legacy(const Handle &handle,const float (&val)[12]);
    virtual ErrorType rawGetParamMat34Legacy(const Handle &handle,float (&val)[12]) const;
    virtual ErrorType rawGetParamMat34LegacyArray(const Handle &handle, float *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamMat34LegacyArray(const Handle &handle, const float *array, int32_t n, int32_t offset);

	virtual ErrorType rawSetParamBounds3(const Handle &handle,physx::PxBounds3 val);
    virtual ErrorType rawGetParamBounds3(const Handle &handle,physx::PxBounds3 &val) const;
    virtual ErrorType rawGetParamBounds3Array(const Handle &handle,physx::PxBounds3 *array, int32_t n, int32_t offset) const;
    virtual ErrorType rawSetParamBounds3Array(const Handle &handle, const physx::PxBounds3 *array, int32_t n, int32_t offset);

	virtual ErrorType rawSetParamTransform(const Handle &handle,physx::PxTransform val);
	virtual ErrorType rawGetParamTransform(const Handle &handle,physx::PxTransform &val) const;
	virtual ErrorType rawGetParamTransformArray(const Handle &handle,physx::PxTransform *array, int32_t n, int32_t offset) const;
	virtual ErrorType rawSetParamTransformArray(const Handle &handle, const physx::PxTransform *array, int32_t n, int32_t offset);

	// WARNING!
	// Binary deserializer relies on layout of fields
	// If you change anything be sure to update it as well

	Traits *mParameterizedTraits;
	SerializationCallback *mSerializationCb;
	void *mCbUserData;
	void *mBuffer;
	int32_t *mRefCount;
	const char *mName;
	const char *mClassName;
	bool mDoDeallocateSelf; //if true - memory should be deallocated in destroy()
	bool mDoDeallocateName; //if true - mName is in inplace-buffer and should not be freed
	bool mDoDeallocateClassName; //see comment for mDoDeallocateName

private:

	void initRandom(NvParameterized::Handle& handle);

    bool equals(const NvParameterized::Interface &obj,
                Handle &param_handle,
				Handle *handlesOfInequality,
				uint32_t numHandlesOfInequality,
				bool doCompareNotSerialized) const;

	bool checkAlignments(Handle &param_handle) const;

    bool areParamsOK(Handle &handle, Handle *invalidHandles, uint32_t numInvalidHandles, uint32_t &numRemainingHandles);

	ErrorType copy(const NvParameterized::Interface &other,
				   Handle &param_handle);

	// recursively call pre serialization callback
	ErrorType callPreSerializeCallback(Handle& handle) const;

    ErrorType checkParameterHandle(const Handle &handle) const;

};

} // end of namespace

#endif // PX_PARAMETERS_H
