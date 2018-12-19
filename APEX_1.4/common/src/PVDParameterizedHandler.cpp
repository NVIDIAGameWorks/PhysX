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



#include "PVDParameterizedHandler.h"
#include "ApexPvdClient.h"

#ifndef WITHOUT_PVD

#include "PxPvdDataStream.h"
#include "nvparameterized/NvParameterized.h"
#include "NvParameters.h"

using namespace nvidia;
using namespace nvidia::shdfnd;

#define SET_PROPERTY_VALUE 	\
	if (pvdAction != PvdAction::DESTROY) \
	{\
		ok = propertyHandle.getParam(val) == NvParameterized::ERROR_NONE; \
		if (ok)\
		{\
			if (isArrayElement)\
				mPvdStream->appendPropertyValueData(DataRef<const uint8_t>((const uint8_t*)&val, sizeof(val)));\
			else\
				mPvdStream->setPropertyValue(pvdInstance, propertyName, val);\
		}\
	}\

namespace physx
{
namespace pvdsdk
{



bool PvdParameterizedHandler::createClass(const NamespacedName& className)
{
	bool create = !mCreatedClasses.contains(className.mName);
	if (create)
	{
		mPvdStream->createClass(className);
		mCreatedClasses.insert(className.mName);
	}

	return create;
}



bool PvdParameterizedHandler::getPvdType(const NvParameterized::Definition& def, NamespacedName& pvdTypeName)
{
	NvParameterized::DataType paramType = def.type();

	bool ok = true;
	switch(paramType)
	{
	case NvParameterized::TYPE_BOOL :
		pvdTypeName = getPvdNamespacedNameForType<bool>();
		break;

	case NvParameterized::TYPE_STRING :
		pvdTypeName = getPvdNamespacedNameForType<const char*>();
		break;

	case NvParameterized::TYPE_I8 :
		pvdTypeName = getPvdNamespacedNameForType<int8_t>();
		break;

	case NvParameterized::TYPE_I16 :
		pvdTypeName = getPvdNamespacedNameForType<int16_t>();
		break;

	case NvParameterized::TYPE_I32 :
		pvdTypeName = getPvdNamespacedNameForType<int32_t>();
		break;

	case NvParameterized::TYPE_I64 :
		pvdTypeName = getPvdNamespacedNameForType<int64_t>();
		break;

	case NvParameterized::TYPE_U8 :
		pvdTypeName = getPvdNamespacedNameForType<uint8_t>();
		break;

	case NvParameterized::TYPE_U16 :
		pvdTypeName = getPvdNamespacedNameForType<uint16_t>();
		break;

	case NvParameterized::TYPE_U32 :
		pvdTypeName = getPvdNamespacedNameForType<uint32_t>();
		break;

	case NvParameterized::TYPE_U64 :
		pvdTypeName = getPvdNamespacedNameForType<uint64_t>();
		break;

	case NvParameterized::TYPE_F32 :
		pvdTypeName = getPvdNamespacedNameForType<float>();
		break;

	case NvParameterized::TYPE_F64 :
		pvdTypeName = getPvdNamespacedNameForType<double>();
		break;

	case NvParameterized::TYPE_VEC2 :
		pvdTypeName = getPvdNamespacedNameForType<PxVec2>();
		break;

	case NvParameterized::TYPE_VEC3 :
		pvdTypeName = getPvdNamespacedNameForType<PxVec3>();
		break;

	case NvParameterized::TYPE_VEC4 :
		pvdTypeName = getPvdNamespacedNameForType<uint16_t>();
		break;

	case NvParameterized::TYPE_QUAT :
		pvdTypeName = getPvdNamespacedNameForType<PxQuat>();
		break;

	case NvParameterized::TYPE_MAT33 :
		pvdTypeName = getPvdNamespacedNameForType<PxMat33>();
		break;

	case NvParameterized::TYPE_BOUNDS3 :
		pvdTypeName = getPvdNamespacedNameForType<PxBounds3>();
		break;

	case NvParameterized::TYPE_MAT44 :
		pvdTypeName = getPvdNamespacedNameForType<PxMat44>();
		break;

	case NvParameterized::TYPE_POINTER :
		pvdTypeName = getPvdNamespacedNameForType<VoidPtr>();
		break;

	case NvParameterized::TYPE_TRANSFORM :
		pvdTypeName = getPvdNamespacedNameForType<PxTransform>();
		break;

	case NvParameterized::TYPE_REF :
	case NvParameterized::TYPE_STRUCT :
		pvdTypeName = getPvdNamespacedNameForType<ObjectRef>();
		break;

	case NvParameterized::TYPE_ARRAY:
		{
			PX_ASSERT(def.numChildren() > 0);
			const NvParameterized::Definition* arrayMemberDef = def.child(0);

			ok = getPvdType(*arrayMemberDef, pvdTypeName);

			// array of strings is not supported by pvd
			if (arrayMemberDef->type() == NvParameterized::TYPE_STRING)
			{
				ok = false;
			}

			break;
		}

	default:
		ok = false;
		break;
	};

	return ok;
}


size_t PvdParameterizedHandler::getStructId(void* structAddress, const char* structName, bool deleteId)
{
	StructId structId(structAddress, structName);

	size_t pvdStructId = 0;
	if (mStructIdMap.find(structId) != NULL)
	{
		pvdStructId = mStructIdMap[structId];
	}
	else
	{
		PX_ASSERT(!deleteId);

		 // addresses are 4 byte aligned, so this id is probably not used by another object
		pvdStructId = mNextStructId++;
		pvdStructId = (pvdStructId << 1) | 1;

		mStructIdMap[structId] = pvdStructId;
	}

	if (deleteId)
	{
		mStructIdMap.erase(structId);
	}

	return pvdStructId;
}


const void* PvdParameterizedHandler::getPvdId(const NvParameterized::Handle& handle, bool deleteId)
{
	void* retVal = 0;
	NvParameterized::DataType type = handle.parameterDefinition()->type();

	switch(type)
	{
	case NvParameterized::TYPE_REF:
		{
			// references use the referenced interface pointer as ID
			NvParameterized::Interface* paramRef = NULL;
			handle.getParamRef(paramRef);
			retVal = (void*)paramRef;
			break;
		}

	case NvParameterized::TYPE_STRUCT:
		{
			// structs use custom ID's, because two structs can have the same location if
			// a struct contains another struct as its first member
			NvParameterized::NvParameters* param = (NvParameterized::NvParameters*)(handle.getInterface());
			if (param != NULL)
			{
				// get struct address
				size_t offset = 0;
				void* structAddress = 0;
				param->getVarPtr(handle, structAddress, offset);

				// create an id from address and name
				retVal = (void*)getStructId(structAddress, handle.parameterDefinition()->longName(), deleteId);
			}
			break;
		}

	default:
		break;
	}

	return retVal;
}


bool PvdParameterizedHandler::setProperty(const void* pvdInstance, NvParameterized::Handle& propertyHandle, bool isArrayElement, PvdAction::Enum pvdAction)
{
	const char* propertyName = propertyHandle.parameterDefinition()->name();
	NvParameterized::DataType propertyType = propertyHandle.parameterDefinition()->type();

	bool ok = true;
	switch(propertyType)
	{
	case NvParameterized::TYPE_BOOL :
		{
			//bool val;
			//SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_STRING :
		{
			if (isArrayElement)
			{
				// pvd doesn't support arrays of strings
				ok = false;
			}
			else
			{
				const char* val;
				ok = propertyHandle.getParamString(val) == NvParameterized::ERROR_NONE;
				if (ok)
				{
					mPvdStream->setPropertyValue(pvdInstance, propertyName, val);
				}
			}
			
			break;
		}

	case NvParameterized::TYPE_I8 :
		{
			int8_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_I16 :
		{
			int16_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_I32 :
		{
			int32_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_I64 :
		{
			int64_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_U8 :
		{
			uint8_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_U16 :
		{
			uint16_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_U32 :
		{
			uint32_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_U64 :
		{
			uint64_t val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_F32 :
		{
			float val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_F64 :
		{
			double val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_VEC2 :
		{
			PxVec2 val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_VEC3 :
		{
			PxVec3 val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_VEC4 :
		{
			PxVec4 val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_QUAT :
		{
			PxQuat val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_MAT33 :
		{
			PxMat33 val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_BOUNDS3 :
		{
			PxBounds3 val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_MAT44 :
		{
			PxMat44 val;
			SET_PROPERTY_VALUE;
			break;
		}

		/*
	case NvParameterized::TYPE_POINTER :
		{
			void* val;
			SET_PROPERTY_VALUE;
			break;
		}
		*/

	case NvParameterized::TYPE_TRANSFORM :
		{
			PxTransform val;
			SET_PROPERTY_VALUE;
			break;
		}

	case NvParameterized::TYPE_STRUCT:
		{
			const void* pvdId = getPvdId(propertyHandle, pvdAction == PvdAction::DESTROY);

			if (pvdId != 0)
			{
				if (!mInstanceIds.contains(pvdId))
				{
					// create pvd instance for struct
					mInstanceIds.insert(pvdId);
					NamespacedName structName(APEX_PVD_NAMESPACE, propertyHandle.parameterDefinition()->structName());
					mPvdStream->createInstance(structName, pvdId);
					mPvdStream->setPropertyValue(pvdInstance, propertyName, DataRef<const uint8_t>((const uint8_t*)&pvdId, sizeof(NvParameterized::Interface*)), getPvdNamespacedNameForType<ObjectRef>());
				}

				// recursively update struct properties
				updatePvd(pvdId, propertyHandle, pvdAction);

				if (pvdAction == PvdAction::DESTROY)
				{
					// destroy pvd instance of struct
					mPvdStream->destroyInstance(pvdId);
					mInstanceIds.erase(pvdId);
				}
			}
			break;
		}

	case NvParameterized::TYPE_REF:
		{
			const void* pvdId = getPvdId(propertyHandle, pvdAction == PvdAction::DESTROY);

			if (pvdId != 0)
			{
				// get a handle in the referenced parameterized object
				NvParameterized::Handle refHandle = propertyHandle;
				propertyHandle.getChildHandle(0, refHandle);
				NvParameterized::Interface* paramRef = NULL;
				ok = refHandle.getParamRef(paramRef) == NvParameterized::ERROR_NONE;

				if (ok)
				{
					if (!mInstanceIds.contains(pvdId))
					{
						// create a pvd instance for the reference
						mInstanceIds.insert(pvdId);
						NamespacedName refClassName(APEX_PVD_NAMESPACE, paramRef->className());
						mPvdStream->createInstance(refClassName, pvdId);
						mPvdStream->setPropertyValue(pvdInstance, propertyName, DataRef<const uint8_t>((const uint8_t*)&pvdId, sizeof(NvParameterized::Interface*)), getPvdNamespacedNameForType<ObjectRef>());
					}

					// recursivly update pvd instance of the referenced object
					refHandle = NvParameterized::Handle(paramRef);
					updatePvd(pvdId, refHandle, pvdAction);

					if (pvdAction == PvdAction::DESTROY)
					{
						// destroy pvd instance of reference
						mPvdStream->destroyInstance(pvdId);
						mInstanceIds.erase(pvdId);
					}
				}
			}
			break;
		}

	case NvParameterized::TYPE_ARRAY:
		{
			const NvParameterized::Definition* def = propertyHandle.parameterDefinition();
			PX_ASSERT(def->numChildren() > 0);

			const NvParameterized::Definition* arrayMemberDef = def->child(0);
			NvParameterized::DataType arrayMemberType = arrayMemberDef->type();

			PX_ASSERT(def->arrayDimension() == 1);
			int32_t arraySize = 0;
			propertyHandle.getArraySize(arraySize);

			if (arraySize > 0)
			{
				if (arrayMemberType == NvParameterized::TYPE_STRUCT || arrayMemberType == NvParameterized::TYPE_REF)
				{
					for (int32_t i = 0; i < arraySize; ++i)
					{
						NvParameterized::Handle childHandle(propertyHandle);
						propertyHandle.getChildHandle(i, childHandle);

						const void* pvdId = getPvdId(childHandle, pvdAction == PvdAction::DESTROY);

						// get the class name of the member
						NamespacedName childClassName(APEX_PVD_NAMESPACE, "");
						if (arrayMemberType == NvParameterized::TYPE_STRUCT)
						{
							childClassName.mName = childHandle.parameterDefinition()->structName();
						}
						else if (arrayMemberType == NvParameterized::TYPE_REF)
						{
							// continue on a handle in the referenced object
							NvParameterized::Interface* paramRef = NULL;
							ok = childHandle.getParamRef(paramRef) == NvParameterized::ERROR_NONE;
							PX_ASSERT(ok);
							if (!ok)
							{
								break;
							}
							childHandle = NvParameterized::Handle(paramRef);
							childClassName.mName = paramRef->className();
						}

						if (!mInstanceIds.contains(pvdId))
						{
							// create pvd instance for struct or ref and add it to the array
							mInstanceIds.insert(pvdId);
							mPvdStream->createInstance(childClassName, pvdId);
							mPvdStream->pushBackObjectRef(pvdInstance, propertyName, pvdId);
						}
						
						// recursively update the array member
						updatePvd(pvdId, childHandle, pvdAction);

						if (pvdAction == PvdAction::DESTROY)
						{
							// destroy pvd instance for struct or ref
							mPvdStream->removeObjectRef(pvdInstance, propertyName, pvdId); // might not be necessary
							mPvdStream->destroyInstance(pvdId);
							mInstanceIds.erase(pvdId);
						}
					}
				}
				else if (pvdAction != PvdAction::DESTROY)
				{
					// for arrays of simple types just update the property values
					NamespacedName pvdTypeName;
					if (getPvdType(*def, pvdTypeName))
					{
						mPvdStream->beginSetPropertyValue(pvdInstance, propertyName, pvdTypeName);
						for (int32_t i = 0; i < arraySize; ++i)
						{
							NvParameterized::Handle childHandle(propertyHandle);
							propertyHandle.getChildHandle(i, childHandle);

							setProperty(pvdInstance, childHandle, true, pvdAction);
						}
						mPvdStream->endSetPropertyValue();
					}
				}
			}
			break;
		}

	default:
		ok = false;
		break;
	};

	return ok;
}



void PvdParameterizedHandler::initPvdClasses(const NvParameterized::Definition& paramDefinition, const char* className)
{
	NamespacedName pvdClassName(APEX_PVD_NAMESPACE, className);

	// iterate all properties
	const int numChildren = paramDefinition.numChildren();
	for (int i = 0; i < numChildren; i++)
	{
		const NvParameterized::Definition* childDef = paramDefinition.child(i);

		const char* propertyName = childDef->name();
		NvParameterized::DataType propertyDataType = childDef->type();
		

		// First, recursively create pvd classes for encountered structs
		//
		// if it's an array, continue with its member type, and remember that it's an array
		bool isArray = false;
		if (propertyDataType == NvParameterized::TYPE_ARRAY)
		{
			PX_ASSERT(childDef->numChildren() > 0);

			const NvParameterized::Definition* arrayMemberDef = childDef->child(0);
			if (arrayMemberDef->type() == NvParameterized::TYPE_STRUCT)
			{
				NamespacedName memberClassName(APEX_PVD_NAMESPACE, arrayMemberDef->structName());
				if (createClass(memberClassName))
				{
					// only recurse if this we encounter the struct the first time and a class has been created
					initPvdClasses(*arrayMemberDef, memberClassName.mName);
				}
			}

			isArray = true;
		}
		else if (propertyDataType == NvParameterized::TYPE_STRUCT)
		{
			// create classes for structs
			// (doesn't work for refs, looks like Definitions don't contain the Definitions of references)

			NamespacedName childClassName(APEX_PVD_NAMESPACE, childDef->structName());
			if (createClass(childClassName))
			{
				// only recurse if this we encounter the struct the first time and a class has been created
				initPvdClasses(*childDef, childClassName.mName);
			}
		}


		// Then, create the property
		NamespacedName typeName;
		if (!childDef->hint("NOPVD") && getPvdType(*childDef, typeName))
		{
			mPvdStream->createProperty(pvdClassName, propertyName, "", typeName, isArray ? PropertyType::Array : PropertyType::Scalar);
		}
	}
}


void PvdParameterizedHandler::updatePvd(const void* pvdInstance, NvParameterized::Handle& paramsHandle, PvdAction::Enum pvdAction)
{
	// iterate all properties
	const int numChildren = paramsHandle.parameterDefinition()->numChildren();
	for (int i = 0; i < numChildren; i++)
	{
		paramsHandle.set(i);

		if (!paramsHandle.parameterDefinition()->hint("NOPVD"))
		{
			setProperty(pvdInstance, paramsHandle, false, pvdAction);
		}
		paramsHandle.popIndex();
	}
}

}
}

#endif