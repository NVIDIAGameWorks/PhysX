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
#include "NvParameters.h"
#include "nvparameterized/NvParameterizedTraits.h"

#include "NvTraitsInternal.h"

#include "XmlDeserializer.h"

#define XML_WARNING(_format, ...) \
	NV_PARAM_TRAITS_WARNING(mTraits, "XML serializer: " _format, ##__VA_ARGS__)



namespace NvParameterized
{

/*!
Get number of elements in array
*/
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define PX_ARRAY_SIZE(_array) (sizeof(ArraySizeHelper(_array)))

	static uint32_t ReadVersion(const physx::shdfnd::FastXml::AttributePairs& attr)
	{
		const char *versionText = attr.get("version");

		// If there's no version, assume version is 0.0
		if( !versionText )
			return 0;

		//XML stores versions in "x.y"-format
		//FIXME: strtoul is unsafe

		const char *dot = strchr(versionText, '.');
		uint32_t minor = dot ? strtoul(dot + 1, 0, 10) : 0;

		uint32_t major = strtoul(versionText, 0, 10);

		return (major << 16) + minor;
	}

	static PX_INLINE bool isWhiteSpace(char c)
	{
		return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',';
	}

	static PX_INLINE const char * skipWhiteSpace(const char *scan)
	{
		while ( isWhiteSpace(*scan) && *scan ) scan++;
		return *scan ? scan : 0;
	}

	static PX_FORCE_INLINE const char * skipNonWhiteSpace(const char* scan)
	{
		while ( !isWhiteSpace(*scan) && *scan ) scan++;
		return *scan ? scan : 0;
	}

	bool XmlDeserializer::verifyObject(Interface *obj, const physx::shdfnd::FastXml::AttributePairs& attr)
	{
		if( ReadVersion(attr) != obj->version() )
		{
			XML_WARNING("unknown error");
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_UNKNOWN;
			return false;
		}

		char *checksum = (char *)attr.get("checksum");
		if( checksum && !DoIgnoreChecksum(*obj) )
		{
			uint32_t objBits;
			const uint32_t *objChecksum = obj->checksum(objBits);

			uint32_t bits = 0;

			char *cur = checksum, *next = 0;
			bool sameBits = true;
			for(uint32_t i = 0; ; ++i, cur = next)
			{
				uint32_t val = (uint32_t)strtoul(cur, &next, 0); //FIXME: strtoul is not safe
				if( cur == next )
					break;

				bits += 32;
				if( bits > objBits || val != objChecksum[i] )
				{
					NV_PARAM_TRAITS_WARNING(
						mTraits,
						"Schema checksum is different for object of class %s and version %u, "
							"asset may be corrupted",
						obj->className(),
						(unsigned)obj->version()
						);
					sameBits = false;
					break;
				}
			}

			if( objBits != bits && sameBits )
			{
				NV_PARAM_TRAITS_WARNING(
					mTraits,
					"Schema checksum is different for object of class %s and version %u, "
						"asset may be corrupted",
					obj->className(),
					(unsigned)obj->version()
					);
			}
		} //if( checksum )

		return true;
	}

	bool XmlDeserializer::initAddressString(char *dest, uint32_t len, const char *name)
	{
		char *end = dest + len;

		for (uint32_t i = 0; i < tos().getIndex(); i++)
		{
			FieldInfo &field = tos().getFieldInfo(i);

			const char *n = field.name;
			FieldType type = field.type;

			if( SKIP == type )
				continue;

			while ( n && *n && dest < end )
				*dest++ = *n++;

			if ( ARRAY == type )
			{
				char temp[512];
				physx::shdfnd::snprintf(temp, 512, "[%d]", field.idx);

				if( dest + ::strlen(temp) >= end )
				{
					XML_WARNING("buffer overflow");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
					return false;
				}

				const char *scan = temp;
				while ( *scan && dest < end )
				{
					*dest++ = *scan++;
				}
			}
			else
			{
				if( dest + 1 >= end )
				{
					XML_WARNING("buffer overflow");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
					return false;
				}

				*dest++ = '.';
			}
		}

		while ( name && *name && dest < end )
		{
			*dest++ = *name++;
		}

		*dest = 0;
		//printf("Fully qualified name: %s\n", scratch );

		return true;
	}

	bool XmlDeserializer::processClose(const char *tag,uint32_t depth,bool &isError)
	{
		isError = true; //By default if we return with false it's due to error

		if( strcmp(tag, "NxParameters") == 0 ||
			strcmp(tag, "NvParameters") == 0)
		{
			mInRootElement = false;

			// in xml there's only 1 root element allowed,
			// so we want to stop after the first NvParameters
			isError = false;
			return false;
		}

		static const char *validTags[] = {
			"struct",
			"value",
			"array"
		};

		for(uint32_t i = 0; i < PX_ARRAY_SIZE(validTags); ++i)
		{
			if( 0 != ::strcmp(validTags[i], tag) )
				continue;

#	ifndef NDEBUG
			uint32_t idx = tos().getIndex();
			DEBUG_ASSERT( idx > 0 );

			static FieldType validTypes[] = {
				STRUCT,
				VALUE,
				ARRAY
			};

			// Make gcc happy
			const FieldType* tmp = &validTypes[0];
			PX_UNUSED(tmp);

			FieldType type = tos().getFieldInfo(idx - 1).type;
			PX_UNUSED(type);
			DEBUG_ASSERT( type == SKIP || type == validTypes[i] );
#	endif

			if( !popField() )
				return false;

			if (depth == 1 && mRootIndex > MAX_ROOT_OBJ)
			{
				DEBUG_ASSERT(i == 1);
				mObjects[0].getObject()->destroy();
			}

			return tos().getIndex() ? depth != 0 : popObj();
		}

		return false;
	}

	bool XmlDeserializer::processElement(
		const char *elementName,
		const char *elementData,
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int32_t /*lineno*/)
	{
		//Force DOCTYPE
		if( !mHasDoctype )
		{
			XML_WARNING("DOCTYPE is missing");
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_MISSING_DOCTYPE;
			return false;
		}

		if( strcmp(elementName, "NxParameters")  == 0 ||
			strcmp(elementName, "NvParameters")  == 0)
		{
			if( mObjIndex )
			{
				XML_WARNING("NvParameters must be root element");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_MISSING_ROOT_ELEMENT;
				return false;
			}

			if( mInRootElement )
			{
				XML_WARNING("More than one root element encountered");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			uint32_t ver = ReadVersion(attr);
			if( ver != mVer )
			{
				XML_WARNING("unknown version of APX file format: %u", (unsigned)ver);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_FILE_VERSION;
				return false;
			}

			const char* numObjects = attr.get("numObjects");
			if (numObjects != NULL)
			{
				PX_ASSERT(atoi(numObjects) >= 0);
				const uint32_t num = static_cast<uint32_t>(atoi(numObjects));
				if (num > MAX_ROOT_OBJ)
				{
					XML_WARNING("APX file has more than %d root objects, only %d will be read", num, MAX_ROOT_OBJ);
				}
			}


			++mRootTags;
			mInRootElement = true;

			return true;
		}

		if( mRootTags > 0 && !mInRootElement )
		{
			XML_WARNING("element %s not under root element", elementName);
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_MISSING_ROOT_ELEMENT;
			return false;
		}

		if ( strcmp(elementName, "struct") == 0 )
		{
			const char *name = attr.get("name");

			if( !mObjIndex )
			{
				XML_WARNING("struct-element %s not under value-element", name);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			pushField(name, STRUCT);
		}
		else if ( strcmp(elementName, "value") == 0 )
		{
			if( !mObjIndex ) //Root object?
			{
				const char *className = attr.get("className");

				uint32_t version = ReadVersion(attr);

				Interface *obj = mTraits->createNvParameterized(className, version);
				if( !obj )
				{
					XML_WARNING("failed to create object of type %s and version %u", className, (unsigned)version);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_OBJECT_CREATION_FAILED;
					return false;
				}

				const char *objectName = attr.get("objectName");
				if( objectName )
					obj->setName(objectName);

				if (mRootIndex < MAX_ROOT_OBJ)
				{
					mRootObjs[mRootIndex] = obj;
				}
				mRootIndex++;

				pushObj(obj);
				pushField("", SKIP); //Root <value> should not be used in initAddressString

				return true;
			}

			const char *name = attr.get("name");

			char scratch[2048];
			if( !initAddressString(scratch, sizeof(scratch), name) )
			{
				return false;
			}

			pushField(name, VALUE);

			Interface *obj = tos().getObject();
			if( !obj )
			{
				XML_WARNING("unknown error");
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_UNKNOWN;
				return false;
			}

			Handle handle(*obj, scratch);
			if( !handle.isValid() )
			{
				XML_WARNING("%s: invalid path", scratch);
				DEBUG_ALWAYS_ASSERT();
//				mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
				return true;
			}

			const char *type = attr.get("type");
			const char *expectedType = typeToStr(handle.parameterDefinition()->type());
			if ( type && 0 != physx::shdfnd::stricmp(type, expectedType) )
			{
				XML_WARNING("%s: invalid type %s (expected %s)", scratch, type, expectedType);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_ATTR;
				return false;
			}

			const char *included = attr.get("included");
			if ( included )
			{
				bool isIncludedRef = 0 != atoi(included);
				if( isIncludedRef != handle.parameterDefinition()->isIncludedRef() )
				{
					XML_WARNING("%s: unexpected included-attribute", scratch);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INVALID_ATTR;
					return false;
				}

				const char *className = attr.get("className");
				if( !className )
				{
					if( attr.get("classNames") )
					{
						// Ref is NULL

						Interface *oldObj = 0;
						if( NvParameterized::ERROR_NONE != handle.getParamRef(oldObj) )
							return false;
						if( oldObj )
							oldObj->destroy();

						handle.setParamRef(0);

						return true;
					}
					else
					{
						XML_WARNING("%s: missing both className and classNames attribute", scratch);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_INVALID_ATTR;
						return false;
					}
				}

				uint32_t version = ReadVersion(attr);

				Interface *refObj = 0;
				if( isIncludedRef )
					refObj = mTraits->createNvParameterized(className, version);
				else
				{
					void *buf = mTraits->alloc(sizeof(NvParameters));
					refObj = PX_PLACEMENT_NEW(buf, NvParameters)(mTraits);

					refObj->setClassName(className);
				}

				if( !refObj )
				{
					XML_WARNING("%s: failed to create object of type %s and version %u", scratch, className, (unsigned)version);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_OBJECT_CREATION_FAILED;
					return false;
				}

				if( refObj && (-1 == handle.parameterDefinition()->refVariantValIndex(refObj->className())) )
				{
					char longName[256];
					handle.getLongName(longName, sizeof(longName));
					NV_PARAM_TRAITS_WARNING(
						mTraits,
						"%s: setting reference of invalid class %s",
						longName,
						refObj->className()
						);
				}

				if( NvParameterized::ERROR_NONE != handle.setParamRef(refObj) )
				{
					XML_WARNING("%s: failed to set reference of type %s", scratch, className);
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_INVALID_REFERENCE;
					return false;
				}

				const char *objectName = attr.get("objectName");
				if( objectName && refObj )
					refObj->setName(objectName);

				if( isIncludedRef )
					pushObj(refObj);
				else if ( elementData && refObj )
					refObj->setName(elementData);
			}
			else
			{
				if ( elementData == 0 )
					elementData = "";

				const char *isNull = attr.get("null");
				if( isNull && 0 != atoi(isNull) )
				{
					//Only strings and enums may be NULL so it's safe to call setParamString

					DataType t = handle.parameterDefinition()->type();
					PX_UNUSED(t);
					DEBUG_ASSERT( TYPE_STRING == t || TYPE_ENUM == t );

					handle.setParamString(0);
				}
				else
				{
					if( NvParameterized::ERROR_NONE != handle.strToValue(elementData, 0) )
					{
						XML_WARNING("%s: failed to convert string to value: %10s", scratch, elementData);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_STRING2VAL_FAILED;
						return false;
					}
				}
			} //if ( included )
		}
		else if ( strcmp(elementName, "array") == 0 )
		{
			const char *name = attr.get("name");

			if( !mObjIndex )
			{
				XML_WARNING("array-element %s not under value-element", name);
				DEBUG_ALWAYS_ASSERT();
				mError = Serializer::ERROR_INVALID_NESTING;
				return false;
			}

			int32_t arraySize = 0;
			if ( const char *sz = attr.get("size") )
			{
				PX_ASSERT(atoi(sz) >= 0);
				arraySize = (int32_t)atoi(sz);
			}

			if ( arraySize > 0 )
			{
				Interface *obj = tos().getObject();
				if( !obj )
				{
					XML_WARNING("unknown error");
					DEBUG_ALWAYS_ASSERT();
					mError = Serializer::ERROR_UNKNOWN;
					return false;
				}

				char scratch[2048];
				if( !initAddressString(scratch, sizeof(scratch), name) )
				{
					return false;
				}

				Handle handle(*obj, scratch);
				if( !handle.isValid() )
				{

					mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
					XML_WARNING("%s: invalid path", scratch);
					DEBUG_ALWAYS_ASSERT();

					return false;
				}

				if( !handle.parameterDefinition()->arraySizeIsFixed() )
					if( NvParameterized::ERROR_NONE != handle.resizeArray(arraySize) )
					{
						XML_WARNING("%s: failed to resize array", scratch);
						DEBUG_ALWAYS_ASSERT();
						mError = Serializer::ERROR_INVALID_ARRAY;
						return false;
					}
					if ( elementData )
					{
						const char *scan = elementData;

						handle.set(0);
						const Definition *paramDef = handle.parameterDefinition();
						handle.popIndex();


						if ( paramDef->type() == TYPE_STRUCT )
						{
							// read the structElements field
							const char* structElements = attr.get("structElements");

							int32_t* simpleStructRedirect = getSimpleStructRedirect(static_cast<uint32_t>(paramDef->numChildren()));
							uint32_t numRedirects = 0;
							while (structElements && *structElements)
							{
								char fieldName[64];
								char type[16];

								size_t count = 0;
								while(*structElements != 0 && *structElements != ',' && *structElements != '(')
									fieldName[count++] = *structElements++;
								if( count >= sizeof(fieldName) )
								{
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
									return false;
								}
								fieldName[count] = 0;

								if (*structElements == '(')
								{
									structElements++;
									count = 0;
									while(*structElements != 0 && *structElements != ',' && *structElements != ')')
										type[count++] = *structElements++;
									if( count >= sizeof(type) )
									{
										DEBUG_ALWAYS_ASSERT();
										mError = Serializer::ERROR_INTERNAL_BUFFER_OVERFLOW;
										return false;
									}
									type[count] = 0;

								}
								if (*structElements == ')')
									structElements++;
								if (*structElements == ',')
									structElements++;

								const Definition* childDef = paramDef->child(fieldName, simpleStructRedirect[numRedirects]);
								const char* trueType = childDef ? typeToStr(childDef->type()) : 0;
								if (childDef && ::strcmp(trueType, type) != 0)
								{
									XML_WARNING(
										"%s[].%s: unexpected type: %s (must be %s)",
										scratch, fieldName, type, trueType ? trueType : "");
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INVALID_DATA_TYPE;
									return false;
								}

								// -2 means to reed the data but not storing it
								// -1 means to not read the data as it was not serialized'
								// i = [0 .. n] means to read the data and store it in child i
								if (childDef == NULL)
								{
									simpleStructRedirect[numRedirects] = -2;

									// Fail fast
									XML_WARNING("%s[]: unexpected structure field: %s", scratch, fieldName);
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_INVALID_PARAM_HANDLE;
									return false;
								}

								numRedirects++;
							}

							const int32_t numChildren = paramDef->numChildren();
							for(int32_t i = 0; i < arraySize; ++i)
							{
								handle.set(i);
								for( int32_t j=0; j<numChildren; j++ )
								{
									if (simpleStructRedirect[j] < 0)
									{
										if (simpleStructRedirect[j] < -1)
										{
											// read the data anyways
											scan = skipWhiteSpace(scan);
											if (scan != NULL)
												scan = skipNonWhiteSpace(scan);
										}
										continue;
									}

									scan = skipWhiteSpace(scan);
									if ( !scan ) break;

									handle.set(simpleStructRedirect[j]);

									if( NvParameterized::ERROR_NONE != handle.strToValue(scan, &scan) )
									{
										XML_WARNING("%s: failed to convert string to value: %10s...", scan);
										DEBUG_ALWAYS_ASSERT();
										mError = Serializer::ERROR_STRING2VAL_FAILED;
										return false;
									}

									handle.popIndex();
									if ( !scan ) break;
								}
								handle.popIndex();
								if ( !scan ) break;
							}
						}
						else
						{
							// LRR: wall clock time is the same for this simple loop as the previous
							//      "unrolled" version
							for (int32_t i = 0; i<arraySize; i++)
							{
								handle.set(i);
								if( NvParameterized::ERROR_NONE != handle.strToValue(scan, &scan) )
								{
									XML_WARNING("%s: failed to convert string to value: %10s", scratch, scan);
									DEBUG_ALWAYS_ASSERT();
									mError = Serializer::ERROR_STRING2VAL_FAILED;
									return false;
								}
								handle.popIndex();
							}
						}
					} //if( elementData )
			} //if ( arraySize > 0 )

			pushField(name, ARRAY);
		}
		else
		{
			XML_WARNING("unknown element %s", elementName);
			DEBUG_ALWAYS_ASSERT();
			mError = Serializer::ERROR_UNKNOWN_XML_TAG;
			return false;
		}

		return true;
	}

	int32_t* XmlDeserializer::getSimpleStructRedirect(uint32_t size)
	{
		if (mSimpleStructRedirectSize < size)
		{
			if (mSimpleStructRedirect != NULL)
				mTraits->free(mSimpleStructRedirect);

			if (size < 16)
				size = 16; // just to not allocate all these small things more than once

			mSimpleStructRedirect = (int32_t*)mTraits->alloc(sizeof(int32_t) * size);
			mSimpleStructRedirectSize = size;
		}

		memset(mSimpleStructRedirect, -1, sizeof(int32_t) * size);
		return mSimpleStructRedirect;
	}

}
