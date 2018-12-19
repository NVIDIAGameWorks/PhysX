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



#include "ApexPvdClient.h"

#ifndef WITHOUT_PVD

#include "Module.h"
#include "ApexSDKIntl.h"
#include "ModuleIntl.h"

#include "PxPvd.h"
#include "PxPvdUserRenderer.h"
#include "PxPvdDataStream.h"
#include "PxPvdTransport.h"
#include "PxPvdObjectModelBaseTypes.h"

#include "PxAllocatorCallback.h"
#include "PsUserAllocated.h"

#include "ModuleFrameworkRegistration.h"

#include "PVDParameterizedHandler.h"

#define INIT_PVD_CLASSES_PARAMETERIZED( parameterizedClassName ) { \
	pvdStream.createClass(NamespacedName(APEX_PVD_NAMESPACE, #parameterizedClassName)); \
	parameterizedClassName* params = DYNAMIC_CAST(parameterizedClassName*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(#parameterizedClassName)); \
	mParameterizedHandler->initPvdClasses(*params->rootParameterDefinition(), #parameterizedClassName); \
	params->destroy(); }

// NOTE: the PvdDataStream is not threadsafe.

using namespace nvidia;
using namespace nvidia::shdfnd;
using namespace physx::pvdsdk;

struct SimpleAllocator : public PxAllocatorCallback 
{
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		PX_UNUSED(filename);
		PX_UNUSED(line);
		PX_UNUSED(typeName);
		
		// Ensure that we don't use a tracking allocation so that we don't get infinite recursion
		return physx::shdfnd::getAllocator().allocate(size, typeName, filename, line);
	}

	virtual void deallocate(void* ptr)
	{
		physx::shdfnd::getAllocator().deallocate(ptr);
	}
};
static SimpleAllocator sAllocator;

class SceneRendererClient : public RendererEventClient, public physx::shdfnd::UserAllocated
{
	PX_NOCOPY(SceneRendererClient)
public:
	SceneRendererClient(PvdUserRenderer* renderer, PxPvd* pvd) :mRenderer(renderer)
	{
		mStream = PvdDataStream::create(pvd);
		mStream->createInstance(renderer);
	}

	~SceneRendererClient()
	{
		mStream->destroyInstance(mRenderer);
		mStream->release();
	}

	virtual void handleBufferFlush(const uint8_t* inData, uint32_t inLength)
	{
		mStream->setPropertyValue(mRenderer, "events", inData, inLength);
	}

private:

	PvdUserRenderer* mRenderer;
	PvdDataStream* mStream;
};

class ApexPvdClientImpl : public UserAllocated, public ApexPvdClient, public PxAllocatorCallback
{
	PX_NOCOPY(ApexPvdClientImpl)

	PsPvd*						mPvd;
	bool						mIsConnected;
	Array<void*>				mInstances;
	PvdDataStream*				mDataStream;
	PvdUserRenderer*			mRenderer;
	RendererEventClient*		mRenderClient;
	PvdParameterizedHandler*	mParameterizedHandler;
	
public:
	static const char* getApexPvdClientNamespace() { return "PVD.ApexPvdClient"; }
	static const NamespacedName getApexPvdClientNamespacedName()
	{
		static NamespacedName instanceNamespace(APEX_PVD_NAMESPACE, "scene");
		return instanceNamespace;
	}

	ApexPvdClientImpl( PxPvd* inPvd )
		: mDataStream( NULL )
		, mRenderer( NULL )
		, mRenderClient(NULL)
		, mParameterizedHandler( NULL )
		, mIsConnected(false)
	{
		mPvd = static_cast<PsPvd*>(inPvd);
		if (mPvd)
		{
			mPvd->addClient(this);
		}
	}

	virtual ~ApexPvdClientImpl()
	{
		if (mPvd)
		{
			mPvd->removeClient(this);
		}
	}

	virtual PxPvd& getPxPvd()
	{
		return *mPvd;
	}
	
	virtual bool isConnected() const
	{
		return mIsConnected;
	}

	virtual void onPvdConnected()
	{
		if(mIsConnected || !mPvd)
			return;

		mIsConnected = true;	
		mDataStream = PvdDataStream::create(mPvd); 	
		mRenderer = PvdUserRenderer::create();
		mRenderClient = PX_NEW(SceneRendererClient)(mRenderer, mPvd);
		mRenderer->setClient(mRenderClient);

		mParameterizedHandler = PX_NEW(PvdParameterizedHandler)(*mDataStream);

		if (mPvd->getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
		{
			initPvdClasses();
			initPvdInstances();
		}

		//Setting the namespace ensure that our class definition *can't* collide with
		//someone else's.  It doesn't protect against instance ids colliding which is why
		//people normally use memory addresses casted to unsigned 64 bit numbers for those.

		//mDataStream->setNamespace( getApexPvdClientNamespace() );
		//mDataStream->createClass( "ApexPvdClient", 1 );
		//mDataStream->defineProperty( 1, "Frame", "", PvdCommLayerDatatype::Section, 1 );
		mDataStream->createClass( getApexPvdClientNamespacedName() );
	}

	virtual void onPvdDisconnected()
	{
		if(!mIsConnected)
			return;
		mIsConnected = false;

		if ( mParameterizedHandler != NULL )
		{
			PX_DELETE(mParameterizedHandler);
			mParameterizedHandler = NULL;
		}

		if ( mDataStream != NULL )
		{
			mDataStream->release();
			mDataStream = NULL;
		}

		if (mRenderClient != NULL)
		{
			PX_DELETE(mRenderClient);
			mRenderClient = NULL;
		}

		if ( mRenderer != NULL )
		{
			mRenderer->release();
			mRenderer = NULL;
		}

		mInstances.clear();
	}

	virtual void flush()
	{
	}

	void* ensureInstance( void* inInstance )
	{
		uint32_t instCount = mInstances.size();
		for ( uint32_t idx = 0; idx < instCount; ++idx )
		{
			if ( mInstances[idx] == inInstance )
				return inInstance;
		}
		if ( mDataStream )
		{
			mDataStream->createInstance(getApexPvdClientNamespacedName(), inInstance);
		}
		mInstances.pushBack( inInstance );
		return inInstance;
	}

	virtual void beginFrame( void* inInstance )
	{
		if ( mDataStream == NULL ) return;
		mDataStream->beginSection( ensureInstance( inInstance ), "ApexFrame");
	}

	virtual void endFrame( void* inInstance )
	{
		if ( mDataStream == NULL ) return;
		//Flush the outstanding memory events.  PVD in some situations tracks memory events
		//and can display graphs of memory usage at certain points.  They have to get flushed
		//at some point...
		
		//getConnectionManager().flushMemoryEvents();
		
		//Also note that PVD is a consumer of the profiling system events.  This ensures
		//that PVD gets a view of the profiling events that pertained to the last frame.
		

		//mDataStream->setPropertyValue( ensureInstance( inInstance ), 1, createSection( SectionType::End ) );
		//PxPvdTransport* transport = mPvd->getTransport();
		//if ( transport ) 
		//{
			//Flushes memory and profiling events out of any buffered areas.
		//	transport->flush();
		//}
		mPvd->flush();

		mDataStream->endSection( ensureInstance( inInstance ), "ApexFrame");

		//flush our data to the main connection
		//and flush the main connection.
		//This could be an expensive call.
		//mDataStream->flush();
		mRenderer->flushRenderEvents();
	}

		
	//destroy this instance;
	virtual void release() 
	{ 
		PX_DELETE( this ); 
	}

	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		PX_UNUSED(filename);
		PX_UNUSED(line);
		PX_UNUSED(typeName);
		return PX_ALLOC(size, typeName);
	}

	virtual void deallocate(void* ptr)
	{
		PX_FREE(ptr);
	}

	virtual PvdDataStream* getDataStream()
	{
		//PX_ASSERT(mIsLocked);
		return mDataStream;
	}

	virtual PvdUserRenderer* getUserRender()
	{
		//PX_ASSERT(mIsLocked);
		return mRenderer;
	}

	virtual void initPvdClasses()
	{
		//PX_ASSERT(mIsLocked);

		// ApexSDK
		NamespacedName apexSdkName = NamespacedName(APEX_PVD_NAMESPACE, "ApexSDK");
		mDataStream->createClass(apexSdkName);
		mDataStream->createProperty(apexSdkName, "Platform", "", getPvdNamespacedNameForType<const char*>(), PropertyType::Scalar);
		mDataStream->createProperty(apexSdkName, "Modules", "", getPvdNamespacedNameForType<ObjectRef>(), PropertyType::Array);

		// init framework parameterized classes
		PvdDataStream& pvdStream = *mDataStream;
		INIT_PVD_CLASSES_PARAMETERIZED(VertexFormatParameters);
		INIT_PVD_CLASSES_PARAMETERIZED(VertexBufferParameters);
		INIT_PVD_CLASSES_PARAMETERIZED(SurfaceBufferParameters);
		INIT_PVD_CLASSES_PARAMETERIZED(SubmeshParameters);
		INIT_PVD_CLASSES_PARAMETERIZED(RenderMeshAssetParameters);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU8x1);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU8x2);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU8x3);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU8x4);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU16x1);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU16x2);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU16x3);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU16x4);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU32x1);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU32x2);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU32x3);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferU32x4);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferF32x1);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferF32x2);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferF32x3);
		INIT_PVD_CLASSES_PARAMETERIZED(BufferF32x4);

		// module classes
		ApexSDKIntl* niApexSDK = GetInternalApexSDK();
		uint32_t numModules = niApexSDK->getNbModules();
		for (uint32_t i = 0; i < numModules; ++i)
		{
			ModuleIntl* niModule = niApexSDK->getInternalModules()[i];
			Module* nxModule = niApexSDK->getModules()[i];

			NamespacedName moduleName;
			moduleName.mNamespace = APEX_PVD_NAMESPACE;
			moduleName.mName = nxModule->getName();
			mDataStream->createClass(moduleName);

			niModule->initPvdClasses(*mDataStream);
		}
	}

	virtual void initPvdInstances()
	{
		//PX_ASSERT(mIsLocked);

		ApexSDK* apexSdk = GetApexSDK();
		mDataStream->createInstance( NamespacedName(APEX_PVD_NAMESPACE, "ApexSDK"), apexSdk);

		// set platform name
		NvParameterized::SerializePlatform platform;
		apexSdk->getCurrentPlatform(platform);
		const char* platformName = apexSdk->getPlatformName(platform);
		mDataStream->setPropertyValue(apexSdk, "Platform", platformName);

		mDataStream->setIsTopLevelUIElement(apexSdk, true);

		// add module instances
		uint32_t numModules = apexSdk->getNbModules();
		ApexSDKIntl* niApexSDK = GetInternalApexSDK();
		for (uint32_t i = 0; i < numModules; ++i)
		{
			// init pvd instances
			Module* nxModule = apexSdk->getModules()[i];
			ModuleIntl* niModule = niApexSDK->getInternalModules()[i];

			NamespacedName moduleName;
			moduleName.mNamespace = APEX_PVD_NAMESPACE;
			moduleName.mName = nxModule->getName();
			mDataStream->createInstance(moduleName, nxModule);
			mDataStream->pushBackObjectRef(apexSdk, "Modules", nxModule);

			niModule->initPvdInstances(*mDataStream);
		}
	}


	virtual void initPvdClasses(const NvParameterized::Definition& paramsHandle, const char* pvdClassName)
	{
		//PX_ASSERT(mIsLocked);

		if (mParameterizedHandler != NULL)
		{
			mParameterizedHandler->initPvdClasses(paramsHandle, pvdClassName);
		}
	}


	virtual void updatePvd(const void* pvdInstance, NvParameterized::Interface& params, PvdAction::Enum pvdAction)
	{
		//PX_ASSERT(mIsLocked);

		if (mParameterizedHandler != NULL)
		{
			NvParameterized::Handle paramsHandle(params);
			mParameterizedHandler->updatePvd(pvdInstance, paramsHandle, pvdAction);
		}
	}
};


ApexPvdClient* ApexPvdClient::create( PxPvd* inPvd )
{
	return PX_NEW( ApexPvdClientImpl) ( inPvd );
}

#else

physx::pvdsdk::ApexPvdClient* physx::pvdsdk::ApexPvdClient::create( physx::PxPvd& )
{
	return NULL;
}

#endif // WITHOUT_PVD