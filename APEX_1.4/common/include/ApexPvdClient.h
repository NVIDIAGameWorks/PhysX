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



#ifndef APEX_PVD_CLIENT_H
#define APEX_PVD_CLIENT_H

#include "Px.h"
#include "ApexDefs.h"

#define APEX_PVD_NAMESPACE "Apex"

//#define WITHOUT_PVD 1
#ifdef WITHOUT_PVD
namespace physx
{
	class PxPvd;
	namespace pvdsdk
	{
		class PvdDataStream;
		class PvdUserRenderer;
	}
}
#else
#include "PsPvd.h"
#include "PxPvdClient.h"
#include "PxPvdObjectModelBaseTypes.h"
#include "PxPvdDataStream.h"
#include "PxPvdUserRenderer.h"
#endif

namespace NvParameterized
{
	class Interface;
	class Definition;
	class Handle;
}

namespace physx
{
namespace pvdsdk
{
	/**
	\brief Define what action needs to be done when updating pvd with an NvParameterized object.
	*/
	struct PvdAction
	{
		/**
		\brief Enum
		*/
		enum Enum
		{
			/**
			\brief Create instances and update properties.
			*/
			UPDATE,

			/**
			\brief Destroy instances.
			*/
			DESTROY
		};
	};



	/**
	\brief The ApexPvdClient class allows APEX and PhysX to both connect to the PhysX Visual Debugger (PVD)
	*/
	class ApexPvdClient : public PvdClient
	{
	public:
		/**
		\brief Check if the PVD connection is active
		*/
		virtual bool isConnected() const = 0;

		/**
		\brief Called when PVD connection established
		*/
		virtual void onPvdConnected() = 0;

		/**
		\brief Called when PVD connection finished
		*/
		virtual void onPvdDisconnected() = 0;

		/**
		\brief Flush data streams etc.
		*/
		virtual void flush() = 0;

		/**
		\brief Retrieve the PxPvd
		*/
		virtual PxPvd& getPxPvd() = 0;

		/**
		\brief Returns the data stream if Pvd is connected.
		*/
		virtual PvdDataStream* getDataStream() = 0;

		/**
		\brief Returns the PvdUserRenderer if Pvd is connected.
		*/
		virtual PvdUserRenderer* getUserRender() = 0;

		//virtial PvdMetaDataBinding* getMetaDataBinding() = 0;

		/**
		\brief Initializes the classes sent to pvd.
		*/
		virtual void initPvdClasses() = 0;

		/**
		\brief Sends the existing instances to pvd.
		*/
		virtual void initPvdInstances() = 0;

		/**
		\brief Adds properties of an NvParameterized object to the provided class and creates necessary subclasses for structs.

		\note The pvd class pvdClassName must already exist. Pvd classes for structs are being created, but not for references.
		*/
		virtual void initPvdClasses(const NvParameterized::Definition& paramsHandle, const char* pvdClassName) = 0;

		/**
		\brief Creates or destroys pvdInstances and/or updates properties.
		*/
		virtual void updatePvd(const void* pvdInstance, NvParameterized::Interface& params, PvdAction::Enum pvdAction = PvdAction::UPDATE) = 0;

		//////////////////

		/**
		\brief Start the profiling frame
		\note inInstanceId must *not* be used already by pvd
		*/
		virtual void beginFrame( void* inInstanceId ) = 0;

		/**
		\brief End the profiling frame
		*/
		virtual void endFrame( void* inInstanceId ) = 0;

		/**
		\brief Destroy this instance
		*/
		virtual void release() = 0;

		/**
		 *	Assumes foundation is already booted up.
		 */
		static ApexPvdClient* create( PxPvd* pvd );
	};

}
}



#endif // APEX_PVD_CLIENT_H
