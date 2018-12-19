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

#ifndef NV_PARAM_UTILS_H
#define NV_PARAM_UTILS_H

#include "NvParameterized.h"

// utility methods to operate on NvParameterized data.

namespace NvParameterized
{

/**
\brief Recursively finds a parameter with the given name

This method will recursively search through not only this parameterized but all referenced
parameterized objects as well.  It sets the handle and returns the NvParameterized::Interface in which the name was found.

\param i the parameterized object that will be searched

\param longName	contains the name of the parameter to be found
the longName will work with arrays, structs, and included references

\param outHandle will contain the output handle that provides read-only access to the specified parameter

\returns the NvParameterized::Interface pointer in which the parameter is contained (this could be different than the top level NvParameterized::Interface if the parameter is contained in an included reference)
*/
PX_INLINE const Interface * findParam(const Interface &i,const char *longName, Handle &outHandle);


/**
\brief Recursively finds a parameter with the given name

This method will recursively search through not only this parameterized but all referenced
parameterized objects as well.  It sets the handle and returns the NvParameterized::Interface in which the name was found.

\param i the parameterized object that will be searched

\param longName	contains the name of the parameter to be found
the longName will work with arrays, structs, and included references

\param outHandle will contain the output handle that provides read-write access to the specified parameter

\returns the NvParameterized::Interface pointer in which the parameter is contained (this could be different than the top level NvParameterized::Interface if the parameter is contained in an included reference)
*/
PX_INLINE Interface * findParam(Interface &i,const char *longName, Handle &outHandle);

/**
\brief Container for results of getParamList
*/
struct ParamResult
{
public:
	/**
	\brief Constructor
	*/
	ParamResult(const char *name,
				const char *longName,
				const char *className,
				const char *instanceName,
				const Handle &handle,
				int32_t arraySize,
				DataType type)
		: mArraySize(arraySize),
		mInstanceName(instanceName),
		mClassName(className),
		mName(name),
		mLongName(longName),
		mHandle(handle),
		mDataType(type)
	{}

	/**
	\brief size of array (if parameter is array)
	*/
	int32_t mArraySize;

	/**
	\brief Name of parameter's parent object
	*/
	const char *mInstanceName;

	/**
	\brief Name of NvParameterized-class of parameter's parent object
	*/
	const char *mClassName;

	/**
	\brief The name of the parameter
	*/
	const char *mName;

	/**
	\brief The fully qualified 'long' name of the parameter
	*/
	const char *mLongName;

	/**
	\brief Use this handle to access the parameter in question
	*/
	Handle mHandle;

	/**
	\brief The type of parameter
	*/
	DataType mDataType;
};

/**
\brief A helper method to retrieve the list of all parameters relative to an interface.

\param [in]  i the input interface 
\param [in]  className is an optional class name to match to.  If this is null, it will return all parameters.
\param [in]  paramName is an optional parameter name to match to.  If this is null, it will return all parameters.
\param [out] count the number of parameters found
\param [in]  recursive if true the search will recurse into every included reference, not just the top parameterized class
\param [in]  classesOnly if true the search will only match against class names
\param [in]  traits typically the APEX traits class, used to allocate the ParamResult, see ApexSDK::getParameterizedTraits()

\note The return pointer is allocated by the NvParameterized Traits class and should be freed by calling releaseParamList()

*/
PX_INLINE const ParamResult * getParamList(const Interface &i,
								 const char *className,	// name of class to match
								 const char *paramName, // name of parameter to match
								 uint32_t &count,
								 bool recursive,
								 bool classesOnly,
								 NvParameterized::Traits *traits);

/// helper function to free the parameter list returned from getParamList
PX_INLINE void				releaseParamList(uint32_t resultCount,const ParamResult *results,NvParameterized::Traits *traits);

/// helper function to get an NvParameterized array size
PX_INLINE bool getParamArraySize(const Interface &pm, const char *name, int32_t &arraySize);

/// helper function to resize an NvParameterized array
PX_INLINE bool resizeParamArray(Interface &pm, const char *name, int32_t newSize);

/**
\brief Callback container for getNamedReferences
*/
class NamedReferenceInterface
{
public:
	/**
	\brief Destructor
	*/
	virtual ~NamedReferenceInterface() {}
	/**
	\brief Callback

	Calls back to the user with any named reference (not included references) in the NvParameterized::Interface
	If the user returns a NULL pointer, than the original named reference is left alone.
	If the user returns a non-NULL pointer, than the named reference is replaced with the 'const char *' returned.
	*/
	virtual const char * namedReferenceCallback(const char *className,const char *namedReference,Handle &handle) = 0;
};

/// Calls back for every non-included named reference.
PX_INLINE uint32_t getNamedReferences(const Interface &i,
										  NamedReferenceInterface &namedReference,
										  bool recursive);

/**
\brief Callback container for getReferences
*/
class ReferenceInterface
{
public:
	/**
	\brief Destructor
	*/
	virtual ~ReferenceInterface() {}
	/**
	\brief Callback

	Calls back to the user with any reference (named or included or both) in the NvParameterized::Interface.
	*/
	virtual void referenceCallback(Handle &handle) = 0;
};

/// Calls back for every reference (named or included or both).
PX_INLINE void getReferences(const Interface &iface,
										  ReferenceInterface &cb,
										  bool named,
										  bool included,
										  bool recursive);

/// helper function to get an NvParameterized value
PX_INLINE bool getParamBool(const Interface &pm, const char *name, bool &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamBool(Interface &pm, const char *name, bool val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamString(const Interface &pm, const char *name, const char *&val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamString(Interface &pm, const char *name, const char *val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamEnum(const Interface &pm, const char *name, const char *&val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamEnum(Interface &pm, const char *name, const char *val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamRef(const Interface &pm, const char *name, NvParameterized::Interface *&val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamRef(Interface &pm, const char *name, NvParameterized::Interface *val, bool doDestroyOld = false) ;

/// helper function to init an NvParameterized value
PX_INLINE bool initParamRef(Interface &pm, const char *name, const char *className, bool doDestroyOld = false);
/// helper function to init an NvParameterized value
PX_INLINE bool initParamRef(Interface &pm, const char *name, const char *className, const char *objName, bool doDestroyOld = false);

/// helper function to get an NvParameterized value
PX_INLINE bool getParamI8(const Interface &pm, const char *name, int8_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamI8(Interface &pm, const char *name, int8_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamI16(const Interface &pm, const char *name, int16_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamI16(Interface &pm, const char *name, int16_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamI32(const Interface &pm, const char *name, int32_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamI32(Interface &pm, const char *name, int32_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamI64(const Interface &pm, const char *name, int64_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamI64(Interface &pm, const char *name, int64_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamU8(const Interface &pm, const char *name, uint8_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamU8(Interface &pm, const char *name, uint8_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamU16(const Interface &pm, const char *name, uint16_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamU16(Interface &pm, const char *name, uint16_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamU32(const Interface &pm, const char *name, uint32_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamU32(Interface &pm, const char *name, uint32_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamU64(const Interface &pm, const char *name, uint64_t &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamU64(Interface &pm, const char *name, uint64_t val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamF32(const Interface &pm, const char *name, float &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamF32(Interface &pm, const char *name, float val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamF64(const Interface &pm, const char *name, double &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamF64(Interface &pm, const char *name, double val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamVec2(const Interface &pm, const char *name, physx::PxVec2 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamVec2(Interface &pm, const char *name, const physx::PxVec2 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamVec3(const Interface &pm, const char *name, physx::PxVec3 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamVec3(Interface &pm, const char *name, const physx::PxVec3 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamVec4(const Interface &pm, const char *name, physx::PxVec4 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamVec4(Interface &pm, const char *name, const physx::PxVec4 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamQuat(const Interface &pm, const char *name, physx::PxQuat &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamQuat(Interface &pm, const char *name, const physx::PxQuat &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamMat33(const Interface &pm, const char *name, physx::PxMat33 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamMat33(Interface &pm, const char *name, const physx::PxMat33 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamMat44(const Interface &pm, const char *name, physx::PxMat44 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamMat44(Interface &pm, const char *name, const physx::PxMat44 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamBounds3(const Interface &pm, const char *name, physx::PxBounds3 &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamBounds3(Interface &pm, const char *name, const physx::PxBounds3 &val) ;

/// helper function to get an NvParameterized value
PX_INLINE bool getParamTransform(const Interface &pm, const char *name, physx::PxTransform &val);
/// helper function to set an NvParameterized value
PX_INLINE bool setParamTransform(Interface &pm, const char *name, const physx::PxTransform &val) ;

} // namespace NvParameterized


#include "NvParamUtils.inl"

#endif // NV_PARAM_UTILS_H
