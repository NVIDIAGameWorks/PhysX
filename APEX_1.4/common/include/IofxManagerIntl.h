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



#ifndef IOFX_MANAGER_INTL_H
#define IOFX_MANAGER_INTL_H

#include "PsArray.h"
#include "PxVec3.h"
#include "PxVec4.h"
#include "PxTaskManager.h"

namespace physx
{
	class PxGpuCopyDescQueue;
}

namespace nvidia
{
namespace apex
{
class IofxAsset;
class RenderVolume;

template <class T>
class ApexMirroredArray;


class IofxManagerDescIntl
{
public:
	IofxManagerDescIntl() :
		iosAssetName(NULL),
		iosOutputsOnDevice(false),
		iosSupportsDensity(false),
		iosSupportsCollision(false),
		iosSupportsUserData(false),
		maxObjectCount(0),
		maxInputCount(0),
		maxInStateCount(0)
	{
	}

	const char* iosAssetName;
	bool		iosOutputsOnDevice;
	bool		iosSupportsDensity;
	bool		iosSupportsCollision;
	bool		iosSupportsUserData;
	uint32_t		maxObjectCount;
	uint32_t		maxInputCount;
	uint32_t		maxInStateCount;
};

/// The IOFX will update the volumeID each simulation step, the IOS must
/// persist this output. IOS provides initial volumeID, based on emitter's
/// preferred volume.
struct IofxActorIDIntl
{
	uint32_t		value;

	PX_CUDA_CALLABLE PX_INLINE IofxActorIDIntl() {}
	PX_CUDA_CALLABLE PX_INLINE explicit IofxActorIDIntl(uint32_t arg)
	{
		value = arg;
	}

	PX_CUDA_CALLABLE PX_INLINE void set(uint16_t volumeID, uint16_t actorClassID)
	{
		value = (uint32_t(volumeID) << 16) | uint32_t(actorClassID);
	}

	PX_CUDA_CALLABLE PX_INLINE uint16_t getVolumeID() const
	{
		return uint16_t(value >> 16);
	}
	PX_CUDA_CALLABLE PX_INLINE void setVolumeID(uint16_t volumeID)
	{
		value &= 0x0000FFFFu;
		value |= (uint32_t(volumeID) << 16);
	}

	PX_CUDA_CALLABLE PX_INLINE uint16_t getActorClassID() const
	{
		return uint16_t(value & 0xFFFFu);
	}
	PX_CUDA_CALLABLE PX_INLINE void setActorClassID(uint16_t actorClassID)
	{
		value &= 0xFFFF0000u;
		value |= uint32_t(actorClassID);
	}

	static const uint16_t NO_VOLUME = 0xFFFFu;
	static const uint16_t IPX_ACTOR = 0xFFFFu;
};


/* IOFX Manager returned pointers for simulation data */
class IosBufferDescIntl
{
public:
	/* All arrays are indexed by input ID */
	ApexMirroredArray<PxVec4>*			pmaPositionMass;
	ApexMirroredArray<PxVec4>*			pmaVelocityLife;
	ApexMirroredArray<PxVec4>*			pmaCollisionNormalFlags;
	ApexMirroredArray<float>*			pmaDensity;
	ApexMirroredArray<IofxActorIDIntl>*	pmaActorIdentifiers;
	ApexMirroredArray<uint32_t>*			pmaInStateToInput;
	ApexMirroredArray<uint32_t>*			pmaOutStateToInput;

	ApexMirroredArray<uint32_t>*			pmaUserData;

	//< Value in inStateToInput field indicates a dead particle, input to IOFX
	static const uint32_t NOT_A_PARTICLE = 0xFFFFFFFFu;

	//< Flag in inStateToInput field indicates a new particle, input to IOFX
	static const uint32_t NEW_PARTICLE_FLAG = 0x80000000u;
};

// This is a representative of uint4 on host
struct IofxSlice
{
	uint32_t x, y, z, w;
};

typedef void (*EventCallback)(void*);

class IofxManagerCallbackIntl
{
public:
	virtual void operator()(void* stream = NULL) = 0;
};

class IofxManagerClientIntl
{
public:
	struct Params
	{
		float objectScale;

		Params()
		{
			setDefaults();
		}

		void setDefaults()
		{
			objectScale = 1.0f;
		}
	};
	virtual void getParams(IofxManagerClientIntl::Params& params) const = 0;
	virtual void setParams(const IofxManagerClientIntl::Params& params) = 0;
};


class IofxManagerIntl
{
public:
	//! An IOS Actor will call this once, at creation
	virtual void createSimulationBuffers(IosBufferDescIntl& outDesc) = 0;

	//! An IOS actor will call this once, when it creates its fluid simulation
	virtual void setSimulationParameters(float radius, const PxVec3& up, float gravity, float restDensity) = 0;

	//! An IOS Actor will call this method after each simulation step
	virtual void updateEffectsData(float deltaTime, uint32_t numObjects, uint32_t maxInputID, uint32_t maxStateID, void* extraData = 0) = 0;

	//! An IOS Actor will call this method at the start of each step IOFX will run
	virtual PxTaskID getUpdateEffectsTaskID(PxTaskID) = 0;

	virtual uint16_t getActorClassID(IofxManagerClientIntl* client, uint16_t meshID) = 0;

	virtual IofxManagerClientIntl* createClient(nvidia::apex::IofxAsset* asset, const IofxManagerClientIntl::Params& params) = 0;
	virtual void releaseClient(IofxManagerClientIntl* client) = 0;

	virtual uint16_t getVolumeID(nvidia::apex::RenderVolume* vol) = 0;

	//! Triggers the IOFX Manager to copy host buffers to the device
	//! This is intended for use in an IOS post-update task, if they
	//! need the output buffers on the device.
	virtual void outputHostToDevice(PxGpuCopyDescQueue& copyQueue) = 0;
	
	//! IofxManagerCallbackIntl will be called before Iofx computations
	virtual void setOnStartCallback(IofxManagerCallbackIntl*) = 0;
	//! IofxManagerCallbackIntl will be called after Iofx computations
	virtual void setOnFinishCallback(IofxManagerCallbackIntl*) = 0;

	//! Called when IOS is being deleted
	virtual void release() = 0;

	//get bounding box
	virtual PxBounds3 getBounds() const = 0;

protected:
	virtual ~IofxManagerIntl() {}
};


}
} // end namespace nvidia::apex

#endif // IOFX_MANAGER_INTL_H
