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

#include "PxSimpleTypes.h"
#include "PxAssert.h"
#include "PsArray.h"
#include "PxVec3.h"
#include "PxQuat.h"
#include "PxBounds3.h"
#include "PsFastXml.h"
#include "PsIOStream.h"

#include "nvparameterized/NvSerializer.h"
#include "XmlSerializer.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "NvParameters.h"

#include "NvTraitsInternal.h"
#include "XmlDeserializer.h"

#define PRINT_ELEMENT_HINTS 0
#define PRINT_ELEMENTS_WITHIN_EMPTY_ARRAYS 0

#define UNOPTIMIZED_XML 0

namespace NvParameterized
{

static const char indentStr[] = "  ";

struct traversalState
{
	traversalState()
	{
		indent[0] = 0;
		indentLen = 0;
		level = 0;
	}

	void incLevel()
	{
		physx::shdfnd::strlcat(indent, (uint32_t)strlen(indent) + (uint32_t)strlen(indentStr) + 1, indentStr);
		level++;
	}

	void decLevel()
	{
		level--;
		indentLen = (sizeof(indentStr) - 1) * level;
		if(indentLen < sizeof(indent))
			indent[indentLen] = 0;
	}

	char indent[4096];
	uint32_t indentLen;
	int32_t level;
};

Serializer::ErrorType XmlSerializer::peekNumObjects(char *data, uint32_t len, uint32_t &numObjects)
{
	//FIXME: this code is not robust

	data[len-1] = 0;

	const char *root = ::strstr(data, "<NvParameters");
	if( !root )
	{
		root = ::strstr(data, "<NvParameters");
		if (!root)
		{
			return Serializer::ERROR_MISSING_ROOT_ELEMENT;
		}
	}

	const char *numObjectsString = ::strstr(root, "numObjects");
	if( !numObjectsString )
		return Serializer::ERROR_INVALID_ATTR;

	numObjectsString = ::strstr(numObjectsString, "\"");
	if( !numObjectsString )
		return Serializer::ERROR_INVALID_ATTR;

	numObjects = strtoul(numObjectsString + 1, 0, 0); //Skip leading quote
	return Serializer::ERROR_NONE;
}

class InputDataFromPxFileBuf: public physx::PxInputData
{
public:
	InputDataFromPxFileBuf(physx::PxFileBuf& fileBuf): mFileBuf(fileBuf) {}

	// physx::PxInputData interface
	virtual uint32_t	getLength() const
	{
		return mFileBuf.getFileLength();
	}

	virtual void	seek(uint32_t offset)
	{
		mFileBuf.seekRead(offset);
	}

	virtual uint32_t	tell() const
	{
		return mFileBuf.tellRead();
	}

	// physx::PxInputStream interface
	virtual uint32_t read(void* dest, uint32_t count)
	{
		return mFileBuf.read(dest, count);
	}

	PX_NOCOPY(InputDataFromPxFileBuf)
private:
	physx::PxFileBuf& mFileBuf;
};

Serializer::ErrorType XmlSerializer::peekClassNames(physx::PxFileBuf &stream, char **classNames, uint32_t &numClassNames)
{
	class ClassNameReader: public physx::shdfnd::FastXml::Callback
	{
		uint32_t mDepth;
		uint32_t mNumObjs;

		char **mClassNames;
		uint32_t mNumClassNames;

		Traits *mTraits;

	public:
		ClassNameReader(char **classNames, uint32_t &numClassNames, Traits *traits)
			: mDepth(0), mNumObjs(0), mClassNames(classNames), mNumClassNames(numClassNames), mTraits(traits) {}

		uint32_t numObjs() const { return mNumObjs; }

		bool processComment(const char * /*comment*/) { return true; }

		bool processClose(const char * /*element*/,uint32_t /*depth*/,bool & /*isError*/)
		{
			--mDepth;
			return true;
		}

		bool processElement(
			const char *elementName,
			const char  * /*elementData*/,
			const physx::shdfnd::FastXml::AttributePairs& attr,
			int32_t /*lineno*/)
		{
			//Ignore top-level element
			if( 0 == strcmp(elementName, "NvParameters") ||  0 == strcmp(elementName, "NxParameters"))
				return true;

			++mDepth;

			if( 1 != mDepth || 0 != strcmp(elementName, "value") )
				return true;

			//Top-level <value> => read className
			mClassNames[mNumObjs] = mTraits->strdup( attr.get("className") );
			++mNumObjs;

			return mNumObjs < mNumClassNames;
		}

		void *allocate(uint32_t size) { return ::malloc(size); }
		void deallocate(void *ptr) { ::free(ptr); };
	};

	ClassNameReader myReader(classNames, numClassNames, mTraits);
	physx::shdfnd::FastXml *xmlParser = physx::shdfnd::createFastXml(&myReader);

	InputDataFromPxFileBuf inputData(stream);
	xmlParser->processXml(inputData);
	numClassNames = myReader.numObjs();

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType XmlSerializer::peekNumObjectsInplace(const void * data, uint32_t dataLen, uint32_t & numObjects)
{
	if ( !dataLen || ! data )
		return ERROR_STREAM_ERROR;

	char hdr[100];
	uint32_t len = physx::PxMin<uint32_t>(dataLen, sizeof(hdr) - 1);
	physx::shdfnd::strlcpy(hdr, len+1, (const char *)data);

	return peekNumObjects(hdr, len, numObjects);
}

Serializer::ErrorType XmlSerializer::peekNumObjects(physx::PxFileBuf &stream, uint32_t &numObjects)
{
	//FIXME: this code is not robust

	char hdr[100];
	uint32_t len = stream.peek(hdr, sizeof(hdr));

	return peekNumObjects(hdr, len, numObjects);
}

#ifndef WITHOUT_APEX_SERIALIZATION

static void storeVersionAndChecksum(physx::PsIOStream &stream, const Interface *obj)
{
	uint16_t major = obj->getMajorVersion(),
		minor = obj->getMinorVersion(); 

	stream << " version=\"" << major << '.' << minor << '"';

	uint32_t bits;
	const uint32_t *checksum = obj->checksum(bits);

	uint32_t u32s = bits / 32;
	PX_ASSERT( 0 == bits % 32 );

	stream << " checksum=\"";
	for(uint32_t i = 0; i < u32s; ++i)
	{
		char hex[20];
		physx::shdfnd::snprintf(hex, sizeof(hex), "0x%x", checksum[i]);
		stream << hex;
		if( u32s - 1 != i )
			stream << ' ';
	}
	stream << '"';
}

static bool IsSimpleType(const Definition *d)
{
	//We do not consider strings simple because it causes errors with NULL and ""
	if (d->type() == TYPE_ARRAY  || d->type() == TYPE_STRUCT ||
		d->type() == TYPE_REF || d->type() == TYPE_STRING ||
		d->type() == TYPE_ENUM)
	{
		return false;
	}
	else
	{
		PX_ASSERT( d->numChildren() == 0 );
		return true;
	}
}

static bool IsSimpleStruct(const Definition *pd)
{
	bool ret = true;

	int32_t count = pd->numChildren();
	for (int32_t i=0; i < count; i++)
	{
		const Definition *d = pd->child(i);
		if ( !IsSimpleType(d) )
		{
			ret = false;
			break;
		}
	}

	return ret;
}

static bool DoesNeedQuote(const char *c)
{
	bool ret = false;
	while ( *c )
	{
		if ( *c == 32 || *c == ',' || *c == '<' || *c == '>' || *c == 9 )
		{
			ret = true;
			break;
		}
		c++;
	}

	return ret;
}
	
Serializer::ErrorType XmlSerializer::traverseParamDefTree(
	const Interface &obj,
	physx::PsIOStream &stream,
	traversalState &state,
	Handle &handle,
	bool printValues)
{
	bool isRoot = !handle.numIndexes() && 0 == state.level;

	if( !handle.numIndexes() )
	{
		NV_PARAM_ERR_CHECK_RETURN( obj.getParameterHandle("", handle), Serializer::ERROR_UNKNOWN );

		if( isRoot )
		{
			NV_ERR_CHECK_RETURN( emitElement(obj, stream, "value", handle, false, true, true) );
			stream << "\n";

			state.incLevel();
		}
	}

	const Definition *paramDef = handle.parameterDefinition();

	if( !paramDef->hint("DONOTSERIALIZE") )
	{

#	if PRINT_ELEMENT_HINTS
	bool includedRef = false;

	NV_ERR_CHECK_RETURN( emitElementNxHints(stream, handle, state, includedRef) );
#	else
	bool includedRef = paramDef->isIncludedRef();
#	endif

	switch( paramDef->type() )
	{
	case TYPE_STRUCT:
		{
			stream << state.indent;
			NV_ERR_CHECK_RETURN( emitElement(obj, stream, "struct", handle, false, true) );
			stream << "\n";

			state.incLevel();
			for(int32_t i = 0; i < paramDef->numChildren(); ++i)
			{
				handle.set(i);
				NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, printValues) );
				handle.popIndex();
			}

			state.decLevel();

			stream << state.indent << "</struct>\n";

			break;
		}

	case TYPE_ARRAY:
		{
			stream << state.indent;
			NV_ERR_CHECK_RETURN( emitElement(obj, stream, "array", handle, false, true) );

			int32_t arraySize;
			NV_PARAM_ERR_CHECK_RETURN( handle.getArraySize(arraySize), Serializer::ERROR_INVALID_ARRAY );

			if( arraySize)
				stream << "\n";

			state.incLevel();

			if ( arraySize > 0 )
			{
#if UNOPTIMIZED_XML
				for(int32_t i = 0; i < arraySize; ++i)
				{
					handle.set(i);
					NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, printValues) );
					handle.popIndex();
				}
#else
				handle.set(0);
				const Definition *pd = handle.parameterDefinition();
				handle.popIndex();
				switch ( pd->type() )
				{
					case TYPE_STRUCT:
						{
							if ( IsSimpleStruct(pd) )
							{
								for(int32_t i = 0; i < arraySize; ++i)
								{

									if ( (i&3) == 0 )
									{
										if ( i )
											stream << "\n";
										stream << state.indent;
									}

									handle.set(i);

									for( int32_t j=0; j<pd->numChildren(); j++ )
									{
										if (pd->child(j)->hint("DONOTSERIALIZE"))
											continue;

										handle.set(j);

										char buf[512];
										const char *str = 0;
										NV_PARAM_ERR_CHECK_RETURN( handle.valueToStr(buf, sizeof(buf), str), Serializer::ERROR_VAL2STRING_FAILED );

										stream << str;

										if ( (j+1) < pd->numChildren() )
										{
											stream << " ";
										}

										handle.popIndex();
									}

									if ( (i+1) < arraySize )
									{
										stream << ",";
									}

									handle.popIndex();
								} //i
								stream << "\n";
							}
							else
							{
								for(int32_t i = 0; i < arraySize; ++i)
								{
									handle.set(i);
									NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, printValues) );
									handle.popIndex();
								}
							}
						}
						break;

					case TYPE_REF:
						for(int32_t i = 0; i < arraySize; ++i)
						{
							handle.set(i);
							NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, printValues) );
							handle.popIndex();
						}
						break;

					case TYPE_BOOL:
						{
							bool v = false;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamBoolArray(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << (uint32_t)v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&63) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

    				case TYPE_I8:
						{
							int8_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamI8Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&63) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_I16:
						{
							int16_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamI16Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_I32:
						{
							int32_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamI32Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_I64:
						{
							int64_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamI64Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_U8:
						{
							uint8_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamU8Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&63) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_U16:
						{
							uint16_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamU16Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&63) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_U32:
						{
							uint32_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamU32Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_U64:
						{
							uint64_t v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamU64Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_F32:
						{
							float v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamF32Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

    				case TYPE_F64:
						{
							double v = 0;
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamF64Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v;
								if ( (i+1) < arraySize )
								{
									stream << " ";
								}
								if ( ((i+1)&31) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
    					break;

					case TYPE_VEC2:
						{
							physx::PxVec2 v(0,0);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamVec2Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v.x << " " << v.y;
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_VEC3:
						{
							physx::PxVec3 v(0,0,0);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamVec3Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v.x << " " << v.y << " " << v.z;
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_VEC4:
						{
							physx::PxVec4 v(0,0,0,0);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamVec4Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v.x << " " << v.y << " " << v.z << " " << v.w;
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_QUAT:
						{
							physx::PxQuat v(0,0,0,1);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamQuatArray(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v.x << " " << v.y << " " << v.z << " " << v.w;
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_MAT33:
						{
							physx::PxMat33 m = physx::PxMat33(physx::PxIdentity);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamMat33Array(&m,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								const float *f = (const float *)m.front();
								stream << f[0] << " " << f[1] << " " << f[2] << " " ;
								stream << f[3] << " " << f[4] << " " << f[5] << " ";
								stream << f[6] << " " << f[7] << " " << f[8];
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_BOUNDS3:
						{
							physx::PxBounds3 v;
							v.minimum = physx::PxVec3(0.0f);
							v.maximum = physx::PxVec3(0.0f);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamBounds3Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								stream << v.minimum.x << " " << v.minimum.y << " " << v.minimum.z << " " << v.maximum.x << " " << v.maximum.y << " " << v.maximum.z;
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_MAT44:
						{
							physx::PxMat44 v = physx::PxMat44(physx::PxIdentity);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamMat44Array(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								const float *f = (const float *)v.front();
								stream << f[0] << " " << f[1] << " " << f[2] << " " << f[3] << " ";
								stream << f[4] << " " << f[5] << " " << f[6] << " " << f[7] << " ";
								stream << f[8] << " " << f[9] << " " << f[10] << " " << f[11] << " ";
								stream << f[12] << " " << f[13] << " " << f[14] << " " << f[15];
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					case TYPE_TRANSFORM:
						{
							physx::PxTransform v = physx::PxTransform(physx::PxIdentity);
							stream << state.indent;
							for (int32_t i=0; i<arraySize; i++)
							{
								NV_PARAM_ERR_CHECK_RETURN( handle.getParamTransformArray(&v,1,i), Serializer::ERROR_ARRAY_INDEX_OUT_OF_RANGE );
								const float *f = (const float *)&v;
								stream << f[0] << " " << f[1] << " " << f[2] << " " << f[3] << " ";
								stream << f[4] << " " << f[5] << " " << f[6] << " ";
								if ( (i+1) < arraySize )
								{
									stream << ", ";
								}
								if ( ((i+1)&15) == 0 )
								{
									stream << "\n";
									stream << state.indent;
								}
							}
							stream << "\n";
						}
						break;

					NV_PARAMETRIZED_STRING_DATATYPE_LABELS
					NV_PARAMETRIZED_ENUM_DATATYPE_LABELS
					NV_PARAMETRIZED_SERVICE_DATATYPE_LABELS 
					NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
					NV_PARAMETRIZED_LEGACY_DATATYPE_LABELS
					case NvParameterized::TYPE_ARRAY:
					default:
						for(int32_t i = 0; i < arraySize; ++i)
						{
							handle.set(i);
							NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, printValues) );
							handle.popIndex();
						}
						break;
				}
#endif
			}

#	if PRINT_ELEMENTS_WITHIN_EMPTY_ARRAYS
			if( arraySize == 0 )
			{
				handle.set(0);
				NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle, false) );
				handle.popIndex();
			}
#	endif

			state.decLevel();
			if( arraySize)
				stream << state.indent;

			stream << "</array>\n";

			break;
		}

	case TYPE_REF:
		{
			stream << state.indent;
			NV_ERR_CHECK_RETURN( emitElement(obj, stream, "value", handle, includedRef, printValues) );

			if( printValues && includedRef )
			{
				stream << state.indent << "\n";

				Interface *refObj = 0;
				NV_PARAM_ERR_CHECK_RETURN( handle.getParamRef(refObj), Serializer::ERROR_UNKNOWN );

				if( refObj )
				{
					Handle refHandle(refObj);
					state.incLevel();
					NV_ERR_CHECK_RETURN( traverseParamDefTree(*refObj, stream, state, refHandle) );
					state.decLevel();
					stream << state.indent;
				}
			}

			stream << "</value>\n";

			break;
		}

	case TYPE_POINTER:
		//Don't do anything with pointer
		break;

NV_PARAMETRIZED_LINAL_DATATYPE_LABELS
NV_PARAMETRIZED_ARITHMETIC_DATATYPE_LABELS
NV_PARAMETRIZED_STRING_DATATYPE_LABELS
NV_PARAMETRIZED_ENUM_DATATYPE_LABELS
NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
	default:
		{
			stream << state.indent;
			NV_ERR_CHECK_RETURN( emitElement(obj, stream, "value", handle, includedRef, printValues) );

			char buf[512];
			const char *str = 0;
			if(	printValues )
				NV_PARAM_ERR_CHECK_RETURN( handle.valueToStr(buf, sizeof(buf), str), Serializer::ERROR_VAL2STRING_FAILED );

			if( str )
				stream << str;

			stream << "</value>\n";

			break;
		} //default
	} //switch

	} //DONOTSERIALIZE

	if( isRoot )
	{
		state.decLevel();
		stream << "</value>\n";
	}

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType XmlSerializer::emitElementNxHints(
	physx::PsIOStream &stream,
	Handle &handle,
	traversalState &state,
	bool &includedRef)
{
	const Definition *paramDef = handle.parameterDefinition();

	for(int32_t j = 0; j < paramDef->numHints(); ++j)
	{
		if( 0 == j )
			stream << "\n";

		const Hint *hint = paramDef->hint(j);

		stream << state.indent << "<!-- " << hint->name() << ": ";

		if( hint->type() == TYPE_STRING )
			stream << hint->asString() ;
		else if( hint->type() == TYPE_U64 )
			stream << hint->asUInt() ;
		else if( hint->type() == TYPE_F64 )
			stream << hint->asFloat() ;

		stream << " -->\n";
	}

	includedRef = paramDef->isIncludedRef();

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType XmlSerializer::emitElement(
	const Interface &obj,
	physx::PsIOStream &stream,
	const char *elementName,
	Handle &handle,
	bool includedRef,
	bool printValues,
	bool isRoot)
{
	const Definition *paramDef = handle.parameterDefinition();

	DataType parentType = TYPE_UNDEFINED;

	if( paramDef->parent() )
		parentType = paramDef->parent()->type();

	stream << '<' << elementName;

	if( isRoot )
	{
		stream << " name=\"\""
			<< " type=\"Ref\""
			<< " className=\"" << obj.className() << "\"";

		const char *objectName = obj.name();
		if( objectName )
			stream << " objectName=\"" << objectName << "\"";

		if( isRoot ) //We only emit version info for root <struct>
			storeVersionAndChecksum(stream, &obj);
	}
	else
	{
		if( parentType != TYPE_ARRAY )
		{
			const char *name = paramDef->name();
			stream << " name=\"" << (name ? name : "") << "\"";
		}
	}

	switch( paramDef->type() )
	{
	case TYPE_STRUCT:
		break;

	case TYPE_ARRAY:
		{
			int32_t arraySize;
			NV_PARAM_ERR_CHECK_RETURN( handle.getArraySize(arraySize), Serializer::ERROR_INVALID_ARRAY );
			stream << " size=\"" << arraySize << '"';
			handle.set(0);
			const Definition *pd = handle.parameterDefinition();
			handle.popIndex();
			stream << " type=\"" << typeToStr(pd->type()) << '"';
			// ** handle use case for simple structs written out flat..
#if !UNOPTIMIZED_XML
			if ( pd->type() == TYPE_STRUCT && IsSimpleStruct(pd) )
			{
				stream << " structElements=\"";
				const int32_t count = pd->numChildren();

				// find how many of them need serialization
				int32_t serializeCount = 0;
				for (int32_t i=0; i<count; i++)
				{
					const Definition *d = pd->child(i);
					if (d->hint("DONOTSERIALIZE") == NULL)
					{
						serializeCount++;
					}
				}

				for (int32_t i=0; i<count; i++)
				{
					const Definition *d = pd->child(i);
					if (d->hint("DONOTSERIALIZE"))
						continue;

					stream << d->name();
					stream << "(";
					stream << typeToStr(d->type());
					stream << ")";
					if ( (i+1) < serializeCount )
					{
						stream<<",";
					}
				}
				stream << "\"";
			}
#endif
			//
			break;
		}

	case TYPE_REF:
		{
			stream << " type=\"" << typeToStr(paramDef->type()) << '"';

			Interface *paramPtr = 0;
			if( printValues )
				NV_PARAM_ERR_CHECK_RETURN( handle.getParamRef(paramPtr), Serializer::ERROR_UNKNOWN );

			stream << " included=\"" << ( includedRef ? "1" : "0" ) << "\"";

			if( !printValues || !paramPtr )
			{
				stream << " classNames=\"";
				for(int32_t i = 0; i < paramDef->numRefVariants(); ++i)
				{
					const char *ref = paramDef->refVariantVal(i);
					if ( DoesNeedQuote(ref) )
						stream << "%20" << ref << "%20" << " ";
					else
						stream << ref << " ";
				}
				stream << '"';

				break;
			}

			stream << " className=\"" << paramPtr->className() << '"';

			const char *objectName = paramPtr->name();
			if( objectName )
				stream << " objectName=\"" << objectName << "\"";

			if( includedRef )
				storeVersionAndChecksum(stream, paramPtr);

			break;
		}

	case TYPE_STRING:
	case TYPE_ENUM:
		{
			const char *val;
			NV_PARAM_ERR_CHECK_RETURN( handle.getParamString(val), Serializer::ERROR_UNKNOWN );

			//Make a note if value is NULL
			if( !val )
				stream << " null=\"1\"";
		}

		//Fall-through to default

NV_PARAMETRIZED_LINAL_DATATYPE_LABELS
NV_PARAMETRIZED_ARITHMETIC_DATATYPE_LABELS
NV_PARAMETRIZED_SERVICE_DATATYPE_LABELS
NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
	default:
		stream << " type=\"" << typeToStr(paramDef->type()) << "\"";
		break;
	} //switch

	stream << '>';

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType XmlSerializer::internalSerialize(physx::PxFileBuf &fbuf, const Interface **objs, uint32_t n, bool doMetadata)
{
	PX_UNUSED(doMetadata);

	physx::PsIOStream stream(fbuf, fbuf.getFileLength());
	stream.setBinary(false);

	uint32_t minor = version() & 0xffffUL,
		major = version() >> 16;

	stream << "<!DOCTYPE NvParameters>\n"
		<< "<NvParameters "
		<< "numObjects=\"" << n << "\" "
		<< "version=\"" << major << '.' << minor << "\" "
		<< ">\n";

	for(uint32_t i = 0; i < n; ++i)
	{
		const Interface &obj = *objs[i];
		Handle handle(obj);

		traversalState state;
		NV_ERR_CHECK_RETURN( traverseParamDefTree(obj, stream, state, handle) );
	}

	stream << "</NvParameters>\n";

	return Serializer::ERROR_NONE;
}

#endif

Serializer::ErrorType XmlSerializer::internalDeserialize(
	physx::PxFileBuf &stream,
	Serializer::DeserializedData &res,
	bool & /*doesNeedUpdate*/)
{
	XmlDeserializer *d = XmlDeserializer::Create(mTraits, XmlSerializer::version());
	physx::shdfnd::FastXml *xmlParser = physx::shdfnd::createFastXml(d);
	InputDataFromPxFileBuf inputData(stream);
	if( xmlParser && !xmlParser->processXml(inputData) )
	{
		Serializer::ErrorType err = d->getLastError();
		if( Serializer::ERROR_NONE == err ) //Proper error code not set?
		{
			DEBUG_ALWAYS_ASSERT(); //XmlDeserializer should set explicit error codes
			err = Serializer::ERROR_UNKNOWN;
		}

		xmlParser->release();

		d->releaseAll();
		d->destroy();

		return err;
	}

	if ( xmlParser )
		xmlParser->release();

	res.init(mTraits, d->getObjs(), d->getNobjs());

	d->destroy();
	
	return Serializer::ERROR_NONE;
}
bool isXmlFormat(physx::PxFileBuf &stream)
{
	// if it is at least 32 bytes long and the first 32 byte are all ASCII, then consider it potentially valid XML

	if( stream.getFileLength() < 32 )
		return false;

	char hdr[32];
	stream.peek(hdr, sizeof(hdr));

	for(size_t i = 0; i < sizeof(hdr); ++i)
	{
		char c = hdr[i];
		if( !(c == '\r' || c == '\t' || c == '\n' || ( c >= 32 && c < 127)) )
			return false;
	}

	const char *magic1 = "<!DOCTYPE NvParameters>";
	const char *magic2 = "<!DOCTYPE NxParameters>";
	return 0 == ::strncmp(hdr, magic1, strlen(magic1)) || 
			0 == ::strncmp(hdr, magic2, strlen(magic2));
}

} // namespace NvParameterized

