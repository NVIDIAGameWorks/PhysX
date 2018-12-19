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

#ifndef NV_SERIALIZER_H
#define NV_SERIALIZER_H

/*!
\file
\brief NvParameterized serializer class
*/


#include <filebuf/PxFileBuf.h>


namespace NvParameterized
{

PX_PUSH_PACK_DEFAULT

/**
\brief Platform descriptor

This class describes target serialization platform which consists of processor architecture, compiler and OS.
*/
struct SerializePlatform
{
	/** 
	\brief Processor architectures enumeration
	\warning Do not change values of enums!
	*/
	typedef enum
	{
		ARCH_GEN = 0,
		ARCH_X86 = 1,
		ARCH_X86_64 = 2,
		ARCH_PPC = 3,
		ARCH_CELL = 4,
		ARCH_ARM = 5,
		ARCH_ARM_64 = 6,
		ARCH_LAST
	} ArchType;

	/**
	\brief Platform processor architecture
	*/
	ArchType archType;

	/** 
	\brief Compilers enumeration
	\warning Do not change values of enums!
	*/
	typedef enum
	{
		COMP_GEN = 0,
		COMP_GCC = 1,
		COMP_VC = 2,
		COMP_MW = 3,
		COMP_LAST
	} CompilerType;

	/**
	\brief Platform compiler
	*/
	CompilerType compilerType;

	/**
	\brief Platform compiler version
	*/
	uint32_t compilerVer;

	/** 
	\brief OSes enumeration
	\warning Do not change values of enums!
	*/
	typedef enum
	{
		OS_WINDOWS = 0,
		OS_LINUX = 1,
		OS_LV2 = 2, // PS3
		OS_MACOSX = 3,
		OS_XBOX = 4,
		OS_GEN = 5,
		OS_ANDROID = 6,
		OS_XBOXONE = 7,
		OS_PS4 = 8,
		OS_HOS = 9,
		OS_LAST
	} OsType;

	/**
	\brief Platform OS
	*/
	OsType osType;

	/**
	\brief Platform OS version
	*/
	uint32_t osVer;

	/**
	\brief This value identfies that version is unknown
	*/
	static const uint32_t ANY_VERSION = (uint32_t)-1;

	PX_INLINE SerializePlatform();
	
	/**
	\brief Constructor of SerializePlatform
	*/
	PX_INLINE SerializePlatform(ArchType archType, CompilerType compType, uint32_t compVer, OsType osType, uint32_t osVer);

	/**
	\brief Checks if platforms are binary-compatible
	*/
	PX_INLINE bool operator ==(const SerializePlatform &p) const;

	/**
	\brief Checks if platforms are binary-incompatible
	*/
	PX_INLINE bool operator !=(const SerializePlatform &p) const;
};

class Interface;
class Definition;
class Traits;
struct SerializePlatform;

/**
\brief Interface class for serializer-deserializer of NvParameterized objects

Serializer serializes and deserializes one or more NvParameterized objects to file using various output formats
(see SerializeType).
*/
class Serializer
{
public:

	/**
	\brief Status enums that the Serializer methods may return
	*/
	enum ErrorType
	{
		ERROR_NONE = 0,

		ERROR_UNKNOWN,
		ERROR_NOT_IMPLEMENTED,

		// File format related errors
		ERROR_INVALID_PLATFORM,
		ERROR_INVALID_PLATFORM_NAME,
		ERROR_INVALID_FILE_VERSION,
		ERROR_INVALID_FILE_FORMAT,
		ERROR_INVALID_MAGIC,
		ERROR_INVALID_CHAR,

		// External errors
		ERROR_STREAM_ERROR,
		ERROR_MEMORY_ALLOCATION_FAILURE,
		ERROR_UNALIGNED_MEMORY,
		ERROR_PRESERIALIZE_FAILED,
		ERROR_INTERNAL_BUFFER_OVERFLOW,
		ERROR_OBJECT_CREATION_FAILED,
		ERROR_CONVERSION_FAILED,

		// Xml-specific errors
		ERROR_VAL2STRING_FAILED,
		ERROR_STRING2VAL_FAILED,
		ERROR_INVALID_TYPE_ATTRIBUTE,
		ERROR_UNKNOWN_XML_TAG,
		ERROR_MISSING_DOCTYPE,
		ERROR_MISSING_ROOT_ELEMENT,
		ERROR_INVALID_NESTING,
		ERROR_INVALID_ATTR,

		// Other stuff
		ERROR_INVALID_ARRAY,
		ERROR_ARRAY_INDEX_OUT_OF_RANGE,
		ERROR_INVALID_VALUE,
		ERROR_INVALID_INTERNAL_PTR,
		ERROR_INVALID_PARAM_HANDLE,
		ERROR_INVALID_RELOC_TYPE,
		ERROR_INVALID_DATA_TYPE,
		ERROR_INVALID_REFERENCE
	};

	/**
	\brief The supported serialization formats
	*/
	enum SerializeType
	{
		/// serialize in XML format.
		NST_XML = 0,

		/// serialize in a binary format
		NST_BINARY,

		NST_LAST
	};

	/**
	\brief Get type of stream (binary or xml)
	\param [in] stream stream to be analyzed
	*/
	static SerializeType peekSerializeType(physx::general_PxIOStream2::PxFileBuf &stream);

	/**
	\brief Get stream native platform
	\param [in] stream stream to be analyzed
	\param [out] platform stream native platform
	*/
	static ErrorType peekPlatform(physx::general_PxIOStream2::PxFileBuf &stream, SerializePlatform &platform);

	virtual ~Serializer() {}

	/**
	\brief Set platform to use in platform-dependent serialization
	\param [in] platform target platform

	\warning Currently this is used only in binary serializer

	Application running on target platforms may potentially make use of extremely fast
	inplace deserialization (using method deserializeInplace) on files which were serialized
	for this platform.
	*/
	virtual ErrorType setTargetPlatform(const SerializePlatform &platform) = 0;

	/**
	\brief Sets whether serializer will automatically update
		objects after deserialization
	\param [in] doUpdate should automatic update be done?

	\warning Normally you will not need this
	\warning This is true by default
	*/
	virtual void setAutoUpdate(bool doUpdate) = 0;

	/**
	\brief Serialize array of NvParameterized-objects to a stream
	\param [in] stream the stream to which the object will be serialized
	\param [in] objs NvParameterized-objects which will be serialized
	\param [in] nobjs number of objects
	\param [in] doSerializeMetadata set this to store object metadata in file

	\warning Serialized file may depend on selected target platform
	*/
	virtual ErrorType serialize(
		physx::general_PxIOStream2::PxFileBuf &stream,
		const ::NvParameterized::Interface **objs,
		uint32_t nobjs,
		bool doSerializeMetadata = false) = 0;

	/**
	\brief Peek number of NvParameterized-objects in stream with serialized data
	\param [in] stream the stream from which the object will be deserialized
	\param [out] numObjects number of objects

	\warning Not all streams support peeking
	*/
	virtual ErrorType peekNumObjects(physx::general_PxIOStream2::PxFileBuf &stream, uint32_t &numObjects) = 0;

	/**
	\brief Peek number of NvParameterized-objects in stream with serialized data
	\param [in] stream the stream from which objects will be deserialized
	\param [in] classNames pointer to buffer for resulting names
	\param [in,out] numClassNames limit on number of returned classNames; number of returned names

	\warning User is responsible for releasing every element of classNames via Traits::strfree()
	*/
	virtual ErrorType peekClassNames(physx::general_PxIOStream2::PxFileBuf &stream, char **classNames, uint32_t &numClassNames) = 0;

	/**
	\brief Peek number of NvParameterized-objects in memory buffer with serialized data
	\param [in] data pointer to memory buffer
	\param [in] dataLen length of memory buffer
	\param [out] numObjects number of objects
	*/
	virtual ErrorType peekNumObjectsInplace(const void *data, uint32_t dataLen, uint32_t &numObjects) = 0;

	/// TODO
	template < typename T, int bufSize = 8 > class DeserializedResults
	{
		T buf[bufSize]; //For small number of objects

		T *objs;

		uint32_t nobjs;

		Traits *traits;

		void clear();

	public:

		PX_INLINE DeserializedResults();

		PX_INLINE ~DeserializedResults();

		/**
		\brief Copy constructor
		*/
		PX_INLINE DeserializedResults(const DeserializedResults &data);

		/**
		\brief Assignment operator
		*/
		PX_INLINE DeserializedResults &operator =(const DeserializedResults &rhs);

		/**
		\brief Allocate memory for values
		*/
		PX_INLINE void init(Traits *traits_, uint32_t nobjs_);

		/**
		\brief Allocate memory and set values
		*/
		PX_INLINE void init(Traits *traits_, T *objs_, uint32_t nobjs_);

		/**
		\brief Number of objects in a container
		*/
		PX_INLINE uint32_t size() const;

		/**
		\brief Access individual object in container
		*/
		PX_INLINE T &operator[](uint32_t i);

		/**
		\brief Const-access individual object in container
		*/
		PX_INLINE const T &operator[](uint32_t i) const;

		/**
		\brief Read all NvParameterized objects in container to buffer outObjs
		\warning outObjs must be large enough to hold all contained objects
		*/
		PX_INLINE void getObjects(T *outObjs);

		/**
		\brief Release all objects
		*/
		PX_INLINE void releaseAll();
	};

	/**
	\brief Container for results of deserialization

	DeserializedData holds array of NvParameterized objects obtained during deserialization.
	*/
	typedef DeserializedResults< ::NvParameterized::Interface *> DeserializedData;

	/// This class keeps metadata of a single NvParameterized class
	struct MetadataEntry
	{
		/// Class name
		const char *className;

		/// Class version
		uint32_t version;

		/// Class metadata
		Definition *def;
	};

	/**
	\brief Container for results of metadata deserialization

	DeserializedMetadata holds array of MetadataEntry obtained during metadata deserialization.
	*/
	typedef DeserializedResults<MetadataEntry> DeserializedMetadata;

	/**
	\brief Deserialize metadata from a stream into one or more definitions
	\param [in] stream the stream from which metadata will be deserialized
	\param [out] desData storage for deserialized metadata
	\warning This is a draft implementation!
	*/
	virtual ErrorType deserializeMetadata(physx::general_PxIOStream2::PxFileBuf &stream, DeserializedMetadata &desData);

	/**
	\brief Deserialize a stream into one or more NvParameterized objects
	\param [in] stream the stream from which objects will be deserialized
	\param [out] desData storage for deserialized data
	*/
	virtual ErrorType deserialize(physx::general_PxIOStream2::PxFileBuf &stream, DeserializedData &desData);

	/**
	\brief Deserialize a stream into one or more NvParameterized objects
	\param [in] stream the stream from which objects will be deserialized
	\param [out] desData storage for deserialized data
	\param [out] isUpdated true if any legacy object was updated, false otherwise
	*/
	virtual ErrorType deserialize(physx::general_PxIOStream2::PxFileBuf &stream, DeserializedData &desData, bool &isUpdated) = 0;

	/**
	\brief Deserialize memory buffer into one or more NvParameterized objects
	\param [in] data pointer to serialized data. It should be allocated via Traits.
	\param [in] dataLen length of serialized data
	\param [out] desData storage for deserialized data

	\warning Currently only binary serializer supports inplace deserialization
	\warning Memory must be aligned to 8 byte boundary
	*/
	virtual ErrorType deserializeInplace(void *data, uint32_t dataLen, DeserializedData &desData);

	/**
	\brief Deserialize memory buffer into one or more NvParameterized objects
	\param [in] data pointer to serialized data
	\param [in] dataLen length of serialized data
	\param [out] desData storage for deserialized data
	\param [out] isUpdated true if any legacy object was updated, false otherwise

	\warning Currently only binary serializer supports inplace deserialization
	\warning Memory must be aligned to the boundary required by the data (see getInplaceAlignment)
	*/
	virtual ErrorType deserializeInplace(void *data, uint32_t dataLen, DeserializedData &desData, bool &isUpdated) = 0;

	/**
	\brief Get minimum alignment required for inplace deserialization of data in stream
	\param [in] stream stream which will be inplace deserialized
	\param [out] align alignment required for inplace deserialization of stream
	\note For most of the objects this will return default alignment of 8 bytes
	*/
	virtual ErrorType peekInplaceAlignment(physx::general_PxIOStream2::PxFileBuf& stream, uint32_t& align) = 0;

	/**
	\brief Release deserializer and any memory allocations associated with it
	*/
	virtual void release() = 0;
};

PX_POP_PACK

} // namespace NvParameterized

#include "NvSerializer.inl"

#endif // NV_SERIALIZER_H
