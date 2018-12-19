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

#ifndef PXPVDSDK_PXPVDOBJECTMODEL_H
#define PXPVDSDK_PXPVDOBJECTMODEL_H

#include "PsBasicTemplates.h"
#include "PxPvdObjectModelMetaData.h"

namespace physx
{
namespace pvdsdk
{

#if PX_VC == 11 || PX_VC == 12 || PX_VC == 14
#pragma warning(push)
#pragma warning(disable : 4435) // 'class1' : Object layout under /vd2 will change due to virtual base 'class2'
#endif

class PvdInputStream;
class PvdOutputStream;

struct InstanceDescription
{
	int32_t mId;
	int32_t mClassId;
	void* mInstPtr;
	bool mAlive;

	InstanceDescription(int32_t id, int32_t classId, void* inst, bool alive)
	: mId(id), mClassId(classId), mInstPtr(inst), mAlive(alive)
	{
	}
	InstanceDescription() : mId(-1), mClassId(-1), mInstPtr(NULL), mAlive(false)
	{
	}
	operator void*()
	{
		PX_ASSERT(mAlive);
		if(mAlive)
			return mInstPtr;
		return NULL;
	}
	operator int32_t()
	{
		return mId;
	}
};

typedef physx::shdfnd::Pair<int32_t, int32_t> InstancePropertyPair;

class PvdObjectModelBase
{
  protected:
	virtual ~PvdObjectModelBase()
	{
	}

  public:
	virtual void addRef() = 0;
	virtual void release() = 0;
	virtual void* idToPtr(int32_t instId) const = 0;
	virtual int32_t ptrToId(void* instPtr) const = 0;
	virtual InstanceDescription idToDescriptor(int32_t instId) const = 0;
	virtual InstanceDescription ptrToDescriptor(void* instPtr) const = 0;
	virtual Option<ClassDescription> getClassOf(void* instId) const = 0;
	virtual const PvdObjectModelMetaData& getMetaData() const = 0;
};

class PvdObjectModelMutator : public virtual PvdObjectModelBase
{
  protected:
	virtual ~PvdObjectModelMutator()
	{
	}

  public:
	// if the instance is alive, this destroyes any arrays and sets the instance back to its initial state.
	virtual InstanceDescription createInstance(int32_t clsId, int32_t instId) = 0;
	virtual InstanceDescription createInstance(int32_t clsId) = 0;
	// Instances that are pinned are not removed from the system, ever.
	// This means that createInstance, pinInstance, deleteInstance
	// can be called in this order and you can still call getClassOf, etc. on the instances.
	// The instances will never be removed from memory if they are pinned, so use at your
	// careful discretion.
	virtual void pinInstance(void* instId) = 0;
	virtual void unPinInstance(void* instId) = 0;
	// when doing capture, should update all events in a section at once, otherwis there possible parse data
	// incompltely.
	virtual void recordCompletedInstances() = 0;

	virtual void destroyInstance(void* instId) = 0;
	virtual int32_t getNextInstanceHandleValue() const = 0;
	// reserve a set of instance handle values by getting the current, adding an amount to it
	// and setting the value.  You can never set the value lower than it already is, it only climbs.
	virtual void setNextInstanceHandleValue(int32_t hdlValue) = 0;
	// If incoming type is provided, then we may be able to marshal simple types
	// This works for arrays, it just completely replaces the entire array.
	// Because if this, it is an error of the property identifier
	virtual bool setPropertyValue(void* instId, int32_t propId, const uint8_t* data, uint32_t dataLen,
	                              int32_t incomingType) = 0;
	// Set a set of properties defined by a property message
	virtual bool setPropertyMessage(void* instId, int32_t msgId, const uint8_t* data, uint32_t dataLen) = 0;
	// insert an element(s) into array index.  If index > numElements, element(s) is(are) appended.
	virtual bool insertArrayElement(void* instId, int32_t propId, int32_t index, const uint8_t* data, uint32_t dataLen,
	                                int32_t incomingType = -1) = 0;
	virtual bool removeArrayElement(void* instId, int32_t propId, int32_t index) = 0;
	// Add this array element to end end if it doesn't already exist in the array.
	// The option is false if there was an error with the function call.
	// The integer has no value if nothing was added, else it tells you the index
	// where the item was added.  Comparison is done using memcmp.
	virtual Option<int32_t> pushBackArrayElementIf(void* instId, int32_t propId, const uint8_t* data, uint32_t dataLen,
	                                               int32_t incomingType = -1) = 0;
	// Remove an array element if it exists in the array.
	// The option is false if there was an error with the function call.
	// the integer has no value if the item wasn't found, else it tells you the index where
	// the item resided.  Comparison is memcmp.
	virtual Option<int32_t> removeArrayElementIf(void* instId, int32_t propId, const uint8_t* data, uint32_t dataLen,
	                                             int32_t incomingType = -1) = 0;
	virtual bool setArrayElementValue(void* instId, int32_t propId, int32_t propIdx, const uint8_t* data,
	                                  uint32_t dataLen, int32_t incomingType) = 0;

	virtual void originShift(void* instId, PxVec3 shift) = 0;

	InstanceDescription createInstance(const NamespacedName& name)
	{
		return createInstance(getMetaData().findClass(name)->mClassId);
	}
	template <typename TDataType>
	bool setPropertyValue(void* instId, const char* propName, const TDataType* dtype, uint32_t count)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return false;
		}
		const PropertyDescription& prop(descOpt);
		Option<ClassDescription> incomingCls(getMetaData().findClass(getPvdNamespacedNameForType<TDataType>()));
		if(incomingCls.hasValue())
			return setPropertyValue(instId, prop.mPropertyId, reinterpret_cast<const uint8_t*>(dtype),
			                        sizeof(*dtype) * count, incomingCls.getValue().mClassId);
		return false;
	}

	// Simplest possible setPropertyValue
	template <typename TDataType>
	bool setPropertyValue(void* instId, const char* propName, const TDataType& dtype)
	{
		return setPropertyValue(instId, propName, &dtype, 1);
	}

	template <typename TDataType>
	bool setPropertyMessage(void* instId, const TDataType& msg)
	{
		Option<PropertyMessageDescription> msgId =
		    getMetaData().findPropertyMessage(getPvdNamespacedNameForType<TDataType>());
		if(msgId.hasValue() == false)
			return false;
		return setPropertyMessage(instId, msgId.getValue().mMessageId, reinterpret_cast<const uint8_t*>(&msg),
		                          sizeof(msg));
	}
	template <typename TDataType>
	bool insertArrayElement(void* instId, const char* propName, int32_t idx, const TDataType& dtype)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return false;
		}
		const PropertyDescription& prop(descOpt);
		Option<ClassDescription> incomingCls(getMetaData().findClass(getPvdNamespacedNameForType<TDataType>()));
		if(incomingCls.hasValue())
		{
			return insertArrayElement(instId, prop.mPropertyId, idx, reinterpret_cast<const uint8_t*>(&dtype),
			                          sizeof(dtype), incomingCls.getValue().mClassId);
		}
		return false;
	}

	bool removeArrayElement(void* instId, const char* propName, int32_t idx)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return false;
		}
		const PropertyDescription& prop(descOpt);
		return removeArrayElement(instId, prop.mPropertyId, idx);
	}
	template <typename TDataType>
	Option<int32_t> pushBackArrayElementIf(void* instId, const char* pname, const TDataType& item)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, pname));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return None();
		}
		const PropertyDescription& prop(descOpt);
		Option<ClassDescription> incomingCls(getMetaData().findClass(getPvdNamespacedNameForType<TDataType>()));
		if(incomingCls.hasValue() && (incomingCls.getValue().mClassId == prop.mDatatype))
		{
			return pushBackArrayElementIf(instId, prop.mPropertyId, reinterpret_cast<const uint8_t*>(&item),
			                              sizeof(item), incomingCls.getValue().mClassId);
		}
		return None();
	}
	template <typename TDataType>
	Option<int32_t> removeArrayElementIf(void* instId, const char* propId, const TDataType& item)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propId));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return None();
		}
		const PropertyDescription& prop(descOpt);
		Option<ClassDescription> incomingCls(getMetaData().findClass(getPvdNamespacedNameForType<TDataType>()));
		if(incomingCls.hasValue() && (incomingCls.getValue().mClassId == prop.mDatatype))
		{
			return removeArrayElementIf(instId, prop.mPropertyId, reinterpret_cast<const uint8_t*>(&item), sizeof(item),
			                            incomingCls.getValue().mClassId);
		}
		return None();
	}
	template <typename TDataType>
	bool setArrayElementValue(void* instId, const char* propName, int32_t propIdx, TDataType& item)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return false;
		}
		const PropertyDescription& prop(descOpt);
		Option<ClassDescription> incomingCls(getMetaData().findClass(getPvdNamespacedNameForType<TDataType>()));
		if(incomingCls.hasValue() && (incomingCls.getValue().mClassId == prop.mDatatype))
			return setArrayElementValue(instId, prop.mPropertyId, propIdx, reinterpret_cast<const uint8_t*>(&item),
			                            sizeof(item), incomingCls.getValue().mClassId);
		PX_ASSERT(false);
		return false;
	}
};

class PvdObjectModelReader : public virtual PvdObjectModelBase
{
  protected:
	virtual ~PvdObjectModelReader()
	{
	}

  public:
	// Return the byte size of a possible nested property
	virtual uint32_t getPropertyByteSize(void* instId, int32_t propId) = 0;
	uint32_t getPropertyByteSize(void* instId, String propName)
	{
		int32_t propId = getMetaData().findProperty(getClassOf(instId)->mClassId, propName)->mPropertyId;
		return getPropertyByteSize(instId, propId);
	}
	// Return the value of a possible nested property
	virtual uint32_t getPropertyValue(void* instId, int32_t propId, uint8_t* outData, uint32_t outDataLen) = 0;
	// Get the actual raw database memory.  This is subject to change drastically if the object gets deleted.
	virtual DataRef<uint8_t> getRawPropertyValue(void* instId, int32_t propId) = 0;

	DataRef<uint8_t> getRawPropertyValue(void* instId, const char* propName)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return 0;
		}
		return getRawPropertyValue(instId, descOpt->mPropertyId);
	}

	template <typename TDataType>
	DataRef<TDataType> getTypedRawPropertyValue(void* instId, int32_t propId)
	{
		DataRef<uint8_t> propVal = getRawPropertyValue(instId, propId);
		return DataRef<TDataType>(reinterpret_cast<const TDataType*>(propVal.begin()),
		                          propVal.size() / sizeof(TDataType));
	}

	template <typename TDataType>
	DataRef<TDataType> getTypedRawPropertyValue(void* instId, const char* propName)
	{
		DataRef<uint8_t> propVal = getRawPropertyValue(instId, propName);
		return DataRef<TDataType>(reinterpret_cast<const TDataType*>(propVal.begin()),
		                          propVal.size() / sizeof(TDataType));
	}

	template <typename TDataType>
	uint32_t getPropertyValue(void* instId, const char* propName, TDataType* outBuffer, uint32_t outNumBufferItems)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return 0;
		}
		const PropertyDescription& prop(descOpt);
		uint32_t desired = outNumBufferItems * sizeof(TDataType);
		return getPropertyValue(instId, prop.mPropertyId, reinterpret_cast<uint8_t*>(outBuffer), desired) /
		       sizeof(TDataType);
	}

	template <typename TDataType>
	Option<TDataType> getPropertyValue(void* instId, const char* propName)
	{
		TDataType retval;
		if(getPropertyValue(instId, propName, &retval, 1) == 1)
			return retval;
		return None();
	}

	// Get this one item out of the array
	// return array[idx]
	virtual uint32_t getPropertyValue(void* instId, int32_t propId, int inArrayIndex, uint8_t* outData,
	                                  uint32_t outDataLen) = 0;
	// Get this sub element of one item out of the array
	// return array[idx].a
	virtual uint32_t getPropertyValue(void* instId, int32_t propId, int inArrayIndex, int nestedProperty,
	                                  uint8_t* outData, uint32_t outDataLen) = 0;

	// Get a set of properties defined by a property message
	virtual bool getPropertyMessage(void* instId, int32_t msgId, uint8_t* data, uint32_t dataLen) const = 0;

	template <typename TDataType>
	bool getPropertyMessage(void* instId, TDataType& msg)
	{
		Option<PropertyMessageDescription> msgId(
		    getMetaData().findPropertyMessage(getPvdNamespacedNameForType<TDataType>()));
		if(msgId.hasValue() == false)
			return false;
		return getPropertyMessage(instId, msgId.getValue().mMessageId, reinterpret_cast<uint8_t*>(&msg), sizeof(msg));
	}

	// clearing the array is performed with a set property value call with no data.
	virtual uint32_t getNbArrayElements(void* instId, int32_t propId) = 0;
	uint32_t getNbArrayElements(void* instId, const char* propName)
	{
		ClassDescription cls(getClassOf(instId));
		Option<PropertyDescription> descOpt(getMetaData().findProperty(cls.mClassId, propName));
		if(!descOpt.hasValue())
		{
			PX_ASSERT(false);
			return false;
		}
		const PropertyDescription& prop(descOpt);
		return getNbArrayElements(instId, prop.mPropertyId);
	}

	// Write this instance out.  Offset is set as the instances last write offset.
	// This offset is cleared if the object is changed.
	// If offset doesn't have a value, then the instance isn't changed.
	virtual void writeInstance(void* instId, PvdOutputStream& stream) = 0;

	virtual uint32_t getNbInstances() const = 0;
	virtual uint32_t getInstances(InstanceDescription* outBuffer, uint32_t count, uint32_t startIndex = 0) const = 0;

	// Get the list of updated objects since the last time someone cleared the updated instance list.
	virtual uint32_t getNbUpdatedInstances() const = 0;
	virtual uint32_t getUpdatedInstances(InstanceDescription* outBuffer, uint32_t count, uint32_t startIndex = 0) = 0;
	// Must be called for instances to be released.  Only instances that aren't live nor are they updated
	// are valid.
	virtual void clearUpdatedInstances() = 0;
};

class PvdObjectModel : public PvdObjectModelMutator, public PvdObjectModelReader
{
  protected:
	virtual ~PvdObjectModel()
	{
	}

  public:
	virtual void destroyAllInstances() = 0;
	virtual bool setPropertyValueToDefault(void* instId, int32_t propId) = 0;
	// Read an instance data and put a copy of the data in the output stream.
	static bool readInstance(PvdInputStream& inStream, PvdOutputStream& outStream);
	virtual InstanceDescription readInstance(DataRef<const uint8_t> writtenData) = 0;
	// Set just this property from this serialized instance.
	// Expects the instance to be alive, just like setPropertyValue
	virtual bool readInstanceProperty(DataRef<const uint8_t> writtenData, int32_t propId) = 0;

	virtual void recordCompletedInstances() = 0;

	// OriginShift seekback support
	virtual uint32_t getNbShifted() = 0;
	virtual void getShiftedPair(InstancePropertyPair* outData, uint32_t count) = 0;
	virtual void clearShiftedPair() = 0;
	virtual void shiftObject(void* instId, int32_t propId, PxVec3 shift) = 0;
	static PvdObjectModel& create(physx::PxAllocatorCallback& callback, PvdObjectModelMetaData& metaData,
	                              bool isCapture = false);
};

#if PX_VC == 11 || PX_VC == 12 || PX_VC == 14
#pragma warning(pop)
#endif
}
}
#endif // PXPVDSDK_PXPVDOBJECTMODEL_H
