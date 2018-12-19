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


#if TODO_PVD_NXPARAM_SERIALIZER

#include "ApexUsingNamespace.h"
#include "PvdNxParamSerializer.h"
#include "nvparameterized/NvParameterized.h"
#include "PvdConnection.h"
#include "PVDCommLayerDebuggerStream.h"
#include "ApexString.h"
#include "PxMat33.h"
#include "PxMat34Legacy.h"

using namespace PVD;
using namespace nvidia::apex;
using namespace NvParameterized;

namespace PvdNxParamSerializer
{


inline void Append(ApexSimpleString& inStr, const char* inAppend)
{
	while (inAppend && *inAppend)
	{
		inStr += *inAppend;
		++inAppend;
	}
}

inline const char* GetVariableName(ApexSimpleString& inWorkString, const char* inNamePrefix, const char* inVarName)
{
	if (inNamePrefix && *inNamePrefix)
	{
		Append(inWorkString, inNamePrefix);
		Append(inWorkString, ".");
		Append(inWorkString, inVarName);
		return inWorkString.c_str();
	}
	return inVarName;
}


/**
 *	The serialization architecture is complicated by dynamic arrays of information.  I made a decision that a dynamic array cannot
 *	contain another dynamic array in the debugger's world thus if the real world has this limitation then there will be data
 *	loss when dealing with the debugger.
 *
 *	In general, the TopLevel handler sends across properties as it encounters them giving property names dotted notations
 *	when they are part of a struct.  The debugger UI on the other end has logic create objects based on the dotted names
 *	and thus even though the database layer doesn't support struct objects the UI makes it appear as though it does.
 *	Static or non-resizeable-arrays are treated as a struct.
 *
 *	Dynamic arrays require at least two passes and perhaps three.  The first pass only needs to check out the first object.
 *	The next pass creates all ref objects if necessary and needs to pass over every item in the array.
 *	the third pass collects data values and sends such values over the wire using the array blocks.
 *
 *	An initial test with the APEX integration tests sent 11 megs of data into the database; so I imagine
 *	most of the information in the APEX system, at least as far as initialization and static
 *	or asset based information is being sent to the debugger.
 *
 *	Due to time constraints I haven't been able to upgrade the PVD UI to handle arrays well, which is unfortunate because
 *	it seems that most of the APEX information is in arrays.
 */

#define HANDLE_PARAM_TYPE( datatype, paramFuncName ) { \
		datatype tmp; \
		handle.paramFuncName(tmp); \
		return HandleDataType( tmp, theVariableName ); }


class TopLevelParamTreeHandler
{
protected:
	NvParameterized::Interface* mObj;
	PvdDataStream* mRemoteDebugger;
	uint64_t mCurPvdObj;
	const char* mVariablePrefix;
	uint64_t mDynamicArrayHandle;

public:
	TopLevelParamTreeHandler(NvParameterized::Interface* obj, PvdDataStream* remoteDebugger, uint64_t inCurrentObject, const char* inVariablePrefix = NULL)
		: mObj(obj)
		, mRemoteDebugger(remoteDebugger)
		, mCurPvdObj(inCurrentObject)
		, mVariablePrefix(inVariablePrefix)
	{
		mDynamicArrayHandle = mCurPvdObj + 1;
	}

	TopLevelParamTreeHandler(const TopLevelParamTreeHandler& inOther, const char* inNewPrefix)
		: mObj(inOther.mObj)
		, mRemoteDebugger(inOther.mRemoteDebugger)
		, mCurPvdObj(inOther.mCurPvdObj)
		, mVariablePrefix(inNewPrefix)
		, mDynamicArrayHandle(inOther.mDynamicArrayHandle)
	{}

	template<typename THandlerType>
	inline NvParameterized::ErrorType DoHandleStruct(NvParameterized::Handle& inHandle, const char* inParamName)
	{
		const NvParameterized::Definition* paramDef = inHandle.parameterDefinition();
		const char* newPrefix = mVariablePrefix;
		ApexSimpleString theWorker;
		if (inParamName && *inParamName)
		{
			newPrefix = GetVariableName(theWorker, mVariablePrefix, inParamName);
		}

		THandlerType theNewHandler(*this, newPrefix);
		for (int i = 0; i < paramDef->numChildren(); ++i)
		{
			inHandle.set(i);
			theNewHandler.TraverseParamDefTree(inHandle);
			inHandle.popIndex();
		}
		TransferStackInformationBack(theNewHandler);
		return(NvParameterized::ERROR_NONE);
	}
	template<typename THandlerType>
	inline NvParameterized::ErrorType DoHandleArray(NvParameterized::Handle& handle, const char* paramName)
	{
		int arraySize = 0;

		const char* arrayName = paramName;
		ApexSimpleString theVariableNamePrefix;
		Append(theVariableNamePrefix, arrayName);

		const Definition* theDef = handle.parameterDefinition();
		bool isFixedSize = theDef->arraySizeIsFixed();
		isFixedSize = false;

		if (handle.getArraySize(arraySize) != NvParameterized::ERROR_NONE)
		{
			return(ERROR_INVALID_ARRAY_SIZE);
		}

		ApexSimpleString theWorkString(theVariableNamePrefix);


		for (int i = 0; i < arraySize; ++i)
		{
			theWorkString = theVariableNamePrefix;
			theWorkString += '[';
			char tempBuf[20];
			shdfnd::snprintf(tempBuf, 20, "%7d", i);
			Append(theWorkString, tempBuf);
			theWorkString += ']';
			handle.set(i);
			THandlerType theNewHandler(*this, theWorkString.c_str());
			theNewHandler.TraverseParamDefTree(handle);
			handle.popIndex();
			TransferStackInformationBack(theNewHandler);
		}
		return(NvParameterized::ERROR_NONE);
	}
	virtual ~TopLevelParamTreeHandler() {}
	virtual void TransferStackInformationBack(const TopLevelParamTreeHandler& inOther)
	{
		mDynamicArrayHandle = inOther.mDynamicArrayHandle;
	}
	virtual NvParameterized::ErrorType HandleStruct(NvParameterized::Handle& handle, const char* inParamName);
	virtual NvParameterized::ErrorType HandleArray(NvParameterized::Handle& handle, const char* paramName);
	virtual NvParameterized::ErrorType HandleDynamicArray(NvParameterized::Handle& handle, const char* paramName);
	virtual NvParameterized::ErrorType HandleRef(NvParameterized::Handle& handle, const char*);
	virtual NvParameterized::ErrorType HandleProperty(const PvdCommLayerValue& inValue, const char* inParamName);
	template<typename TDataType>
	inline NvParameterized::ErrorType HandleDataType(const TDataType& inDataType, const char* inParamName)
	{
		return HandleProperty(CreateCommLayerValue(inDataType), inParamName);
	}

	virtual NvParameterized::ErrorType TraverseParamDefTree(NvParameterized::Handle& handle);
};

//Run through a type define all the properties ignoring dynamic arrays and
//ref objs.
class PropertyDefinitionTreeHandler : public TopLevelParamTreeHandler
{
	physx::Array<uint32_t> mProperties;
	physx::Array<PVD::PvdCommLayerDatatype> mDatatypes;
	uint32_t mClassKey;
	bool hasRefs;
public:
	PropertyDefinitionTreeHandler(const TopLevelParamTreeHandler& inOther, uint32_t inClassKey)
		: TopLevelParamTreeHandler(inOther, "")
		, mClassKey(inClassKey)
		, hasRefs(false)
	{}

	PropertyDefinitionTreeHandler(const TopLevelParamTreeHandler& inOther, const char* inParamName)
		: TopLevelParamTreeHandler(inOther, inParamName)
	{
		const PropertyDefinitionTreeHandler& realOther = static_cast<const PropertyDefinitionTreeHandler&>(inOther);
		mClassKey = realOther.mClassKey;
		hasRefs |= realOther.hasRefs;
	}

	virtual NvParameterized::ErrorType HandleDynamicArray(NvParameterized::Handle&, const char*)
	{
		return NvParameterized::ERROR_NONE;
	}
	virtual NvParameterized::ErrorType HandleArray(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleArray<PropertyDefinitionTreeHandler>(handle, inParamName);
	}
	virtual NvParameterized::ErrorType HandleRef(NvParameterized::Handle&, const char* inParamName)
	{
		HandleProperty(createInstanceId(0), inParamName);
		hasRefs = true;
		return NvParameterized::ERROR_NONE;
	}
	virtual NvParameterized::ErrorType HandleStruct(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleStruct<PropertyDefinitionTreeHandler>(handle, inParamName);
	}
	virtual void TransferStackInformationBack(const TopLevelParamTreeHandler& inOther)
	{
		const PropertyDefinitionTreeHandler& realOther = static_cast<const PropertyDefinitionTreeHandler&>(inOther);
		hasRefs |= realOther.hasRefs;
		for (uint32_t idx = 0; idx < realOther.mProperties.size(); ++idx)
		{
			mProperties.pushBack(realOther.mProperties[idx]);
		}
	}

	virtual NvParameterized::ErrorType HandleProperty(const PvdCommLayerValue& inValue, const char* inParamName)
	{
		uint32_t thePropertyKey = HashFunction(inParamName);
		mRemoteDebugger->defineProperty(mClassKey, inParamName, NULL, inValue.getDatatype(), thePropertyKey);
		mProperties.pushBack(thePropertyKey);
		mDatatypes.pushBack(inValue.getDatatype());
		return NvParameterized::ERROR_NONE;
	}
	uint32_t GetPropertyCount()
	{
		return mProperties.size();
	}
	const uint32_t* GetProperties()
	{
		return mProperties.begin();
	}
	const PVD::PvdCommLayerDatatype* getDatatypes()
	{
		return mDatatypes.begin();
	}
	bool HasRefs()
	{
		return hasRefs;
	}
};

//Simply create the parameter ref objects.
class ParamRefTreeHandler : public TopLevelParamTreeHandler
{
public:
	ParamRefTreeHandler(TopLevelParamTreeHandler& inOther)
		: TopLevelParamTreeHandler(inOther)
	{
	}

	ParamRefTreeHandler(TopLevelParamTreeHandler& inOther, const char* inParamName)
		: TopLevelParamTreeHandler(inOther, inParamName)
	{
	}

	virtual NvParameterized::ErrorType HandleStruct(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleStruct<ParamRefTreeHandler>(handle, inParamName);
	}

	virtual NvParameterized::ErrorType HandleArray(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleArray<ParamRefTreeHandler>(handle, inParamName);
	}
	virtual NvParameterized::ErrorType HandleDynamicArray(NvParameterized::Handle&, const char*)
	{
		return NvParameterized::ERROR_NONE;
	}
	virtual NvParameterized::ErrorType HandleProperty(const PvdCommLayerValue& , const char*)
	{
		return NvParameterized::ERROR_NONE;
	}
};

class ValueRecorderTreeHandler : public TopLevelParamTreeHandler
{
	physx::Array<PvdCommLayerValue>* mValues;
public:
	ValueRecorderTreeHandler(TopLevelParamTreeHandler& inHandler, physx::Array<PvdCommLayerValue>* inValues)
		: TopLevelParamTreeHandler(inHandler, "")
		, mValues(inValues)
	{
	}

	ValueRecorderTreeHandler(TopLevelParamTreeHandler& inHandler, const char* inParamName)
		: TopLevelParamTreeHandler(inHandler, inParamName)
	{
		const ValueRecorderTreeHandler& realOther = static_cast< const ValueRecorderTreeHandler& >(inHandler);
		mValues = realOther.mValues;
	}

	virtual NvParameterized::ErrorType HandleDynamicArray(NvParameterized::Handle&, const char*)
	{
		return NvParameterized::ERROR_NONE;
	}

	virtual NvParameterized::ErrorType HandleStruct(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleStruct<ValueRecorderTreeHandler>(handle, inParamName);
	}

	virtual NvParameterized::ErrorType HandleArray(NvParameterized::Handle& handle, const char* inParamName)
	{
		return DoHandleArray<ValueRecorderTreeHandler>(handle, inParamName);
	}

	virtual NvParameterized::ErrorType HandleRef(NvParameterized::Handle& handle, const char* inParamName)
	{
		NvParameterized::Interface* refObj = 0;
		if (handle.getParamRef(refObj) != NvParameterized::ERROR_NONE)
		{
			return(NvParameterized::ERROR_INVALID_PARAMETER_HANDLE);
		}
		uint64_t refObjId(PtrToPVD(refObj));
		return HandleProperty(createInstanceId(refObjId), inParamName);
	}

	virtual NvParameterized::ErrorType HandleProperty(const PvdCommLayerValue& inValue, const char*)
	{
		mValues->pushBack(inValue);
		return NvParameterized::ERROR_NONE;
	}

};

class DynamicArrayParamTreeHandler : public TopLevelParamTreeHandler
{
	physx::Array<PvdCommLayerValue>	mValues;
	ApexSimpleString					mTypeName;
	uint64_t								mInstanceHandle;
public:
	DynamicArrayParamTreeHandler(const TopLevelParamTreeHandler& inOther, const char* inNewPrefix, uint64_t inInstanceHandle)
		: TopLevelParamTreeHandler(inOther, "")
		, mInstanceHandle(inInstanceHandle)
	{
		Append(mTypeName, mObj->className());
		mTypeName += '.';
		Append(mTypeName, inNewPrefix);
	}

	virtual NvParameterized::ErrorType TraverseParamDefTree(NvParameterized::Handle& handle)
	{
		const NvParameterized::Definition* theDef = handle.parameterDefinition();
		int arraySize = 0;
		handle.getArraySize(arraySize);
		if (arraySize > 0)
		{
			uint32_t theClassKey((uint32_t)(size_t)theDef);
			mRemoteDebugger->createClass(mTypeName.c_str(), theClassKey);
			handle.set(0);
			PropertyDefinitionTreeHandler theHandler(*this, theClassKey);
			theHandler.TraverseParamDefTree(handle);
			handle.popIndex();
			uint32_t thePropertyCount(theHandler.GetPropertyCount());
			if (thePropertyCount)
			{
				if (theHandler.HasRefs())
				{
					for (int idx = 0; idx < arraySize; ++idx)
					{
						handle.set(idx);
						ParamRefTreeHandler refHandler(*this);
						refHandler.TraverseParamDefTree(handle);   //Create the ref objects
						handle.popIndex();
					}
				}
				mValues.reserve(thePropertyCount);
				mRemoteDebugger->beginArrayBlock(theClassKey, mInstanceHandle, theHandler.GetProperties(), theHandler.getDatatypes(), thePropertyCount);
				for (int idx = 0; idx < arraySize; ++idx)
				{
					handle.set(idx);
					ValueRecorderTreeHandler valueRecorder(*this, &mValues);
					valueRecorder.TraverseParamDefTree(handle);   //Set the values in the array.
					handle.popIndex();
					uint32_t theValueSize(mValues.size());
					if (theValueSize >= thePropertyCount)
					{
						mRemoteDebugger->sendArrayObject(&mValues[0]);
					}
					mValues.clear();
				}
				mRemoteDebugger->endArrayBlock();
			}
		}
		return NvParameterized::ERROR_NONE;
	}
};

NvParameterized::ErrorType TopLevelParamTreeHandler::HandleStruct(NvParameterized::Handle& handle, const char* inParamName)
{
	return DoHandleStruct<TopLevelParamTreeHandler>(handle, inParamName);
}

NvParameterized::ErrorType TopLevelParamTreeHandler::HandleArray(NvParameterized::Handle& handle, const char* paramName)
{
	return DoHandleArray<TopLevelParamTreeHandler>(handle, paramName);
}

NvParameterized::ErrorType TopLevelParamTreeHandler::HandleDynamicArray(NvParameterized::Handle& handle, const char* paramName)
{
	int arraySize = 0;
	handle.getArraySize(arraySize);
	if (arraySize > 0)
	{
		uint64_t theArrayHandle = mDynamicArrayHandle;
		++mDynamicArrayHandle;
		DynamicArrayParamTreeHandler theHandler(*this, paramName, theArrayHandle);
		theHandler.TraverseParamDefTree(handle);
		HandleProperty(PVD::createInstanceId(theArrayHandle), paramName);
	}
	else
	{
		HandleProperty(PVD::createInstanceId(0), paramName);
	}
	return NvParameterized::ERROR_NONE;
}

NvParameterized::ErrorType TopLevelParamTreeHandler::HandleRef(NvParameterized::Handle& handle, const char* inParamName)
{
	const NvParameterized::Definition* paramDef = handle.parameterDefinition();
	bool includedRef = false;
	for (int j = 0; j < paramDef->numHints(); j++)
	{
		const NvParameterized::Hint* hint = paramDef->hint(j);

		if (strcmp("INCLUDED", hint->name()) == 0 && hint->type() == NvParameterized::TYPE_U64)
		{
			if (hint->asUInt())
			{
				includedRef = true;
			}
		}
	}

	NvParameterized::Interface* refObj = 0;
	if (handle.getParamRef(refObj) != NvParameterized::ERROR_NONE)
	{
		return(NvParameterized::ERROR_INVALID_PARAMETER_HANDLE);
	}
	uint64_t refObjId(PtrToPVD(refObj));

	if (includedRef)
	{
		//traversalState state;
		if (!refObj)
		{
			return(NvParameterized::ERROR_INVALID_REFERENCE_VALUE);
		}
		const char* refName = refObj->className();
		PVD::CreateObject(mRemoteDebugger, refObjId, refName);
		TopLevelParamTreeHandler theHandler(refObj, mRemoteDebugger, refObjId);
		NvParameterized::Handle refHandle(*refObj);
		theHandler.TraverseParamDefTree(refHandle);
		HandleProperty(PVD::createInstanceId(refObjId), inParamName);
		return NvParameterized::ERROR_NONE;
	}
	else
	{
		const char* refName = paramDef->name();
		PVD::CreateObject(mRemoteDebugger, refObjId, refName);
		PVD::SetPropertyValue(mRemoteDebugger, refObjId, CreateCommLayerValue(refObj->className()), true, "type");
		PVD::SetPropertyValue(mRemoteDebugger, refObjId, CreateCommLayerValue(refObj->name()), true, "name");
	}

	HandleProperty(createInstanceId(refObjId), inParamName);
	//exit here?
	return(NvParameterized::ERROR_NONE);
}
NvParameterized::ErrorType TopLevelParamTreeHandler::HandleProperty(const PvdCommLayerValue& inValue, const char* inParamName)
{
	PVD::SetPropertyValue(mRemoteDebugger, mCurPvdObj, inValue, true, inParamName);
	return NvParameterized::ERROR_NONE;
}

NvParameterized::ErrorType TopLevelParamTreeHandler::TraverseParamDefTree(NvParameterized::Handle& handle)
{
	if (handle.numIndexes() < 1)
	{
		if (mObj->getParameterHandle("", handle) != NvParameterized::ERROR_NONE)
		{
			return(NvParameterized::ERROR_INVALID_PARAMETER_HANDLE);
		}
	}
	const NvParameterized::Definition* paramDef = handle.parameterDefinition();
	ApexSimpleString tmpStr;
	const char* theVariableName(GetVariableName(tmpStr, mVariablePrefix, paramDef->name()));
	switch (paramDef->type())
	{
	case TYPE_ARRAY:
		if (paramDef->arraySizeIsFixed())
		{
			return HandleArray(handle, theVariableName);
		}
		return HandleDynamicArray(handle, theVariableName);
	case TYPE_STRUCT:
		return HandleStruct(handle, theVariableName);

	case TYPE_BOOL:
		HANDLE_PARAM_TYPE(bool, getParamBool);

	case TYPE_STRING:
		HANDLE_PARAM_TYPE(const char*, getParamString);

	case TYPE_ENUM:
		HANDLE_PARAM_TYPE(const char*, getParamEnum);

	case TYPE_REF:
		return HandleRef(handle, theVariableName);

	case TYPE_I8:
		HANDLE_PARAM_TYPE(int8_t, getParamI8);
	case TYPE_I16:
		HANDLE_PARAM_TYPE(int16_t, getParamI16);
	case TYPE_I32:
		HANDLE_PARAM_TYPE(int32_t, getParamI32);
	case TYPE_I64:
		HANDLE_PARAM_TYPE(int64_t, getParamI64);

	case TYPE_U8:
		HANDLE_PARAM_TYPE(uint8_t, getParamU8);
	case TYPE_U16:
		HANDLE_PARAM_TYPE(uint16_t, getParamU16);
	case TYPE_U32:
		HANDLE_PARAM_TYPE(uint32_t, getParamU32);
	case TYPE_U64:
		HANDLE_PARAM_TYPE(uint64_t, getParamU64);

	case TYPE_F32:
		HANDLE_PARAM_TYPE(float, getParamF32);
	case TYPE_F64:
		HANDLE_PARAM_TYPE(double, getParamF64);

	case TYPE_VEC2:
		HANDLE_PARAM_TYPE(physx::PxVec2, getParamVec2);

	case TYPE_VEC3:
		HANDLE_PARAM_TYPE(physx::PxVec3, getParamVec3);

	case TYPE_VEC4:
		HANDLE_PARAM_TYPE(physx::PxVec4, getParamVec4);

	case TYPE_TRANSFORM:
		HANDLE_PARAM_TYPE(physx::PxTransform, getParamTransform);

	case TYPE_QUAT:
		HANDLE_PARAM_TYPE(physx::PxQuat, getParamQuat);

	case TYPE_MAT33:
	{
		physx::PxMat33 tmp;
		handle.getParamMat33(tmp);
		return HandleDataType(physx::PxMat33(tmp), theVariableName);
	}

	case TYPE_MAT34:
	{
		physx::PxMat44 tmp;
		handle.getParamMat34(tmp);
		return HandleDataType(PxMat34Legacy(tmp), theVariableName);
	}

	case TYPE_BOUNDS3:
		HANDLE_PARAM_TYPE(physx::PxBounds3, getParamBounds3);

	case TYPE_POINTER:
	case TYPE_MAT44: //mat44 unhandled for now
	case TYPE_UNDEFINED:
	case TYPE_LAST:
		return NvParameterized::ERROR_TYPE_NOT_SUPPORTED;
	}
	return NvParameterized::ERROR_NONE;
}



NvParameterized::ErrorType
traverseParamDefTree(NvParameterized::Interface& obj,
                     PVD::PvdDataStream* remoteDebugger,
                     void* curPvdObj,
                     NvParameterized::Handle& handle)
{
	TopLevelParamTreeHandler theHandler(&obj, remoteDebugger, PtrToPVD(curPvdObj));
	theHandler.TraverseParamDefTree(handle);
	return(NvParameterized::ERROR_NONE);
}
}

#endif