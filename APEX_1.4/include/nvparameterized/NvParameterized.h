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
 
#ifndef NV_PARAMETERIZED_H
#define NV_PARAMETERIZED_H

/**
 * APEX uses a framework called NvParameterized for storing asset and actor data. 
 * NvParameterized classes provide reflection on every parameter they include making it 
 * effective for serialization and auto-generation of user interfaces for tools.<br>
 *
 * NvParameterized stores data in C++ classes which are auto-generated from their 
 * descriptions written in a special domain specific language (DSL). Description files 
 * contain information on internal layout and various high-level metadata of corresponding 
 * classes and their members. All generated classes implement special interface for 
 * run time reflection and modification of data.
 * */

/*!
\file
\brief NvParameterized classes
*/

#include "foundation/PxVec2.h"
#include "foundation/PxVec3.h"
#include "foundation/PxVec4.h"
#include "foundation/PxQuat.h"
#include "foundation/PxBounds3.h"
#include "foundation/PxMat44.h"
#include <stdio.h>
#include <stdarg.h>
#include <new> // for placement new

PX_PUSH_PACK_DEFAULT

//! \brief NvParameterized namespace
namespace NvParameterized
{

class Interface;
class Traits;

/**
\brief Various errors that may be returned from NvParameterized calls
*/
enum ErrorType
{
   ERROR_NONE = 0,
   ERROR_TYPE_NOT_SUPPORTED,
   ERROR_INDEX_OUT_OF_RANGE,
   ERROR_INVALID_PARAMETER_NAME,
   ERROR_INVALID_PARAMETER_HANDLE,
   ERROR_CAST_FAILED,
   ERROR_INVALID_ENUM_VAL,
   ERROR_SYNTAX_ERROR_IN_NAME,
   ERROR_IS_LEAF_NODE,
   ERROR_RESULT_BUFFER_OVERFLOW,
   ERROR_NAME_DOES_NOT_MATCH_DEFINITION,
   ERROR_NOT_AN_ARRAY,
   ERROR_ARRAY_SIZE_IS_FIXED,
   ERROR_ARRAY_RESIZING_IS_NOT_SUPPORTED,
   ERROR_ARRAY_IS_TOO_SMALL,
   ERROR_INVALID_ARRAY_DIMENSION,
   ERROR_INVALID_ARRAY_SIZE,
   ERROR_PARAMETER_HANDLE_DOES_NOT_MATCH_CLASS,
   ERROR_MEMORY_ALLOCATION_FAILURE,
   ERROR_INVALID_REFERENCE_INCLUDE_HINT,
   ERROR_INVALID_REFERENCE_VALUE,
   ERROR_PARAMETER_HANDLE_NOT_INITIALIZED,
   ERROR_PARAMETER_DEFINITIONS_DO_NOT_MATCH,
   ERROR_HANDLE_MISSING_INTERFACE_POINTER,
   ERROR_HANDLE_INVALID_INTERFACE_POINTER,
   ERROR_INVALID_CALL_ON_NAMED_REFERENCE,
   ERROR_NOT_IMPLEMENTED,
   ERROR_OBJECT_CONSTRUCTION_FAILED,
   ERROR_MODIFY_CONST_HANDLE
};

/**
\brief These types are supported in NvParameterized schemas
\warning Do not change values of enums!
*/    
enum DataType
{
    TYPE_UNDEFINED = 0,

    /**
    \brief Array type, size may be static or dynamic
    \see Definition::arraySizeIsFixed(), Handle::resizeArray(), Handle::getArraySize()
    */
    TYPE_ARRAY = 1,

    TYPE_STRUCT = 2,

    TYPE_BOOL = 3,
    /**
    \brief String type, represented by a const char pointer
    \see Handle::getParamString(), Handle::setParamString()
    */
    TYPE_STRING = 4,

    /**
    \brief Enum type, represented by a const char pointer
    \see Definition::numEnumVals(), Definition::enumVal(), Handle::getParamEnum(), Handle::setParamEnum()
    */
    TYPE_ENUM = 5,

    /**
    \brief Reference type, may be a named or included reference

    References are intended to be used in instances where a class needs either a 
    named reference (an emitter asset references an IOFX and IOS asset), or an 
    included reference (a destructible asset that serializes an APEX render mesh).
    References may also used to create a unions within a class.  Each reference will
    contain one or more variants to allow for unions.
        
    \see Handle::initParamRef(), Definition::numRefVariants(), Definition::refVariantVal(), Handle::initParamRef(), Handle::getParamRef(), Handle::setParamRef()
    */
    TYPE_REF = 6,

    TYPE_I8 = 7,
    TYPE_I16 = 8,
    TYPE_I32 = 9,
    TYPE_I64 = 10,

    TYPE_U8 = 11,
    TYPE_U16 = 12,
    TYPE_U32 = 13,
    TYPE_U64 = 14,

    TYPE_F32 = 15,
    TYPE_F64 = 16,

    TYPE_VEC2 = 17,
    TYPE_VEC3 = 18,
    TYPE_VEC4 = 19,
    TYPE_QUAT = 20,
    TYPE_MAT33 = 21,
    TYPE_BOUNDS3 = 23,
    TYPE_MAT44 = 24,

    TYPE_POINTER = 25,

    TYPE_TRANSFORM = 26,

	TYPE_MAT34 = 27,

    TYPE_LAST
};

/**
\brief Provides hints about the parameter definition

\see Definition
*/
class Hint
{
    public:

    /**
    \brief Destructor
    */
    virtual ~Hint() {}
    /**
    \brief Returns the name of the hint
    */
    virtual const char *    name(void) const = 0;

    /**
    \brief Returns the type of the hint
    */    
    virtual DataType         type(void) const = 0;

    /**
    \brief Returns the unsigned 64-bit value of the hint
    \note Undefined results if the type != TYPE_U64
    */
    virtual    uint64_t             asUInt(void) const = 0;
    
    /**
    \brief Returns the 64-bit floating point value of the hint
    \note Undefined results if the type != TYPE_FU64
    */    
    virtual double             asFloat(void) const = 0;

    /**
    \brief Returns the const character pointer for hint
    \note Undefined results if the type != TYPE_STRING
    */
    virtual const char *    asString(void) const = 0;

    /**
    \brief Set the value if the hint is a 64bit unsigned type
    */
	virtual bool setAsUInt(uint64_t v) = 0;

    private:
};

/**
\brief Provides information about a parameter
*/
class Definition
{
public:

    /**
    \brief Destructor
    */
    virtual ~Definition() {}

    /**
    \brief Destroys the Definition object and all nested dynamic objects contained within
    */
    virtual void destroy() = 0;

    /**
    \brief Returns the number of hints in the parameter Definition
    */
    virtual int32_t                             numHints(void) const = 0;

    /**
    \brief Returns the Hint located at the index
    \returns NULL if index >= Hint::numHints()
    */
    virtual const Hint *                    hint(int32_t index) const = 0;

    /**
    \brief Returns the Hint that matches the input name
    \returns NULL if name does not match any of the Definition's hints
    */
    virtual const Hint *                    hint(const char *name) const = 0;

	/**
	\brief Store parameter hints
	\warning Only for internal use
	*/
    virtual void                            setHints(const Hint **hints, int32_t n) = 0;

	/**
	\brief Add parameter hint
	\warning Only for internal use
	*/
	virtual void                            addHint(Hint *hint) = 0;
    
    /**
    \brief Returns this Definition's parent Definition
    \returns NULL if at the top level
    */
    virtual const Definition *    parent(void) const = 0;

    /**
    \brief Returns this Definition's top most ancestor
    */
    virtual const Definition *    root(void) const = 0;

    /**
    \brief Returns the name of the parameter 
    */
    virtual const char *                    name(void) const = 0;

    /**
    \brief Returns the long name of the parameter 
    If the parameter is a member of a struct or an array the long name will contain these
    parent names
    */
    virtual const char *                    longName(void) const = 0;

    /**
    \brief Returns the name of parameter's struct type
	\returns NULL if not struct
    */
    virtual const char *                    structName(void) const = 0;

	/**
    \brief Returns the parameter type
    */
    virtual DataType                        type(void) const = 0;

	/**
	\brief Return the parameter type in string form
	*/
	virtual const char*						typeString() const = 0;

    /**
    \brief Returns the number of variants this parameter could be
    A reference is sometimes a union of different types, each different type is referred to as 
    a "variant". Variants can be used in either included or named references.
    */
    virtual    int32_t                             numRefVariants(void) const = 0;

    /**
    \brief Given the ref variant name, get its val index
    \returns -1 if input ref_val is not found
    */
    virtual    int32_t                             refVariantValIndex( const char * ref_val ) const = 0;

    /**
    \brief Get the string value of the reference variant
    */
    virtual    const char *                     refVariantVal(int32_t index) const = 0;

    /**
    \brief Returns the number of enums for the parameter
    */
    virtual int32_t                             numEnumVals(void) const = 0;

    /**
    \brief Given the enum string, get the enum val index
    \returns -1 if input enum_val is not found
    */
    virtual    int32_t                             enumValIndex( const char * enum_val ) const = 0;

    /**
    \brief Returns the Enum string located at the index
    \returns NULL if index >= Hint::numEnumVals()
    */
    virtual const char *                    enumVal(int32_t index) const = 0;

	/**
	\brief Store possible enum values for TYPE_ENUM parameter
	\warning Only for internal use
	*/
    virtual void                             setEnumVals(const char **enum_vals, int32_t n) = 0;

	/**
	\brief Add new enum value to a list of possible enum values for TYPE_ENUM parameter
	\warning Only for internal use
	*/
    virtual void                             addEnumVal(const char *enum_val) = 0;

    /**
    \brief Returns custom alignment if parameter uses it; otherwise returns 0
    */
    virtual uint32_t alignment(void) const = 0;

    /**
    \brief Returns custom padding if parameter uses it; otherwise returns 0
    */
    virtual uint32_t padding(void) const = 0;

	/**
    \brief Returns the number of dimensions of a static array
    */
    virtual int32_t arrayDimension(void) const = 0;

    /**
    \brief Returns the array size of a static array
    \returns 0 for dynamic arrays
    */
    virtual int32_t arraySize(int32_t dimension = 0) const = 0;

    /**
    \brief Used to determine if an array is static or dynamic
    */
    virtual bool arraySizeIsFixed(void) const = 0;

	/**
	\brief Set size of static array
	\warning Only for internal use
	*/
    virtual bool setArraySize(int32_t size) = 0; // -1 if the size is not fixed

	/**
	\brief Used to determine if parameter is aggregate (TYPE_STRUCT or TYPE_ARRAY) or not.
	*/
    virtual bool isLeaf(void) const = 0;
    
    /**
    \brief Used to determine if reference is included or not.
    */
    virtual bool isIncludedRef(void) const = 0;

    /**
    \brief Returns the number of children (for a struct or static array)
    */
    virtual int32_t numChildren(void) const = 0;

	/**
	\brief Access definition of i-th child parameter.
	*/
    virtual const Definition *child(int32_t index) const = 0;

	/**
	\brief Access definition of child parameter with given name
	\warning Only used with TYPE_STRUCT
	*/
    virtual const Definition *child(const char *name, int32_t &index) const = 0;

	/**
	\brief Store definitions of child parameters
	\warning Only for internal use
	*/
	virtual void setChildren(Definition **children, int32_t n) = 0;

	/**
	\brief Add definition of one morechild parameter
	\warning Only for internal use
	*/
    virtual void addChild(Definition *child) = 0;

    /**
    \brief Set indices of child handles which must be released when downsizing array
    */
    virtual void setDynamicHandleIndicesMap(const uint8_t *indices, uint32_t numIndices) = 0;

    /**
    \brief Get indices of child handles which must be released when downsizing array
    */
    virtual const uint8_t * getDynamicHandleIndicesMap(uint32_t &outNumIndices) const = 0;

    /**
    \brief Used to determine whether type is not an aggregate (array, ref or struct)
    \param [in] simpleStructs structure of simple types is also considered simple
    \param [in] simpleStrings strings are considered simple
    */
    virtual bool isSimpleType(bool simpleStructs = true, bool simpleStrings = true) const = 0;
};

/**
\brief Provides access to individual parameters within the NvParameterized object
*/
class Handle
{
public:

    enum { MAX_DEPTH = 16 };

    /**
    \brief The constructor takes a pointer (if the user requires an instance before the ::NvParameterized::Interface exists)
    */
    PX_INLINE Handle(::NvParameterized::Interface *iface);

    /**
    \brief The constructor takes a reference
    */
    PX_INLINE Handle(::NvParameterized::Interface &iface);

    /**
    \brief The constructor takes a const interface
    \note Handles which are constructed from const objects are not allowed to modify them.
    */
    PX_INLINE Handle(const ::NvParameterized::Interface &iface);

    /**
    \brief Copy constructor
    */
    PX_INLINE Handle(const Handle &param_handle);

    /**
    \brief This constructor allows the user to create a handle that initially points at a particular parameter in the instance
    */
    PX_INLINE Handle(::NvParameterized::Interface &instance, const char *longName);

    /**
    \brief This constructor allows the user to create a handle that initially points at a particular parameter in the instance
    \note Handles which are constructed from const objects are not allowed to modify them.
    */
    PX_INLINE Handle(const ::NvParameterized::Interface &instance, const char *longName);

	/**
    \brief Get the parameter Definition for the handle's parameter
    */
    PX_INLINE const Definition *parameterDefinition(void) const { return(mParameterDefinition); }
    
    /**
    \brief Get the depth of the handle within the NvParameterized object
    */
    PX_INLINE int32_t numIndexes(void) const { return(mNumIndexes); }

    /**
    \brief Get the index at the specified depth within the NvParameterized object
    */
    PX_INLINE int32_t index(int32_t i) const { return(mIndexList[i]); }

    /**
    \brief Reduce the handle's depth within the NvParameterized object
    */
    PX_INLINE int32_t popIndex(int32_t levels = 1);

	/**
     \brief Set the Handle so it references a parameter in the specified instance with the name child_long_name
    */
    PX_INLINE ErrorType set(const ::NvParameterized::Interface *instance, const Definition *root, const char *child_long_name);

	/**
    \brief Set the Handle so it references a parameter in the specified instance with the name child_long_name
    */
   	PX_INLINE ErrorType set(const ::NvParameterized::Interface *instance, const char *child_long_name);
    
    /**
    \brief Get the parameter specified by longName and set the Handle to point it
    Given a long name like "mystruct.somearray[10].foo", it will return
    a handle to that specific parameter.  The handle can then be used to
    set/get values, as long as it's a handle to a leaf node.
    \note this method will not work if an included reference's child is included in the longName
    */
    PX_INLINE ErrorType getParameter(const char *longName);

    /**
    \brief Set the depth of the handle within the handle's parameter

    The set method is useful for accessing indices within an array of parameters or members
    within a struct.
    */
    PX_INLINE ErrorType set(int32_t child_index);

    /**
    \brief Get a child handle of this handle
    Sets handle to point to a child of this handle, with child_long_name being
    relative to the current long name.
    */
    PX_INLINE ErrorType getChildHandle(const ::NvParameterized::Interface *instance, const char *child_long_name, Handle &outHandle);

	/**
    \brief Get a child handle of this handle
    Sets handle to point to a direct child of this handle.  Works with structs and arrays.
    */
    PX_INLINE ErrorType getChildHandle(int32_t index, Handle &outHandle);

    /**
    \brief Returns the long name of the parameter, with indexes into str.
    \returns false if the long name didn't fit in str.
    */
    PX_INLINE bool getLongName(char *str, uint32_t max_str_len) const;

    /**
    \brief Reset all of the state data for the handle
    */
    PX_INLINE void reset();

	/**
	\brief Does handle correspond to an existing Interface?
	*/
    PX_INLINE bool isConst(void) const { return(mIsConst); }

	/**
	\brief Does handle correspond to a valid parameter?
	*/
    PX_INLINE bool isValid(void) const { return(mIsValid); }

	/**
	\brief Same as isValid
	*/
    PX_INLINE operator bool() const { return(isValid()); }

	/**
	\brief Return user data stored in handle
	*/
    PX_INLINE const void *userData(void) const { return(mUserData); }

	/**
	\brief Return user data stored in handle
	*/
    PX_INLINE void *userData(void) { return(mUserData); }

	/**
	\brief Store user data in handle
	*/
    PX_INLINE void setUserData(void *user_data) { mUserData = user_data; }

	/**
	\brief Get associated NvParameterized object
	\note Will return NULL in case of const handle (use getConstInterface instead)
	*/
	PX_INLINE ::NvParameterized::Interface * getInterface(void) const { return mIsConst ? 0 : mInterface; }

	/**
	\brief Get associated NvParameterized object
	*/
    PX_INLINE const ::NvParameterized::Interface * getConstInterface(void) const { return mInterface; }

	/**
	\brief Set associated NvParameterized object
	*/
    PX_INLINE void setInterface(::NvParameterized::Interface *iface) { mIsConst = false; mInterface = iface; }

	/**
	\brief Set associated NvParameterized object
	*/
	PX_INLINE void setInterface(const ::NvParameterized::Interface *iface) { mIsConst = true; mInterface = (::NvParameterized::Interface *)iface; }

    /**
    \brief Initialize a Reference parameter

    \param [in] chosenRefStr This string must be one of the strings returned from 
    Definition::refVariantVal()

	\param [in] doDestroyOld Sets whether the previous object should be destroyed or not

    \see Interface::initParamRef(), Definition::numRefVariants(), Definition::refVariantVal()
    */
    PX_INLINE ErrorType initParamRef(const char *chosenRefStr = 0, bool doDestroyOld = false);

    /**
    \brief Store this Handle's parameter to a string
    \param [in] buf this buffer is used to store any strings that need to be constructed
    \param [in] bufSize size of buf
    \param [out] ret this contains a pointer to the value as a string, it may or may not 
    be equal to buf, depending if the value had to be constructed dynamically or if it already exists as a static string
    */
    PX_INLINE ErrorType valueToStr(char *buf, uint32_t bufSize, const char *&ret);
    
    /**
    \brief Store the string to this Handle's parameter
    \returns ERROR_TYPE_NOT_SUPPORTED if the Handle's parameter type is an array, struct, or reference
    \returns ERROR_NONE if the Handle's parameter type is a simple type (u32, i32, vec3, etc)
    */
    PX_INLINE ErrorType strToValue(const char *str, const char **endptr);

    /**
    \brief Resize the array that the Handle points to
    */
    ErrorType resizeArray(int32_t new_size);

	/**
    \brief Get the array size for the given array dimension
	*/
    PX_INLINE ErrorType getArraySize(int32_t &size, int32_t dimension = 0) const;

	/**
	\brief Swap two elements of an array
	*/
    PX_INLINE ErrorType swapArrayElements(uint32_t firstElement, uint32_t secondElement);

    // These functions wrap the raw(Get|Set)XXXXX() methods.  They deal with
    // error handling and casting.
	
	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamBool(bool &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamBool(bool val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamBoolArray(bool *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamBoolArray(const bool *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamString(const char *&val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamString(const char *val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamStringArray(char **array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamStringArray(const char **array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamEnum(const char *&val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamEnum(const char *val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamEnumArray(char **array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamEnumArray(const char **array, int32_t n, int32_t offset = 0) ;

	/**
	\see Interface::getParamRef()
	*/
    PX_INLINE ErrorType getParamRef(::NvParameterized::Interface *&val) const ;

	/**
	\see Interface::getParamRefArray()
	*/
    PX_INLINE ErrorType getParamRefArray(::NvParameterized::Interface **array, int32_t n, int32_t offset = 0) const ;

	/**
	\see Interface::setParamRef()
	*/
    PX_INLINE ErrorType setParamRef(::NvParameterized::Interface * val, bool doDestroyOld = false) ;

	/**
	\see Interface::setParamRefArray()
	*/
    PX_INLINE ErrorType setParamRefArray(::NvParameterized::Interface **array, int32_t n, int32_t offset = 0, bool doDestroyOld = false) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI8(int8_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI8(int8_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI8Array(int8_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI8Array(const int8_t *val, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI16(int16_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI16(int16_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI16Array(int16_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI16Array(const int16_t *val, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI32(int32_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI32(int32_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI32Array(int32_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI32Array(const int32_t *val, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI64(int64_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI64(int64_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamI64Array(int64_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamI64Array(const int64_t *val, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU8(uint8_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU8(uint8_t val) ;
	
	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU8Array(uint8_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU8Array(const uint8_t *val, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU16(uint16_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU16(uint16_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU16Array(uint16_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU16Array(const uint16_t *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU32(uint32_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU32(uint32_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU32Array(uint32_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU32Array(const uint32_t *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU64(uint64_t &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU64(uint64_t val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamU64Array(uint64_t *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamU64Array(const uint64_t *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamF32(float &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamF32(float val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamF32Array(float *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamF32Array(const float *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamF64(double &val) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamF64(double val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamF64Array(double *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamF64Array(const double *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamVec2(const physx::PxVec2 &val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamVec2(physx::PxVec2 &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamVec2Array(physx::PxVec2 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamVec2Array(const physx::PxVec2 *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType setParamVec3(const physx::PxVec3 &val) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType getParamVec3(physx::PxVec3 &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamVec3Array(physx::PxVec3 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamVec3Array(const physx::PxVec3 *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType setParamVec4(const physx::PxVec4 &val) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType getParamVec4(physx::PxVec4 &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamVec4Array(physx::PxVec4 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamVec4Array(const physx::PxVec4 *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType setParamQuat(const physx::PxQuat &val) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType getParamQuat(physx::PxQuat &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamQuatArray(physx::PxQuat *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamQuatArray(const physx::PxQuat *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType setParamMat33(const physx::PxMat33 &val) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType getParamMat33(physx::PxMat33 &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamMat33Array(physx::PxMat33 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamMat33Array(const physx::PxMat33 *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamMat44(const physx::PxMat44 &val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamMat44(physx::PxMat44 &val) const ;
    
	/**
	\brief Get param
	*/
	PX_INLINE ErrorType getParamMat44Array(physx::PxMat44 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamMat44Array(const physx::PxMat44 *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamMat34Legacy(const float (&val)[12]) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamMat34Legacy(float (&val)[12]) const ;
    
	/**
	\brief Get param
	*/
	PX_INLINE ErrorType getParamMat34LegacyArray(float* array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamMat34LegacyArray(const float *array, int32_t n, int32_t offset = 0) ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamBounds3(const physx::PxBounds3 &val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamBounds3(physx::PxBounds3 &val) const ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamBounds3Array(physx::PxBounds3 *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamBounds3Array(const physx::PxBounds3 *array, int32_t n, int32_t offset = 0) ;

		/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamTransform(const physx::PxTransform &val) ;

	/**
	\brief Get param
	*/
    PX_INLINE ErrorType getParamTransform(physx::PxTransform &val) const ;
    
	/**
	\brief Get param
	*/
	PX_INLINE ErrorType getParamTransformArray(physx::PxTransform *array, int32_t n, int32_t offset = 0) const ;

	/**
	\brief Set param
	*/
    PX_INLINE ErrorType setParamTransformArray(const physx::PxTransform *array, int32_t n, int32_t offset = 0) ;


	/// Template version of setParamXxx
    template <typename T> PX_INLINE ErrorType setParam(const T &val);

	/// Template version of getParamXxx
    template <typename T> PX_INLINE ErrorType getParam(T &val) const;

	/// Template version of getParamXxxArray
    template <typename T> PX_INLINE ErrorType getParamArray(T *array, int32_t n, int32_t offset = 0) const;

	/// Template version of setParamXxxArray
    template <typename T> PX_INLINE ErrorType setParamArray(const T *array, int32_t n, int32_t offset = 0);

private:

    PX_INLINE void pushIndex(int32_t index);

    bool mIsValid, mIsConst;
    int32_t mNumIndexes;
    int32_t mIndexList[MAX_DEPTH];
    const Definition *mParameterDefinition;
    void *mUserData;
    ::NvParameterized::Interface    *mInterface;
};

/// A callback class for notification just prior to serialization
class SerializationCallback
{
public:
    virtual ~SerializationCallback() {}
	/// Callback method
    virtual void preSerialize(void* userData = NULL) = 0;
};


/**
\brief Represents the interface to the NvParameterized object
*/
class Interface
{
    friend class Handle;
public:

    /**
    \brief Destructor
    */
    virtual ~Interface() {}

    /**
    \brief Destroys the NvParameterized object and all nested dynamic objects contained within
    */
    virtual void destroy() = 0;

    /**
    \brief Initializes all parameters to their default value
    */
    virtual void initDefaults(void) = 0;

    /**
    \brief Initializes all parameters with random values
    */
    virtual void initRandom(void) = 0;

    /**
    \brief Get the class name
    */
    virtual const char * className(void) const = 0;
    
    /**
    \brief Sets the class name
    
    This method is used for named references. The input name should be one of the possible variants.
    \see Definition::numRefVariants(), Definition::refVariantVal()
    */
    virtual void setClassName(const char *name) = 0;

    /**
    \brief Get the name of the NvParameterized object
    This method is used for named references. The name will typically specify an asset to be loaded.
    */
    virtual const char * name(void) const = 0;
    
    /**
    \brief Set the name of the NvParameterized object
    This method is used for named references. The name will typically specify an asset to be loaded.
    */
    virtual void setName(const char *name) = 0;

    /**
    \brief Get the class version
    */
    virtual uint32_t version(void) const = 0;

    /**
    \brief Get the major part of class version
    */
	virtual uint16_t getMajorVersion(void) const = 0;

    /**
    \brief Get the minor part of class version
    */
	virtual uint16_t getMinorVersion(void) const = 0;

    /**
    \brief Get the class checksum.
    \param bits contains the number of bits contained in the checksum
    \returns A pointer to a constant array of uint32_t values representing the checksum
    */    
    virtual const uint32_t * checksum(uint32_t &bits) const = 0;

    /**
    \brief Get the number of parameters contained in the NvParameterized object
    */
    virtual int32_t numParameters(void) = 0;

    /**
    \brief Get the definition of i-th parameter
    */
    virtual const Definition *parameterDefinition(int32_t index) = 0;

    /**
    \brief Get definition of root structure
    */
    virtual const Definition *rootParameterDefinition(void) = 0;

    /**
    \brief Get definition of root structure
    */
	virtual const Definition *rootParameterDefinition(void) const = 0;

    /**
    \brief Set the Handle to point to the parameter specified by longName
    Given a long name like "mystruct.somearray[10].foo", it will return
    a handle to that specific parameter.  The handle can then be used to
    set/get values, as long as it's a handle to a leaf node.
    \note this method will not work if an included reference's child is included in the longName
    */
    virtual ErrorType getParameterHandle(const char *longName, Handle  &handle) const = 0;

    /**
    \brief Set the Handle to point to the parameter specified by longName
    Given a long name like "mystruct.somearray[10].foo", it will return
    a handle to that specific parameter.  The handle can then be used to
    set/get values, as long as it's a handle to a leaf node.
    \note this method will not work if an included reference's child is included in the longName
    */
    virtual ErrorType getParameterHandle(const char *longName, Handle  &handle) = 0;

    /// An application may set a callback function that is called immediately before serialization
    virtual void setSerializationCallback(SerializationCallback *cb, void *userData = NULL) = 0;

	/// Called prior by Serializer to serialization
    virtual ErrorType callPreSerializeCallback() const = 0;


    /**
    \brief Compares two NvParameterized objects
    \param [in] obj The other ::NvParameterized::Interface object (this == obj)
    \param [out] handlesOfInequality If the return value is False, these handles will contain the path to where definition or data is not identical
    \param [in] numHandlesOfInequality The number of handles that can be written to.
	\param [in] doCompareNotSerialized If false differences of parameters with DONOTSERIALIZE-hint are ignored.
    \returns true if parameter definition tree is equal as well as parameter values
    */
    virtual bool equals(const ::NvParameterized::Interface &obj, Handle* handlesOfInequality = NULL, uint32_t numHandlesOfInequality = 0, bool doCompareNotSerialized = true) const = 0;

    /**
    \brief Checks if object satisfies schema constraints
    \param [out] invalidHandles If the return value is False, these handles will contain the path to invalid data
    \param [in] numInvalidHandles The number of handles that can be written to.
    \returns true if all values satisfy constraints
    */
    virtual bool areParamsOK(Handle *invalidHandles = NULL, uint32_t numInvalidHandles = 0) = 0;

    /**
    \brief Copies an NvParameterized object
    \param [in] src the src NvParameterized object will be copied to this object.  It must be of the same type (class name).
    */
    virtual ErrorType copy(const ::NvParameterized::Interface &src) = 0;

    /**
    \brief Clones an NvParameterized object
    \param [out] nullDestObject cloned object; note this is a *reference* to a pointer; the destination cloned object will be created and stored in this pointer; should be NULL on entry!
    */
    virtual ErrorType clone(Interface *&nullDestObject) const = 0;

	/**
	\brief Check that alignments of internal elements match the schema
	\warning Only for internal use
	*/
	virtual bool checkAlignments() const = 0;

protected:
	/**
	\brief Initialize a Reference parameter

	\note By default previous value of parameter isn't destroyed (bool doDestroyOld = false)
	*/
    virtual ErrorType initParamRef(const Handle &handle, const char *chosenRefStr = 0, bool doDestroyOld = false) = 0;

    // These functions wrap the raw(Get|Set)XXXXX() methods.  They deal with
    // error handling and casting.

	/**
	\brief Get param
	*/
    virtual ErrorType getParamBool(const Handle &handle, bool &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamBool(const Handle &handle, bool val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamBoolArray(const Handle &handle, bool *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamBoolArray(const Handle &handle, const bool *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamString(const Handle &handle, const char *&val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamString(const Handle &handle, const char *val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamStringArray(const Handle &handle, char **array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamStringArray(const Handle &handle, const char **array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamEnum(const Handle &handle, const char *&val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamEnum(const Handle &handle, const char *val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamEnumArray(const Handle &handle, char **array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamEnumArray(const Handle &handle, const char **array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamRef(const Handle &handle, ::NvParameterized::Interface *&val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamRefArray(const Handle &handle, ::NvParameterized::Interface **array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\note By default previous value of parameter is not destroyed (bool doDestroyOld = false)
	*/
    virtual ErrorType setParamRef(const Handle &handle, ::NvParameterized::Interface * val, bool doDestroyOld = false) = 0;

	/**
	\note By default previous values of parameter are not destroyed (bool doDestroyOld = false)
	*/
    virtual ErrorType setParamRefArray(const Handle &handle, /*const*/ ::NvParameterized::Interface **array, int32_t n, int32_t offset = 0, bool doDestroyOld = false) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI8(const Handle &handle, int8_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI8(const Handle &handle, int8_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI8Array(const Handle &handle, int8_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI8Array(const Handle &handle, const int8_t *val, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI16(const Handle &handle, int16_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI16(const Handle &handle, int16_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI16Array(const Handle &handle, int16_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI16Array(const Handle &handle, const int16_t *val, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI32(const Handle &handle, int32_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI32(const Handle &handle, int32_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI32Array(const Handle &handle, int32_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI32Array(const Handle &handle, const int32_t *val, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI64(const Handle &handle, int64_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI64(const Handle &handle, int64_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamI64Array(const Handle &handle, int64_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamI64Array(const Handle &handle, const int64_t *val, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU8(const Handle &handle, uint8_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU8(const Handle &handle, uint8_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU8Array(const Handle &handle, uint8_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU8Array(const Handle &handle, const uint8_t *val, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU16(const Handle &handle, uint16_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU16(const Handle &handle, uint16_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU16Array(const Handle &handle, uint16_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU16Array(const Handle &handle, const uint16_t *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU32(const Handle &handle, uint32_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU32(const Handle &handle, uint32_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU32Array(const Handle &handle, uint32_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU32Array(const Handle &handle, const uint32_t *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU64(const Handle &handle, uint64_t &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU64(const Handle &handle, uint64_t val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamU64Array(const Handle &handle, uint64_t *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamU64Array(const Handle &handle, const uint64_t *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamF32(const Handle &handle, float &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamF32(const Handle &handle, float val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamF32Array(const Handle &handle, float *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamF32Array(const Handle &handle, const float *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamF64(const Handle &handle, double &val) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamF64(const Handle &handle, double val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamF64Array(const Handle &handle, double *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamF64Array(const Handle &handle, const double *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamVec2(const Handle &handle, const physx::PxVec2 &val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec2(const Handle &handle, physx::PxVec2 &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec2Array(const Handle &handle, physx::PxVec2 *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamVec2Array(const Handle &handle, const physx::PxVec2 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamVec3(const Handle &handle, const physx::PxVec3 &val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec3(const Handle &handle, physx::PxVec3 &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec3Array(const Handle &handle, physx::PxVec3 *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamVec3Array(const Handle &handle, const physx::PxVec3 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamVec4(const Handle &handle, const physx::PxVec4 &val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec4(const Handle &handle, physx::PxVec4 &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamVec4Array(const Handle &handle, physx::PxVec4 *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamVec4Array(const Handle &handle, const physx::PxVec4 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamQuat(const Handle &handle, const physx::PxQuat &val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamQuat(const Handle &handle, physx::PxQuat &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamQuatArray(const Handle &handle, physx::PxQuat *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamQuatArray(const Handle &handle, const physx::PxQuat *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamMat33(const Handle &handle, const physx::PxMat33 &val) = 0;

	/**
	\brief Get param
	*/
	virtual ErrorType getParamMat33(const Handle &handle, physx::PxMat33 &val) const = 0;

	/**
	\brief Get param
	*/
	virtual ErrorType getParamMat33Array(const Handle &handle, physx::PxMat33 *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamMat33Array(const Handle &handle, const physx::PxMat33 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamMat44(const Handle &handle, const physx::PxMat44 &val) = 0;
    
	/**
	\brief Get param
	*/
	virtual ErrorType getParamMat44(const Handle &handle, physx::PxMat44 &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamMat44Array(const Handle &handle, physx::PxMat44 *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamMat44Array(const Handle &handle, const physx::PxMat44 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
	virtual ErrorType setParamMat34Legacy(const Handle &handle, const float (&val)[12]) = 0;
    
	/**
	\brief Get param
	*/
	virtual ErrorType getParamMat34Legacy(const Handle &handle, float (&val)[12]) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamMat34LegacyArray(const Handle &handle, float *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamMat34LegacyArray(const Handle &handle, const float *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamBounds3(const Handle &handle, const physx::PxBounds3 &val) = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamBounds3(const Handle &handle, physx::PxBounds3 &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamBounds3Array(const Handle &handle, physx::PxBounds3 *array, int32_t n, int32_t offset = 0) const = 0;

		/**
	\brief Set param
	*/
	virtual ErrorType setParamTransform(const Handle &handle, const physx::PxTransform &val) = 0;
    
	/**
	\brief Get param
	*/
	virtual ErrorType getParamTransform(const Handle &handle, physx::PxTransform &val) const = 0;

	/**
	\brief Get param
	*/
    virtual ErrorType getParamTransformArray(const Handle &handle, physx::PxTransform *array, int32_t n, int32_t offset = 0) const = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamTransformArray(const Handle &handle, const physx::PxTransform *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Set param
	*/
    virtual ErrorType setParamBounds3Array(const Handle &handle, const physx::PxBounds3 *array, int32_t n, int32_t offset = 0) = 0;

	/**
	\brief Store value of parameter into a string
	\see Handle::valueToStr
	*/
    virtual ErrorType valueToStr(const Handle &handle, char *buf, uint32_t bufSize, const char *&ret) = 0;

	/**
	\brief Read value of parameter from string
	\see Handle::strToValue
	*/
    virtual ErrorType strToValue(Handle &handle,const char *str, const char **endptr) = 0; // assigns this string to the value

    /**
    \brief Resize array parameter
	\see Handle::resizeArray
    */
    virtual ErrorType resizeArray(const Handle &array_handle, int32_t new_size) = 0;

    /**
    \brief Get size of array parameter
	\see Handle::getArraySize
    */
	virtual ErrorType getArraySize(const Handle &array_handle, int32_t &size, int32_t dimension = 0) const = 0;

    /**
    \brief Swap two elements of an array
	\see Handle::swapArrayElements
    */
    virtual ErrorType swapArrayElements(const Handle &array_handle, uint32_t firstElement, uint32_t secondElement) = 0;
};

} // end of namespace

#include "NvParameterized.inl" // inline the NvParamterHandle methods.

PX_POP_PACK

#endif // NV_PARAMETERIZED_H
