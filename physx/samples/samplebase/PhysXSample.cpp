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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.

#include "PxPhysXConfig.h"
#include "foundation/PxMemory.h"
#include "SamplePreprocessor.h"
#include "PhysXSampleApplication.h"
#include "PhysXSample.h"
#include "SampleCommandLine.h"

#include "PxTkFile.h"
#include "PsString.h"
#include "PsFPU.h"

#include "PxToolkit.h"
#include "extensions/PxDefaultStreams.h"

#include "RenderBoxActor.h"
#include "RenderSphereActor.h"
#include "RenderCapsuleActor.h"
#include "RenderMeshActor.h"
#include "RenderGridActor.h"
#include "RenderMaterial.h"
#include "RenderTexture.h"
#include "RenderPhysX3Debug.h"

#include <SamplePlatform.h>
#include "SampleBaseInputEventIds.h"
#include <SampleUserInputIds.h>
#include "SampleUserInputDefines.h"
#include <SampleInputAsset.h>
#include "SampleInputMappingAsset.h"

#include <algorithm>
#include <ctype.h>

#include "Picking.h"
#include "TestGroup.h"

#if PX_WINDOWS
// Starting with Release 302 drivers, application developers can direct the Optimus driver at runtime to use the High Performance
// Graphics to render any application; even those applications for which there is no existing application profile.
// They can do this by exporting a global variable named PxOptimusEnablement.
// A value of 0x00000001 indicates that rendering should be performed using High Performance Graphics.
// A value of 0x00000000 indicates that this method should be ignored.
extern "C"
{
	_declspec(dllexport) DWORD PxOptimusEnablement = 0x00000001;
}
#endif

using namespace SampleFramework;
using namespace SampleRenderer;

static bool gRecook	= false;
PxDefaultAllocator gDefaultAllocatorCallback;

enum MaterialID
{
	MATERIAL_CLOTH	 = 444,
};

///////////////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE PxSimulationFilterShader getSampleFilterShader()
{
	return PxDefaultSimulationFilterShader;
}

///////////////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE void SetupDefaultRigidDynamic(PxRigidDynamic& body, bool kinematic=false)
{
	body.setActorFlag(PxActorFlag::eVISUALIZATION, true);
	body.setAngularDamping(0.5f);
	body.setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

void PhysXSample::unlink(RenderBaseActor* renderActor, PxShape* shape, PxRigidActor* actor)
{
	PhysXShape theShape(actor, shape);
	mPhysXShapeToRenderActorMap.erase(theShape);

	PX_UNUSED(renderActor);
}

void PhysXSample::link(RenderBaseActor* renderActor, PxShape* shape, PxRigidActor* actor)
{
	PhysXShape theShape(actor, shape);
	mPhysXShapeToRenderActorMap[theShape] = renderActor;

	renderActor->setPhysicsShape(shape, actor);
}

RenderBaseActor* PhysXSample::getRenderActor(PxRigidActor* actor, PxShape* shape)
{
	PhysXShape theShape(actor, shape);
	PhysXShapeToRenderActorMap::iterator it = mPhysXShapeToRenderActorMap.find(theShape);
	if (mPhysXShapeToRenderActorMap.end() != it)
		return it->second;
	return NULL;
}

SampleDirManager& PhysXSample::getSampleOutputDirManager()
{
	static SampleDirManager gSampleOutputDirManager(SAMPLE_OUTPUT_PATH, false);
	return gSampleOutputDirManager;
}

PxToolkit::BasicRandom& getSampleRandom()
{
	static PxToolkit::BasicRandom gRandom(42);
	return gRandom;
}

PxErrorCallback& getSampleErrorCallback()
{
	static PxDefaultErrorCallback gDefaultErrorCallback;
	return gDefaultErrorCallback;
}

// sschirm: same here: would be good to have a place for platform independent path manipulation
// shared for all apps
const char* getFilenameFromPath(const char* filePath, char* buffer)
{
	const char* ptr = strrchr(filePath, '/');
	if (!ptr)
		ptr = strrchr(filePath, '\\');
	
	if (ptr)
	{
		strcpy(buffer, ptr + 1);
	}
	else 
	{
		strcpy(buffer, filePath);
	}
	return buffer;
}

const char* PhysXSample::getSampleOutputFilePath(const char* inFilePath, const char* outExtension)
{
	static char sBuffer[1024];
	char tmpBuffer[1024];
	
	const char* inFilename = getFilenameFromPath(inFilePath, sBuffer);
	sprintf(tmpBuffer, "cached/%s%s", inFilename, outExtension);
	return getSampleOutputDirManager().getFilePath(tmpBuffer, sBuffer, false);	
}

static void GenerateCirclePts(unsigned int nb, PxVec3* pts, float scale, float z)
{
	for(unsigned int i=0;i<nb;i++)
	{
		const PxF32 angle = 6.28f*PxF32(i)/PxF32(nb);
		pts[i].x = cosf(angle)*scale;
		pts[i].y = z;
		pts[i].z = sinf(angle)*scale;
	}
}

static PxConvexMesh* GenerateConvex(PxPhysics& sdk, PxCooking& cooking, PxU32 nbVerts, const PxVec3* verts, bool recenterVerts=false)
{
	PxVec3Alloc* tmp = NULL;
	if(recenterVerts)
	{
		PxVec3 center(0);
		for(PxU32 i=0;i<nbVerts;i++)
			center += verts[i];
		center /= PxReal(nbVerts);

		tmp = SAMPLE_NEW(PxVec3Alloc)[nbVerts];
		PxVec3* recentered = tmp;
		for(PxU32 i=0;i<nbVerts;i++)
			recentered[i] = verts[i] - center;
	}

	PxConvexMesh* convexMesh = PxToolkit::createConvexMesh(sdk, cooking, recenterVerts ? tmp : verts, nbVerts, PxConvexFlag::eCOMPUTE_CONVEX);

	DELETEARRAY(tmp);

	return convexMesh;
}

static PxConvexMesh* GenerateConvex(PxPhysics& sdk, PxCooking& cooking, float scale, bool large=false, bool randomize=true)
{
	const PxI32 minNb = large ? 16 : 3;
	const PxI32 maxNb = large ? 32 : 8;
	const int nbInsideCirclePts = !randomize ? 3 : getSampleRandom().rand(minNb, maxNb);
	const int nbOutsideCirclePts = !randomize ? 8 : getSampleRandom().rand(minNb, maxNb);
	const int nbVerts = nbInsideCirclePts + nbOutsideCirclePts;
	
	// Generate random vertices
	PxVec3Alloc* verts = SAMPLE_NEW(PxVec3Alloc)[nbVerts];
	
	// Two methods
	if(randomize && getSampleRandom().rand(0, 100) > 50)
	{
		for(int i=0;i<nbVerts;i++)
		{
			verts[i].x = scale * getSampleRandom().rand(-2.5f, 2.5f);
			verts[i].y = scale * getSampleRandom().rand(-2.5f, 2.5f);
			verts[i].z = scale * getSampleRandom().rand(-2.5f, 2.5f);
		}
	}
	else
	{
		GenerateCirclePts(nbInsideCirclePts, verts, scale, 0.0f);
		GenerateCirclePts(nbOutsideCirclePts, verts+nbInsideCirclePts, scale*3.0f, 10.0f*scale);
	}

	PxConvexMesh* convexMesh = GenerateConvex(sdk, cooking, nbVerts, verts);

	DELETEARRAY(verts);
	return convexMesh;
}

#if 0

static PxConvexMesh* GenerateConvex(PxPhysics& sdk, PxCooking& cooking, int nbInsideCirclePts, int nbOutsideCirclePts, float scale0, float scale1, float z)
{
	const int nbVerts = nbInsideCirclePts + nbOutsideCirclePts;
	
	// Generate random vertices
	PxVec3Alloc* verts = SAMPLE_NEW(PxVec3Alloc)[nbVerts];
	
	GenerateCirclePts(nbInsideCirclePts, verts, scale0, 0.0f);
	GenerateCirclePts(nbOutsideCirclePts, verts+nbInsideCirclePts, scale1, z);

	PxConvexMesh* convexMesh = GenerateConvex(sdk, cooking, nbVerts, verts);

	DELETEARRAY(verts);
	return convexMesh;
}

#endif

static PxRigidDynamic* GenerateCompound(PxPhysics& sdk, PxScene* scene, PxMaterial* defaultMat, const PxVec3& pos, const PxQuat& rot, const std::vector<PxTransform>& poses, const std::vector<const PxGeometry*>& geometries, bool kinematic=false, PxReal density = 1.0f)
{
	PxRigidDynamic* actor = sdk.createRigidDynamic(PxTransform(pos, rot));
	SetupDefaultRigidDynamic(*actor);

	PX_ASSERT(poses.size() == geometries.size());
	for(PxU32 i=0;i<poses.size();i++)
	{
		const PxTransform& currentPose = poses[i];
		const PxGeometry* currentGeom = geometries[i];

		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *currentGeom, *defaultMat);
		shape->setLocalPose(currentPose);
		PX_ASSERT(shape);
	}

	if(actor)
	{
		PxRigidBodyExt::updateMassAndInertia(*actor, density);
		scene->addActor(*actor);
	}
	return actor;
}

//////////////////////////////////////////////////////////////////////////

void PhysXSample::onRelease(const PxBase* observed, void* userData, PxDeletionEventFlag::Enum deletionEvent)
{
	PX_UNUSED(userData);
	PX_UNUSED(deletionEvent);

	if(observed->is<PxRigidActor>())
	{
		const PxRigidActor* actor = static_cast<const PxRigidActor*>(observed);

		removeRenderActorsFromPhysicsActor(actor);

		std::vector<PxRigidActor*>::iterator actorIter = std::find(mPhysicsActors.begin(), mPhysicsActors.end(), actor);
		if(actorIter != mPhysicsActors.end())
		{
			mPhysicsActors.erase(actorIter);
		}

	}
}

///////////////////////////////////////////////////////////////////////////////

RenderMeshActor* PhysXSample::createRenderMeshFromRawMesh(const RAWMesh& data, PxShape* shape)
{
	// Create render mesh from raw mesh
	const PxU32 nbTris = data.mNbFaces;
	const PxU32* src = data.mIndices;

	PxU16* indices = (PxU16*)SAMPLE_ALLOC(sizeof(PxU16)*nbTris*3);
	for(PxU32 i=0;i<nbTris;i++)
	{
			indices[i*3+0] = src[i*3+0];
			indices[i*3+1] = src[i*3+2];
			indices[i*3+2] = src[i*3+1];
	}

	RenderMeshActor* meshActor = SAMPLE_NEW(RenderMeshActor)(*getRenderer(), data.mVerts, data.mNbVerts, data.mVertexNormals, data.mUVs, indices, NULL, nbTris);
	SAMPLE_FREE(indices);

	if(data.mMaterialID!=0xffffffff)
	{
		size_t nbMaterials = mRenderMaterials.size();
		for(PxU32 i=0;i<nbMaterials;i++)
		{
			if(mRenderMaterials[i]->mID==data.mMaterialID)
			{
				meshActor->setRenderMaterial(mRenderMaterials[i]);
				break;
			}
		}
	}

	meshActor->setTransform(data.mTransform);
	if(data.mName)
		strcpy(meshActor->mName, data.mName);

	mRenderActors.push_back(meshActor);
	return meshActor;
}

///////////////////////////////////////////////////////////////////////////////

RenderTexture* PhysXSample::createRenderTextureFromRawTexture(const RAWTexture& data)
{
	RenderTexture* texture;

	if(!data.mPixels)
	{
		// PT: the pixel data is not included in the RAW file so we use the asset manager to load the texture
		SampleAsset* t = getAsset(getSampleMediaFilename(data.mName), SampleAsset::ASSET_TEXTURE);
		PX_ASSERT(t->getType()==SampleAsset::ASSET_TEXTURE);
		mManagedAssets.push_back(t);

		SampleTextureAsset* textureAsset = static_cast<SampleTextureAsset*>(t);
		texture = SAMPLE_NEW(RenderTexture)(*getRenderer(), data.mID, textureAsset->getTexture());

	}
	else
	{
		// PT: the pixel data is directly included in the RAW file
		texture = SAMPLE_NEW(RenderTexture)(*getRenderer(), data.mID, data.mWidth, data.mHeight, data.mPixels);
	}
	if(data.mName)
		strcpy(texture->mName, data.mName);
	mRenderTextures.push_back(texture);
	return texture;
}

///////////////////////////////////////////////////////////////////////////////

void PhysXSample::newMaterial(const RAWMaterial& data)
{
	RenderTexture* diffuse = NULL;
	if(data.mDiffuseID!=0xffffffff)
	{
		size_t nbTextures = mRenderTextures.size();
		for(PxU32 i=0;i<nbTextures;i++)
		{
			if(mRenderTextures[i]->mID==data.mDiffuseID)
			{
				diffuse = mRenderTextures[i];
				break;
			}
		}
	}

	RenderMaterial* newRM = SAMPLE_NEW(RenderMaterial)(*getRenderer(), data.mDiffuseColor, data.mOpacity, data.mDoubleSided, data.mID, diffuse);
//	strcpy(newRM->mName, data.mName);
	mRenderMaterials.push_back(newRM);
}

void PhysXSample::newMesh(const RAWMesh& data)
{
	// PT: the mesh name should capture the scale value as well, to make sure different scales give birth to different cooked files
	const PxU32 scaleTag = PX_IR(mScale);

	PX_ASSERT(mFilename);
	char extension[256];
	sprintf(extension, "_%d_%x.cooked", mMeshTag, scaleTag);
	
	const char* filePathCooked = getSampleOutputFilePath(mFilename, extension);
	PX_ASSERT(NULL != filePathCooked);

	bool ok = false;
	if(!gRecook)
	{
		SampleFramework::File* fp = NULL;
		PxToolkit::fopen_s(&fp, filePathCooked, "rb");
		if(fp)
		{
			fclose(fp);
			ok = true;
		}
	}

	if(!ok)
	{
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count		= data.mNbVerts;
		meshDesc.triangles.count	= data.mNbFaces;
		meshDesc.points.stride		= 4*3;
		meshDesc.triangles.stride	= 4*3;
		meshDesc.points.data		= data.mVerts;
		meshDesc.triangles.data		= data.mIndices;

		//
		shdfnd::printFormatted("Cooking object... %s",filePathCooked);
		PxDefaultFileOutputStream stream(filePathCooked);
		ok = mCooking->cookTriangleMesh(meshDesc, stream);
		shdfnd::printFormatted(" - Done\n");
	}

	if(ok)
	{
		PxDefaultFileInputData stream(filePathCooked);
		PxTriangleMesh* triangleMesh = mPhysics->createTriangleMesh(stream);
		if(triangleMesh)
		{
			PxTriangleMeshGeometry triGeom;
			triGeom.triangleMesh = triangleMesh;
			PxRigidStatic* actor = mPhysics->createRigidStatic(data.mTransform);
			PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, triGeom, *mMaterial);
			PX_UNUSED(shape);
			mScene->addActor(*actor);
			addPhysicsActors(actor);

			if(0)
			{
				// Create render mesh from PhysX mesh
				PxU32 nbVerts = triangleMesh->getNbVertices();
				const PxVec3* verts = triangleMesh->getVertices();
				PxU32 nbTris = triangleMesh->getNbTriangles();
				const void* tris = triangleMesh->getTriangles();
				bool s = triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
				PX_ASSERT(s);
				PX_UNUSED(s);
				const PxU16* src = (const PxU16*)tris;
				PxU16* indices = (PxU16*)SAMPLE_ALLOC(sizeof(PxU16)*nbTris*3);
				for(PxU32 i=0;i<nbTris;i++)
				{
					indices[i*3+0] = src[i*3+0];
					indices[i*3+1] = src[i*3+2];
					indices[i*3+2] = src[i*3+1];
				}

				RenderMeshActor* meshActor = SAMPLE_NEW(RenderMeshActor)(*getRenderer(), verts, nbVerts, verts, NULL, indices, NULL, nbTris);
				if(data.mName)
					strcpy(meshActor->mName, data.mName);
				PxShape* shape; 
				actor->getShapes(&shape, 1);
				link(meshActor, shape, actor);
				mRenderActors.push_back(meshActor);
				meshActor->setEnableCameraCull(true);
				SAMPLE_FREE(indices);
			}
			else
			{
				PxShape* shape; 
				actor->getShapes(&shape, 1);
				RenderMeshActor* meshActor = createRenderMeshFromRawMesh(data, shape);
				link(meshActor, shape, actor);
				meshActor->setEnableCameraCull(true);
			}

			mMeshTag++;
		}
	}
}

void PhysXSample::newShape(const RAWShape&)
{
}

void PhysXSample::newHelper(const RAWHelper&)
{
}

void PhysXSample::newTexture(const RAWTexture& data)
{
	createRenderTextureFromRawTexture(data);
}

///////////////////////////////////////////////////////////////////////////////

void PhysXSample::togglePvdConnection()
{
	if(mPvd == NULL) return;
	if (mPvd->isConnected())
		mPvd->disconnect();
	else
		mPvd->connect(*mTransport,mPvdFlags);
}

void PhysXSample::createPvdConnection()
{ 
#if PX_SUPPORT_PVD
	//Create a pvd connection that writes data straight to the filesystem.  This is
	//the fastest connection on windows for various reasons.  First, the transport is quite fast as
	//pvd writes data in blocks and filesystems work well with that abstraction.
	//Second, you don't have the PVD application parsing data and using CPU and memory bandwidth
	//while your application is running.
	//physx::PxPvdTransport* transport = physx::PxDefaultPvdFileTransportCreate( "c:\\mywork\\sample.pxd2" );
	
	//The normal way to connect to pvd.  PVD needs to be running at the time this function is called.
	//We don't worry about the return value because we are already registered as a listener for connections
	//and thus our onPvdConnected call will take care of setting up our basic connection state.
	mTransport = physx::PxDefaultPvdSocketTransportCreate(mPvdParams.ip, mPvdParams.port, mPvdParams.timeout);
	if(mTransport == NULL)
		return;

	//The connection flags state overall what data is to be sent to PVD.  Currently
	//the Debug connection flag requires support from the implementation (don't send
	//the data when debug isn't set) but the other two flags, profile and memory
	//are taken care of by the PVD SDK.

	//Use these flags for a clean profile trace with minimal overhead
	mPvdFlags = physx::PxPvdInstrumentationFlag::eALL;
	if (!mPvdParams.useFullPvdConnection )
	{
		mPvdFlags = physx::PxPvdInstrumentationFlag::ePROFILE;
	}
	
	mPvd = physx::PxCreatePvd( *mFoundation );
	mPvd->connect(*mTransport,mPvdFlags);
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
// default implemententions for PhysXSample interface
//
void PhysXSample::onPointerInputEvent(const InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool pressed)
{
	switch (ie.m_Id)
	{
	case CAMERA_MOUSE_LOOK:
		{
			if(mPicking)
				mPicking->moveCursor(x,y);
		}
		break;
	case PICKUP:
		{
			if(mPicking)
			{
				mPicked = !mPicked;;
				if(mPicked)
					mPicking->lazyPick();
				else
					mPicking->letGo();
			}
		}
		break;
	}

}

void PhysXSample::onResize(PxU32 width, PxU32 height)
{
	mApplication.baseResize(width, height);
}

PhysXSample::~PhysXSample()
{
	for (size_t i = mDeletedRenderActors.size(); i--;)
	{
		mDeletedRenderActors[i]->release();
	}
	mDeletedRenderActors.clear();
	PX_ASSERT(!mRenderActors.size());
	PX_ASSERT(!mRenderTextures.size());
	PX_ASSERT(!mRenderMaterials.size());
	PX_ASSERT(!mPhysicsActors.size());
	PX_ASSERT(!mManagedAssets.size());
	PX_ASSERT(!mBufferedActiveActors);
	PX_ASSERT(!mScene);
	PX_ASSERT(!mCpuDispatcher);
//	PX_ASSERT(!mMaterial);
#if PX_SUPPORT_GPU_PHYSX
	PX_ASSERT(!mCudaContextManager);
#endif

	PX_ASSERT(!mCooking);
	PX_ASSERT(!mPhysics);
	PX_ASSERT(!mFoundation);

	if(SCRATCH_BLOCK_SIZE)
		SAMPLE_FREE(mScratchBlock);

	delete mPicking;
}

PhysXSample::PhysXSample(PhysXSampleApplication& app, PxU32 maxSubSteps) :
	mInitialDebugRender(false),
	mCreateCudaCtxManager(true),
	mCreateGroundPlane(true),
	mStepperType(FIXED_STEPPER),
	mMaxNumSubSteps(maxSubSteps),
	mNbThreads(1),
	mDefaultDensity(20.0f),
	mDisplayFPS(true),

	mPause(app.mPause),
	mOneFrameUpdate(app.mOneFrameUpdate),
	mShowHelp(app.mShowHelp),
	mShowDescription(app.mShowDescription),
	mShowExtendedHelp(app.mShowExtendedHelp),
	mHideGraphics(false),
	mEnableAutoFlyCamera(false),
	mCameraController(app.mCameraController),
	mPvdParams(app.mPvdParams),
	mApplication(app),
	mFoundation(NULL),
	mPhysics(NULL),
	mCooking(NULL),
	mScene(NULL),
	mMaterial(NULL),
	mCpuDispatcher(NULL),
	mPvd(NULL),
	mTransport(NULL),
#if PX_SUPPORT_GPU_PHYSX
	mCudaContextManager(NULL),
#endif
	mManagedMaterials(app.mManagedMaterials),
	mSampleInputAsset(NULL),
	mBufferedActiveActors(NULL),
	mActiveTransformCount(0),
	mActiveTransformCapacity(0),
	mIsFlyCamera(false),
	mMeshTag(0),
	mFilename(NULL),
	mScale(1.0f),
	mDebugRenderScale(1.0f),
	mWaitForResults(false),
	mSavedCameraController(NULL),
	
	mDebugStepper(0.016666660f),
	mFixedStepper(0.016666660f, maxSubSteps),
	mVariableStepper(1.0f / 80.0f, 1.0f / 40.0f, maxSubSteps),
	mWireFrame(false),
	mSimulationTime(0.0f),
	mPicked(false),
	mExtendedHelpPage(0),
	mDebugObjectType(DEBUG_OBJECT_BOX)//,
{
	mDebugStepper.setSample(this);
	mFixedStepper.setSample(this);
	mVariableStepper.setSample(this);
	
	mScratchBlock = SCRATCH_BLOCK_SIZE ? SAMPLE_ALLOC(SCRATCH_BLOCK_SIZE) : 0;
	
	mFlyCameraController.init(PxVec3(0.0f, 10.0f, 0.0f), PxVec3(0.0f, 0.0f, 0.0f));

	mPicking = new physx::Picking(*this);

	mDeletedActors.clear();
}

void PhysXSample::render()
{
	PxU32 nbVisible = 0;

	if(!mHideGraphics)
	{
		PX_PROFILE_ZONE("Renderer.CullObjects", 0);
		Camera& camera = getCamera();
		Renderer* renderer = getRenderer();

		for(PxU32 i = 0; i < mRenderActors.size(); ++i)
		{
			RenderBaseActor* renderActor = mRenderActors[i];
			if(camera.mPerformVFC && 
				renderActor->getEnableCameraCull() && 
				getCamera().cull(renderActor->getWorldBounds())==PLANEAABB_EXCLUSION)
				continue;

			renderActor->render(*renderer, mManagedMaterials[MATERIAL_GREY], mWireFrame);
			++nbVisible;
		}

		//if(camera.mPerformVFC)
			//shdfnd::printFormatted("Nb visible: %d\n", nbVisible);
	}

	RenderPhysX3Debug* debugRender = getDebugRenderer();
	if(debugRender)
		debugRender->queueForRenderTriangle();
}

void PhysXSample::displayFPS()
{
	if(!mDisplayFPS)
		return;

	char fpsText[512];
	sprintf(fpsText, "%0.2f fps", mFPS.getFPS());

	Renderer* renderer = getRenderer();

	const PxU32 yInc = 18;
	renderer->print(10, getCamera().getScreenHeight() - yInc*2, fpsText);	

//	sprintf(fpsText, "%d visible objects", mNbVisible);
//	renderer->print(10, mCamera.getScreenHeight() - yInc*3, fpsText);
}

void PhysXSample::onShutdown()
{
	//mScene->fetchResults(true);
	
	releaseAll(mRenderActors);
	releaseAll(mRenderTextures);
	releaseAll(mRenderMaterials);
	{
		PxSceneWriteLock scopedLock(*mScene);
		releaseAll(mPhysicsActors);
	}
	
	SAMPLE_FREE(mBufferedActiveActors);
	mFixedStepper.shutdown();
	mDebugStepper.shutdown();
	mVariableStepper.shutdown();

	const size_t nbManagedAssets = mManagedAssets.size();
	if(nbManagedAssets)
	{
		SampleAssetManager* assetManager = mApplication.getAssetManager();
		for(PxU32 i=0; i<nbManagedAssets; i++)
			assetManager->returnAsset(*mManagedAssets[i]);
	}
	mManagedAssets.clear();

	mApplication.getPlatform()->getSampleUserInput()->shutdown();

	if(mSampleInputAsset)
	{
		mApplication.getAssetManager()->returnAsset(*mSampleInputAsset);
		mSampleInputAsset = NULL;
	}

	mPhysics->unregisterDeletionListener(*this);

	SAFE_RELEASE(mScene);
	SAFE_RELEASE(mCpuDispatcher);

//	SAFE_RELEASE(mMaterial);
	SAFE_RELEASE(mCooking);

	PxCloseExtensions();

	SAFE_RELEASE(mPhysics);	

#if PX_SUPPORT_GPU_PHYSX
	SAFE_RELEASE(mCudaContextManager);
#endif

	SAFE_RELEASE(mPvd);
	SAFE_RELEASE(mTransport);	

	SAFE_RELEASE(mFoundation);
}

//#define USE_MBP

#ifdef USE_MBP

static void setupMBP(PxScene& scene)
{
	const float range = 1000.0f;
	const PxU32 subdiv = 4;
//	const PxU32 subdiv = 1;
//	const PxU32 subdiv = 2;
//	const PxU32 subdiv = 8;

	const PxVec3 min(-range);
	const PxVec3 max(range);
	const PxBounds3 globalBounds(min, max);

	PxBounds3 bounds[256];
	const PxU32 nbRegions = PxBroadPhaseExt::createRegionsFromWorldBounds(bounds, globalBounds, subdiv);

	for(PxU32 i=0;i<nbRegions;i++)
	{
		PxBroadPhaseRegion region;
		region.bounds = bounds[i];
		region.userData = (void*)i;
		scene.addBroadPhaseRegion(region);
	}
}
#endif



void PhysXSample::onInit()
{

	//Recording memory allocations is necessary if you want to 
	//use the memory facilities in PVD effectively.  Since PVD isn't necessarily connected
	//right away, we add a mechanism that records all outstanding memory allocations and
	//forwards them to PVD when it does connect.

	//This certainly has a performance and memory profile effect and thus should be used
	//only in non-production builds.
	bool recordMemoryAllocations = true;
	const bool useCustomTrackingAllocator = true;

	PxAllocatorCallback* allocator = &gDefaultAllocatorCallback;

	if(useCustomTrackingAllocator)		
		allocator = getSampleAllocator();		//optional override that will track memory allocations

	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *allocator, getSampleErrorCallback());
	if(!mFoundation)
		fatalError("PxCreateFoundation failed!");

#if PX_SUPPORT_GPU_PHYSX
	if(mCreateCudaCtxManager)
	{
		PxCudaContextManagerDesc cudaContextManagerDesc;

#if defined(RENDERER_ENABLE_CUDA_INTEROP)
		if (!mApplication.getCommandLine().hasSwitch("nointerop"))
		{
			switch(getRenderer()->getDriverType())
			{
			case Renderer::DRIVER_DIRECT3D11:
				cudaContextManagerDesc.interopMode = PxCudaInteropMode::D3D11_INTEROP;
				break;
			case Renderer::DRIVER_OPENGL:
				cudaContextManagerDesc.interopMode = PxCudaInteropMode::OGL_INTEROP;
				break;
			}
			cudaContextManagerDesc.graphicsDevice = getRenderer()->getDevice();
		}
#endif 
		mCudaContextManager = PxCreateCudaContextManager(*mFoundation, cudaContextManagerDesc, PxGetProfilerCallback());
		if( mCudaContextManager )
		{
			if( !mCudaContextManager->contextIsValid() )
			{
				mCudaContextManager->release();
				mCudaContextManager = NULL;
			}
		}
	}
#endif

	createPvdConnection();

	PxTolerancesScale scale;
	customizeTolerances(scale);

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, scale, recordMemoryAllocations, mPvd);
	if(!mPhysics)
		fatalError("PxCreatePhysics failed!");

	if(!PxInitExtensions(*mPhysics, mPvd))
		fatalError("PxInitExtensions failed!");

	PxCookingParams params(scale);
	params.meshWeldTolerance = 0.001f;
	params.meshPreprocessParams = PxMeshPreprocessingFlags(PxMeshPreprocessingFlag::eWELD_VERTICES);
	params.buildGPUData = true; //Enable GRB data being produced in cooking.
	mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, params);
	if(!mCooking)
		fatalError("PxCreateCooking failed!");

	mPhysics->registerDeletionListener(*this, PxDeletionEventFlag::eUSER_RELEASE);

	// setup default material...
	mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.1f);
	if(!mMaterial)
		fatalError("createMaterial failed!");

	PX_ASSERT(NULL == mScene);

	PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	getDefaultSceneDesc(sceneDesc);
	
	

	if(!sceneDesc.cpuDispatcher)
	{
		mCpuDispatcher = PxDefaultCpuDispatcherCreate(mNbThreads);
		if(!mCpuDispatcher)
			fatalError("PxDefaultCpuDispatcherCreate failed!");
		sceneDesc.cpuDispatcher	= mCpuDispatcher;
	}

	if(!sceneDesc.filterShader)
		sceneDesc.filterShader	= getSampleFilterShader();

#if PX_SUPPORT_GPU_PHYSX
	if(!sceneDesc.cudaContextManager)
		sceneDesc.cudaContextManager = mCudaContextManager;
#endif

	//sceneDesc.frictionType = PxFrictionType::eTWO_DIRECTIONAL;
	//sceneDesc.frictionType = PxFrictionType::eONE_DIRECTIONAL;
	//sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	//sceneDesc.flags |= PxSceneFlag::eENABLE_AVERAGE_POINT;
	sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
	//sceneDesc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
	sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
	sceneDesc.sceneQueryUpdateMode = PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_DISABLED;

	//sceneDesc.flags |= PxSceneFlag::eDISABLE_CONTACT_CACHE;
	//sceneDesc.broadPhaseType =  PxBroadPhaseType::eGPU;
	//sceneDesc.broadPhaseType = PxBroadPhaseType::eSAP;
	sceneDesc.gpuMaxNumPartitions = 8;


	//sceneDesc.solverType = PxSolverType::eTGS;
	
#ifdef USE_MBP
	sceneDesc.broadPhaseType = PxBroadPhaseType::eMBP;
#endif

	customizeSceneDesc(sceneDesc);

	mScene = mPhysics->createScene(sceneDesc);
	if(!mScene)
		fatalError("createScene failed!");
   
	PxSceneWriteLock scopedLock(*mScene);

	PxSceneFlags flag = mScene->getFlags();

	PX_UNUSED(flag);
	mScene->setVisualizationParameter(PxVisualizationParameter::eSCALE,				mInitialDebugRender ? mDebugRenderScale : 0.0f);
	mScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES,	1.0f);

	PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

#ifdef USE_MBP
	setupMBP(*mScene);
#endif

	mApplication.refreshVisualizationMenuState(PxVisualizationParameter::eCOLLISION_SHAPES);
	mApplication.applyDefaultVisualizationSettings();
	mApplication.setMouseCursorHiding(false);
	mApplication.setMouseCursorRecentering(false);
	mCameraController.setMouseLookOnMouseButton(false);
	mCameraController.setMouseSensitivity(1.0f);

	if(mCreateGroundPlane)
		createGrid();

	LOG_INFO("PhysXSample", "Init sample ok!");
}

RenderMaterial* PhysXSample::getMaterial(PxU32 materialID)
{
	for(PxU32 i = 0; i < mRenderMaterials.size(); ++i) 
	{
		if(mRenderMaterials[i]->mID == materialID) 
		{
			return mRenderMaterials[i];
		}
	}

	return NULL;
}

Stepper* PhysXSample::getStepper()
{
	switch(mStepperType)
	{
	case DEFAULT_STEPPER:
		return &mDebugStepper;
	case FIXED_STEPPER:
		return &mFixedStepper;
	default:
		return &mVariableStepper;  
	};
}


// ### PT: TODO: refactor this with onTickPreRender
void PhysXSample::freeDeletedActors()
{
	getRenderer()->finishRendering();

	// delete buffered render actors
	for (size_t i = mDeletedRenderActors.size(); i--;)
	{
		mDeletedRenderActors[i]->release();
	}
	mDeletedRenderActors.clear();
}

void PhysXSample::onTickPreRender(float dtime)
{
	// delete buffered render actors
	for (size_t i = mDeletedRenderActors.size(); i--;)
	{
		mDeletedRenderActors[i]->release();
	}
	mDeletedRenderActors.clear();

#if PX_PROFILE
	{
#endif
		PxSceneWriteLock scopedLock(*mScene);


		mApplication.baseTickPreRender(dtime);

		mFPS.update();
#if PX_PROFILE
	}
#endif
	if(!isPaused())
	{
		Stepper* stepper = getStepper();

		mWaitForResults = false;

		if(mScene)
		{
			updateRenderObjectsAsync(dtime);

#if !PX_PROFILE
			mWaitForResults = stepper->advance(mScene, dtime, mScratchBlock, SCRATCH_BLOCK_SIZE);

			// tells the stepper shape data is not going to be accessed until next frame 
			// (frame ends with stepper->wait(mScene))
			stepper->renderDone();

#else
			// in profile builds we run the whole frame sequentially
			// simulate, wait, update render objects, render
			{
				mWaitForResults = stepper->advance(mScene, dtime, mScratchBlock, SCRATCH_BLOCK_SIZE);
				stepper->renderDone();
				if (mWaitForResults)
				{
					stepper->wait(mScene);
					mSimulationTime = stepper->getSimulationTime();
				}
			}

			// update render objects immediately
			if (mWaitForResults)
			{
				bufferActiveTransforms();
				updateRenderObjectsSync(dtime);
				if (mOneFrameUpdate)
					updateRenderObjectsAsync(dtime);
			}
#endif
		}
	}

	// profile builds should update render actors 
	// and debug draw immediately to avoid one frame lag
#if PX_PROFILE
	RenderPhysX3Debug* debugRenderer = getDebugRenderer();
	if(debugRenderer && mScene)
	{
		const PxRenderBuffer& debugRenderable = mScene->getRenderBuffer();
		debugRenderer->update(debugRenderable);

		updateRenderObjectsDebug(dtime);
	}
#endif


	if(mPicking)
	{
		mPicking->tick();
	}
}

void PhysXSample::onTickPostRender(float dtime)
{
#if !PX_PROFILE

	if(!isPaused())
	{
		if(mScene && mWaitForResults)
		{
			Stepper* stepper = getStepper();
			stepper->wait(mScene);
			mSimulationTime = stepper->getSimulationTime();

			bufferActiveTransforms();

			// make sure that in single step mode, the render objects get updated immediately
			if (mOneFrameUpdate)  
			{
				updateRenderObjectsSync(dtime);
				updateRenderObjectsAsync(dtime);
			}
			else
				updateRenderObjectsSync(dtime);
	
		}
	}
#else

	if(!isPaused() && mScene && mWaitForResults )
	{
		Stepper* stepper = getStepper();
		//stepper->wait(mScene);
		stepper->postRender(dtime);
	}
	
#endif

	
	
	RenderPhysX3Debug* debugRenderer = getDebugRenderer();
	if(debugRenderer && mScene)
	{
		const PxRenderBuffer& debugRenderable = mScene->getRenderBuffer();
		PX_UNUSED(debugRenderable);
		debugRenderer->clear();

#if !PX_PROFILE
		debugRenderer->update(debugRenderable);
		updateRenderObjectsDebug(dtime);
#endif

		renderScene();
	}

	if(mOneFrameUpdate)
	{
		mOneFrameUpdate = false;
		if (!isPaused()) togglePause();
	}

#ifdef PRINT_BLOCK_COUNTERS
	static PxU32 refMax = -1;
	PxU32 newMax = getActiveScene().getMaxNbContactDataBlocksUsed();
	if(refMax != newMax)
		shdfnd::printFormatted("max blocks used: %d\n",newMax);
	refMax = newMax;
#endif // PRINT_BLOCK_COUNTERS
}

void PhysXSample::saveUserInputs()
{
	char name[256];
	char sBuffer[512];

	const char* sampleName = mApplication.mRunning->getName();

	SampleUserInput* sampleUserInput = mApplication.getPlatform()->getSampleUserInput();
	if(!sampleUserInput)
		return;

	strcpy(name,"input");	
	strcat(name,"/UserInputList.txt");

	if(getSampleOutputDirManager().getFilePath(name, sBuffer, false))
	{
		sprintf(name, "input/%s/%sUserInputList.txt", sampleName,mApplication.getPlatform()->getPlatformName());

		if(getSampleOutputDirManager().getFilePath(name, sBuffer, false))
		{
			SampleFramework::File* file = NULL;
			PxToolkit::fopen_s(&file, sBuffer, "w");

			if(file)
			{
				fputs("UserInputList:\n",file);
				fputs("----------------------------------------\n",file);
				const std::vector<UserInput>& ilist = sampleUserInput->getUserInputList();
				for (size_t i = 0; i < ilist.size(); i++)
				{
					const UserInput& ui = ilist[i];
					fputs(ui.m_IdName,file);
					fputs("\n",file);
				}

				fclose(file);
			}
		}
	}
}

void PhysXSample::saveInputEvents(const std::vector<const InputEvent*>& inputEvents)
{
	char name[256];
	char sBuffer[512];

	const char* sampleName = mApplication.mRunning->getName();
	SampleUserInput* sampleUserInput = mApplication.getPlatform()->getSampleUserInput();
	if(!sampleUserInput)
		return;

	strcpy(name,"input");	
	strcat(name,"/InputEventList.txt");

	if(getSampleOutputDirManager().getFilePath(name, sBuffer, false))
	{
		sprintf(name, "input/%s/%sInputEventList.txt", sampleName,mApplication.getPlatform()->getPlatformName());

		if(getSampleOutputDirManager().getFilePath(name, sBuffer, false))
		{
			SampleFramework::File* file = NULL;
			PxToolkit::fopen_s(&file, sBuffer, "w");

			if(file)
			{
				fputs("InputEventList:\n",file);
				fputs("----------------------------------------\n",file);
				for (size_t i = 0; i < inputEvents.size(); i++)
				{
					const InputEvent* inputEvent = inputEvents[i];
					
					if(!inputEvent)
						continue;
					
					const std::vector<size_t>* userInputs = sampleUserInput->getUserInputs(inputEvent->m_Id);
					const char* name = sampleUserInput->translateInputEventIdToName(inputEvent->m_Id);
					if(userInputs && !userInputs->empty() && name)
					{
						fputs(name,file);
						fputs("\n",file);
					}
				}

				fclose(file);
			}
		}
	}
}

void PhysXSample::parseSampleOutputAsset(const char* sampleName,PxU32 userInputCS, PxU32 inputEventCS)
{
	char name[256];
	char sBuffer[512];

	SampleUserInput* sampleUserInput = mApplication.getPlatform()->getSampleUserInput();
	if(!sampleUserInput)
		return;

	sprintf(name, "input/%s/%sInputMapping.ods", sampleName,mApplication.getPlatform()->getPlatformName());
	if(getSampleOutputDirManager().getFilePath(name, sBuffer, false))
	{
		SampleInputMappingAsset* inputAsset = NULL;
		SampleFramework::File* fp = NULL;
		PxToolkit::fopen_s(&fp, sBuffer, "r");
		if(fp)
		{
			inputAsset = SAMPLE_NEW(SampleInputMappingAsset)(fp,sBuffer, false, userInputCS, inputEventCS);
			if(!inputAsset->isOk())
			{
				DELETESINGLE(inputAsset);
				fp = NULL;
			}
		}

		if(inputAsset)
		{
			std::vector<SampleInputMapping> inputMappings;
			for (size_t i = inputAsset->getSampleInputData().size();i--;)
			{
				const SampleInputData& data = inputAsset->getSampleInputData()[i];
				size_t userInputIndex;
				PxI32 userInputId = sampleUserInput->translateUserInputNameToId(data.m_UserInputName,userInputIndex);
				size_t inputEventIndex;
				PxI32 inputEventId = sampleUserInput->translateInputEventNameToId(data.m_InputEventName, inputEventIndex);
				if(userInputId >= 0 && inputEventId >= 0)
				{
					SampleInputMapping mapping;
					mapping.m_InputEventId = inputEventId;
					mapping.m_InputEventIndex = inputEventIndex;
					mapping.m_UserInputId = userInputId;
					mapping.m_UserInputIndex = userInputIndex;
					inputMappings.push_back(mapping);
				}
				else
				{
					//if we get here we read a command mapping from file that is no longer supported in code ... it should be ignored.	
				}
			}

			for (size_t i = inputMappings.size(); i--;)
			{
				const SampleInputMapping& mapping = inputMappings[i];
				sampleUserInput->unregisterInputEvent(mapping.m_InputEventId);
			}

			//now I have the default keys definition left, save it to the mapping
			const std::vector<UserInput>& userInputs = sampleUserInput->getUserInputList();
			const std::map<physx::PxU16, std::vector<size_t> >& inputEventUserInputMap = sampleUserInput->getInputEventUserInputMap();
			std::map<physx::PxU16, std::vector<size_t> >::const_iterator it = inputEventUserInputMap.begin();
			std::map<physx::PxU16, std::vector<size_t> >::const_iterator itEnd = inputEventUserInputMap.end();
			while (it != itEnd)
			{
				PxU16 inputEventId = it->first;
				const std::vector<size_t>& uis = it->second;
				for (size_t j = 0; j < uis.size(); j++)
				{
					const UserInput& ui = userInputs[uis[j]];
					const char* eventName = sampleUserInput->translateInputEventIdToName(inputEventId);
					if(eventName)
					{
						inputAsset->addMapping(ui.m_IdName, eventName);							
					}
				}
				++it;
			}

			for (size_t i = inputMappings.size(); i--;)
			{
				const SampleInputMapping& mapping = inputMappings[i];
				sampleUserInput->registerInputEvent(mapping);
			}
		}
		else
		{
			// the file does not exist create one
			SampleFramework::File* fp = NULL;
			PxToolkit::fopen_s(&fp, sBuffer, "w");
			if(fp)
			{
				inputAsset = SAMPLE_NEW(SampleInputMappingAsset)(fp,sBuffer,true,userInputCS, inputEventCS);	

				const std::vector<UserInput>& userInputs = sampleUserInput->getUserInputList();
				const std::map<physx::PxU16, std::vector<size_t> >& inputEventUserInputMap = sampleUserInput->getInputEventUserInputMap();
				std::map<physx::PxU16, std::vector<size_t> >::const_iterator it = inputEventUserInputMap.begin();
				std::map<physx::PxU16, std::vector<size_t> >::const_iterator itEnd = inputEventUserInputMap.end();
				while (it != itEnd)
				{
					PxU16 inputEventId = it->first;
					const std::vector<size_t>& uis = it->second;
					for (size_t j = 0; j < uis.size(); j++)
					{
						const UserInput& ui = userInputs[uis[j]];
						const char* eventName = sampleUserInput->translateInputEventIdToName(inputEventId);
						if(eventName)
						{
							inputAsset->addMapping(ui.m_IdName, eventName);							
						}
					}
					++it;
				}
			}
		}

		if(inputAsset)
		{
			inputAsset->saveMappings();
			DELETESINGLE(inputAsset);
		}
	}
}

void PhysXSample::prepareInputEventUserInputInfo(const char* sampleName,PxU32 &userInputCS, PxU32 &inputEventCS)
{
	SampleUserInput* sampleUserInput = mApplication.getPlatform()->getSampleUserInput();
	if(!sampleUserInput)
		return;

	saveUserInputs();

	std::vector<const InputEvent*> inputEvents;
	mApplication.collectInputEvents(inputEvents);
	
	for (size_t i = inputEvents.size(); i--;)
	{
		const InputEvent* ie = inputEvents[i];
		inputEventCS += ie->m_Id;

		const std::vector<size_t>* userInputs = sampleUserInput->getUserInputs(ie->m_Id);
		if(userInputs)
		{
			for (size_t j = userInputs->size(); j--;)
			{
				const UserInput& ui = sampleUserInput->getUserInputList()[ (*userInputs)[j] ];
				userInputCS += (ui.m_Id + ie->m_Id);
			}
		}
	}

	inputEventCS = inputEventCS + ((PxU16)sampleUserInput->getInputEventList().size() << 16);
	userInputCS = userInputCS + ((PxU16)sampleUserInput->getUserInputList().size() << 16);

	saveInputEvents(inputEvents);

	char name[256];

	strcpy(name,"input/");
	strcat(name,mApplication.getPlatform()->getPlatformName());
	strcat(name,"/");
	strcat(name,sampleName);
	strcat(name,"InputMapping.ods");

	// load the additional mapping file 
	mSampleInputAsset = (SampleInputAsset*)getAsset(name, SampleAsset::ASSET_INPUT, false);
}

void PhysXSample::unregisterInputEvents()
{
	mApplication.getPlatform()->getSampleUserInput()->shutdown();
}

void PhysXSample::registerInputEvents(bool ignoreSaved)
{
	const char* sampleName = mApplication.mRunning->getName();

	SampleUserInput* sampleUserInput = mApplication.getPlatform()->getSampleUserInput();
	if(!sampleUserInput)
		return;
		
	PxU32 inputEventCS = 0;
	PxU32 userInputCS = 0;
	prepareInputEventUserInputInfo(sampleName, inputEventCS, userInputCS);	

	// register the additional mapping
	if(mSampleInputAsset)
	{
		std::vector<SampleInputMapping> inputMappings;
		for (size_t i = mSampleInputAsset->GetSampleInputData().size();i--;)
		{
			const SampleInputData& data = mSampleInputAsset->GetSampleInputData()[i];
			size_t userInputIndex;
			PxI32 userInputId = sampleUserInput->translateUserInputNameToId(data.m_UserInputName,userInputIndex);
			size_t inputEventIndex;
			PxI32 inputEventId = sampleUserInput->translateInputEventNameToId(data.m_InputEventName, inputEventIndex);
			if(userInputId >= 0 && inputEventId >= 0)
			{
				SampleInputMapping mapping;
				mapping.m_InputEventId = inputEventId;
				mapping.m_InputEventIndex = inputEventIndex;
				mapping.m_UserInputId = userInputId;
				mapping.m_UserInputIndex = userInputIndex;
				inputMappings.push_back(mapping);
			}
			else
			{
				PX_ASSERT(0);
			}
		}

		for (size_t i = inputMappings.size(); i--;)
		{
			const SampleInputMapping& mapping = inputMappings[i];
			sampleUserInput->registerInputEvent(mapping);
		}
	}

	if (!ignoreSaved)
		parseSampleOutputAsset(sampleName, inputEventCS, userInputCS);
}

void PhysXSample::onKeyDownEx(SampleFramework::SampleUserInput::KeyCode keyCode, PxU32 param)
{
}

void PhysXSample::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(SPAWN_DEBUG_OBJECT,				WKEY_1,			OSXKEY_1,		LINUXKEY_1		);
	DIGITAL_INPUT_EVENT_DEF(PAUSE_SAMPLE,					WKEY_P,			OSXKEY_P,		LINUXKEY_P		);
	DIGITAL_INPUT_EVENT_DEF(STEP_ONE_FRAME,					WKEY_O,			OSXKEY_O,		LINUXKEY_O		);
	DIGITAL_INPUT_EVENT_DEF(TOGGLE_VISUALIZATION,			WKEY_V,			OSXKEY_V,		LINUXKEY_V		);
	DIGITAL_INPUT_EVENT_DEF(DECREASE_DEBUG_RENDER_SCALE,	WKEY_F7,		OSXKEY_F7,		LINUXKEY_F7		);
	DIGITAL_INPUT_EVENT_DEF(INCREASE_DEBUG_RENDER_SCALE,	WKEY_F8,		OSXKEY_F8,		LINUXKEY_F8		);
	DIGITAL_INPUT_EVENT_DEF(HIDE_GRAPHICS,					WKEY_G,			OSXKEY_G,		LINUXKEY_G		);
	DIGITAL_INPUT_EVENT_DEF(WIREFRAME,						WKEY_N,			OSXKEY_N,		LINUXKEY_N		);
	DIGITAL_INPUT_EVENT_DEF(TOGGLE_PVD_CONNECTION,			WKEY_F5,		OSXKEY_F5,		LINUXKEY_F5		);
	DIGITAL_INPUT_EVENT_DEF(SHOW_HELP,						WKEY_H,			OSXKEY_H,		LINUXKEY_H		);
	DIGITAL_INPUT_EVENT_DEF(SHOW_DESCRIPTION,				WKEY_F,			OSXKEY_F,		LINUXKEY_F		);
	DIGITAL_INPUT_EVENT_DEF(VARIABLE_TIMESTEP,				WKEY_T,			OSXKEY_T,		LINUXKEY_T		);
	DIGITAL_INPUT_EVENT_DEF(NEXT_PAGE,						WKEY_NEXT,		OSXKEY_RIGHT,	LINUXKEY_NEXT	);
	DIGITAL_INPUT_EVENT_DEF(PREVIOUS_PAGE,					WKEY_PRIOR,		OSXKEY_LEFT,	LINUXKEY_PRIOR	);	
	DIGITAL_INPUT_EVENT_DEF(PROFILE_ONLY_PVD,				WKEY_9,			OSXKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	//DIGITAL_INPUT_EVENT_DEF(PAUSE_SAMPLE,					WKEY_P,			OSXKEY_UNKNOWN,	LINUXKEY_UNKNOWN);
	//DIGITAL_INPUT_EVENT_DEF(STEP_ONE_FRAME,				WKEY_O,			OSXKEY_UNKNOWN,	LINUXKEY_UNKNOWN);

	//digital gamepad events
	DIGITAL_INPUT_EVENT_DEF(SHOW_HELP,						GAMEPAD_SELECT,	GAMEPAD_SELECT,	LINUXKEY_UNKNOWN);
	DIGITAL_INPUT_EVENT_DEF(SPAWN_DEBUG_OBJECT,				GAMEPAD_NORTH,	GAMEPAD_NORTH,	LINUXKEY_UNKNOWN);

	//digital mouse events (are registered in the individual samples as needed)
}

void PhysXSample::onAnalogInputEvent(const SampleFramework::InputEvent& , float val)
{
}

void PhysXSample::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	if (!mScene) return;

	if (val)
	{
		if (mShowExtendedHelp)
		{
			switch (ie.m_Id)
			{
			case NEXT_PAGE:
				{
					mExtendedHelpPage++;
				}
				break;
			case PREVIOUS_PAGE:
				{
					if(mExtendedHelpPage)
						mExtendedHelpPage--;
				}
				break;
			}
			return;
		}

		switch (ie.m_Id)
		{
		case SPAWN_DEBUG_OBJECT:
			spawnDebugObject();
			break;
		case PAUSE_SAMPLE:
			togglePause();
			break;
		case STEP_ONE_FRAME:
			mOneFrameUpdate = !mOneFrameUpdate;
			break;
		case TOGGLE_VISUALIZATION:
			toggleVisualizationParam(*mScene, PxVisualizationParameter::eSCALE);
			break;
		case DECREASE_DEBUG_RENDER_SCALE:
			{
				mDebugRenderScale -= 0.1f;
				mDebugRenderScale = PxMax(mDebugRenderScale, 0.f);
				mScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, mDebugRenderScale);
				break;
			}
		case INCREASE_DEBUG_RENDER_SCALE:
			{
				mDebugRenderScale += 0.1f;
				mScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, mDebugRenderScale);
				break;
			}
		case HIDE_GRAPHICS:
			mHideGraphics = !mHideGraphics;
			break;
		case WIREFRAME:
			{
				mWireFrame = !mWireFrame;
				break;
			}
		case TOGGLE_PVD_CONNECTION:
			{
				togglePvdConnection();
				break;
			}
		case SHOW_HELP:
				mShowHelp = !mShowHelp;
				if(mShowHelp) mShowDescription=false;
			break;
		case SHOW_DESCRIPTION:
				mShowDescription = !mShowDescription;
				if(mShowDescription) mShowHelp=false;
			break;
		case VARIABLE_TIMESTEP:
			mStepperType = (mStepperType == VARIABLE_STEPPER) ? FIXED_STEPPER : VARIABLE_STEPPER;
			//mUseFixedStepper = !mUseFixedStepper;
			mFixedStepper.reset();
			mVariableStepper.reset();		
			break;
		case DELETE_PICKED:
			if(mPicked && mPicking)
			{
				PxActor* pickedActor = mPicking->letGo();
				if(pickedActor)
					pickedActor->release();
			}
			break;
		case PICKUP:
			{
				if(mPicking)
				{
					mPicked = true;
					PxU32 width;
					PxU32 height;
					mApplication.getPlatform()->getWindowSize(width, height);
					mPicking->moveCursor(width/2,height/2);
					mPicking->lazyPick();
				}
			}
			break;
		case PROFILE_ONLY_PVD:
			if (mPvdParams.useFullPvdConnection)
			{				
				if(mPvd)
				{
				  mPvd->disconnect();				
				  mPvdParams.useFullPvdConnection = false;
				  mPvd->connect(*mTransport,physx::PxPvdInstrumentationFlag::ePROFILE);
				}				
			}
			break;
		default: break;
		}
	}
	else
	{
		if (mShowExtendedHelp)
		{
			return;
		}

		if (ie.m_Id == PICKUP)
		{
			if(mPicking)
			{
				mPicked = false;
				mPicking->letGo();
			}
		}
	}
}

void PhysXSample::toggleFlyCamera()
{
	mIsFlyCamera = !mIsFlyCamera;
	if (mIsFlyCamera)
	{
		mSavedCameraController = getCurrentCameraController();
		mApplication.saveCameraState();
		mFlyCameraController.init(getCamera().getViewMatrix());
		mFlyCameraController.setMouseLookOnMouseButton(false);
		mFlyCameraController.setMouseSensitivity(0.2f);
		setCameraController(&mFlyCameraController);
	}
	else
	{
		mApplication.restoreCameraState();
		setCameraController(mSavedCameraController);
	}
}

void PhysXSample::togglePause()
{
	//unregisterInputEvents();
	mPause = !mPause;
	if (mEnableAutoFlyCamera)
	{
		if (mPause)
		{
			mSavedCameraController = getCurrentCameraController();
			mApplication.saveCameraState();
			mFlyCameraController.init(getCamera().getViewMatrix());
			setCameraController(&mFlyCameraController);
		}
		else
		{
			mApplication.restoreCameraState();
			setCameraController(mSavedCameraController);
		}
	}
	//registerInputEvents(true);
}

void PhysXSample::showExtendedInputEventHelp(PxU32 x, PxU32 y)
{
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, 255);

	Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	char message[512];

	PxU32 width = 0;
	PxU32 height = 0;
	renderer->getWindowSize(width, height);
	y += yInc;

	PxU16 numIe = (height - y)/yInc - 8;

	const std::vector<InputEvent> inputEventList = getApplication().getPlatform()->getSampleUserInput()->getInputEventList();
	const std::vector<InputEventName> inputEventNameList = getApplication().getPlatform()->getSampleUserInput()->getInputEventNameList();

	size_t maxHelpPage = inputEventList.size()/numIe;
	if(maxHelpPage < mExtendedHelpPage)
		mExtendedHelpPage = (PxU8) maxHelpPage;

	PxU16 printed = 0;
	PxU16 startPrint = 0; 
	for (size_t i = 0; i < inputEventList.size(); i++)
	{
		const InputEvent& ie = inputEventList[i];
		const char* msg = mApplication.inputInfoMsg("Press "," to ", ie.m_Id, -1);
		if(msg)
		{
			startPrint++;
			if(startPrint <= numIe*mExtendedHelpPage)
				continue;

			strcpy(message,msg);
			strcat(message, inputEventNameList[i].m_Name);
			renderer->print(x, y, message,								scale, shadowOffset, textColor);
			y += yInc;

			printed++;
			if(printed >= numIe)
			{			
				break;
			}
		}
	}

	if(printed == 0)
	{
		if(mExtendedHelpPage)
			mExtendedHelpPage--;
	}

	y += yInc;
	const char* msg = mApplication.inputInfoMsg("Press "," to show next/previous page", NEXT_PAGE, PREVIOUS_PAGE);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
}

///////////////////////////////////////////////////////////////////////////////

RenderBaseActor* PhysXSample::createRenderBoxFromShape(PxRigidActor* actor, PxShape* shape, RenderMaterial* material, const PxReal* uvs)
{
	RenderBaseActor* shapeRenderActor = NULL;
	Renderer* renderer = getRenderer();
	PX_ASSERT(renderer);

	PxGeometryType::Enum geomType = shape->getGeometryType();
	PX_ASSERT(geomType==PxGeometryType::eBOX);
	switch(geomType)
	{
	case PxGeometryType::eBOX:
		{
			// Get physics geometry
			PxBoxGeometry geometry;
			bool status = shape->getBoxGeometry(geometry);
			PX_ASSERT(status);
			PX_UNUSED(status);
			// Create render object
			shapeRenderActor = SAMPLE_NEW(RenderBoxActor)(*renderer, geometry.halfExtents, uvs);
		}
		break;
	default: {}
	};

	if(shapeRenderActor)
	{
		link(shapeRenderActor, shape, actor);
		mRenderActors.push_back(shapeRenderActor);
		shapeRenderActor->setRenderMaterial(material);
		shapeRenderActor->setEnableCameraCull(true);
	}
	return shapeRenderActor;
}


RenderBaseActor* PhysXSample::createRenderObjectFromShape(PxRigidActor* actor, PxShape* shape, RenderMaterial* material)
{
	PX_ASSERT(getRenderer());

	RenderBaseActor* shapeRenderActor = NULL;
	Renderer& renderer = *getRenderer();

	PxGeometryHolder geom = shape->getGeometry();

	switch(geom.getType())
	{
	case PxGeometryType::eSPHERE:
		shapeRenderActor = SAMPLE_NEW(RenderSphereActor)(renderer, geom.sphere().radius);
		break;
	case PxGeometryType::ePLANE:
		shapeRenderActor = SAMPLE_NEW(RenderGridActor)(renderer, 50, 1.f, PxShapeExt::getGlobalPose(*shape, *actor).q);
		break;
	case PxGeometryType::eCAPSULE:
		shapeRenderActor = SAMPLE_NEW(RenderCapsuleActor)(renderer, geom.capsule().radius, geom.capsule().halfHeight);
		break;
	case PxGeometryType::eBOX:
		shapeRenderActor = SAMPLE_NEW(RenderBoxActor)(renderer, geom.box().halfExtents);
		break;
	case PxGeometryType::eCONVEXMESH:
		{
			PxConvexMesh* convexMesh = geom.convexMesh().convexMesh;

			// ### doesn't support scale
			PxU32 nbVerts = convexMesh->getNbVertices();
			PX_UNUSED(nbVerts);
			const PxVec3* convexVerts = convexMesh->getVertices();
			const PxU8* indexBuffer = convexMesh->getIndexBuffer();
			PxU32 nbPolygons = convexMesh->getNbPolygons();

			PxU32 totalNbTris = 0;
			PxU32 totalNbVerts = 0;
			for(PxU32 i=0;i<nbPolygons;i++)
			{
				PxHullPolygon data;
				bool status = convexMesh->getPolygonData(i, data);
				PX_ASSERT(status);
				PX_UNUSED(status);
				totalNbVerts += data.mNbVerts;
				totalNbTris += data.mNbVerts - 2;
			}

			PxVec3Alloc* allocVerts = SAMPLE_NEW(PxVec3Alloc)[totalNbVerts];
			PxVec3Alloc* allocNormals = SAMPLE_NEW(PxVec3Alloc)[totalNbVerts];
			PxReal*	UVs = (PxReal*)SAMPLE_ALLOC(sizeof(PxReal)*(totalNbVerts * 2));
			PxU16* faces = (PxU16*)SAMPLE_ALLOC(sizeof(PxU16)*totalNbTris*3);

			PxU16* triangles = faces;
			PxVec3* vertices = allocVerts,
				  * normals = allocNormals;

			PxU32 offset = 0;
			for(PxU32 i=0;i<nbPolygons;i++)
			{
				PxHullPolygon face;
				bool status = convexMesh->getPolygonData(i, face);
				PX_ASSERT(status);
				PX_UNUSED(status);
	
				const PxU8* faceIndices = indexBuffer+face.mIndexBase;
				for(PxU32 j=0;j<face.mNbVerts;j++)
				{
					vertices[offset+j] = convexVerts[faceIndices[j]];
					normals[offset+j] = PxVec3(face.mPlane[0], face.mPlane[1], face.mPlane[2]);
				}

				for(PxU32 j=2;j<face.mNbVerts;j++)
				{
					*triangles++ = PxU16(offset);
					*triangles++ = PxU16(offset+j);
					*triangles++ = PxU16(offset+j-1);
				}

				offset += face.mNbVerts;
			}

			// prepare UVs for convex:
			// filling like this
			// vertice #0 - 0,0
			// vertice #1 - 0,1
			// vertice #2 - 1,0
			// vertice #3 - 0,0
			// ...
			for(PxU32 i = 0; i < totalNbVerts; ++i)
			{
				PxU32 c = i % 3;
				if(c == 0)
				{
					UVs[2 * i] = 0.0f;
					UVs[2 * i + 1] = 0.0f;
				}
				else if(c == 1) 
				{
					UVs[2 * i] = 0.0f;
					UVs[2 * i + 1] = 1.0f;
				}
				else if(c == 2)
				{
					UVs[2 * i] = 1.0f;
					UVs[2 * i + 1] = 0.0f;
				}
			}

			PX_ASSERT(offset==totalNbVerts);
			shapeRenderActor = SAMPLE_NEW(RenderMeshActor)(renderer, vertices, totalNbVerts, normals, UVs, faces, NULL, totalNbTris);
			shapeRenderActor->setMeshScale(geom.convexMesh().scale);

			SAMPLE_FREE(faces);
			SAMPLE_FREE(UVs);
			DELETEARRAY(allocVerts);
			DELETEARRAY(allocNormals);
		}
		break;
	case PxGeometryType::eTRIANGLEMESH:
		{
			// Get physics geometry
			PxTriangleMesh* tm = geom.triangleMesh().triangleMesh;

			const PxU32 nbVerts = tm->getNbVertices();
			const PxVec3* verts = tm->getVertices();
			const PxU32 nbTris = tm->getNbTriangles();
			const void* tris = tm->getTriangles();
			const bool has16bitIndices = tm->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
			const PxU16* faces16 = has16bitIndices ? (const PxU16*)tris : NULL;
			const PxU32* faces32 = has16bitIndices ? NULL : (const PxU32*)tris;
			shapeRenderActor = SAMPLE_NEW(RenderMeshActor)(renderer, verts, nbVerts, NULL, NULL, faces16, faces32, nbTris, true);			
			shapeRenderActor->setMeshScale(geom.triangleMesh().scale);

			if (!material)
				material = mManagedMaterials[MATERIAL_FLAT];
		}
		break;
	case PxGeometryType::eHEIGHTFIELD:
		{
			// Get physics geometry
			const PxHeightFieldGeometry& geometry = geom.heightField();
			
			const PxReal	rs = geometry.rowScale;
			const PxReal	hs = geometry.heightScale;
			const PxReal	cs = geometry.columnScale;

			// Create render object
			PxHeightField*	hf = geometry.heightField;
			const PxU32		nbCols = hf->getNbColumns();
			const PxU32		nbRows = hf->getNbRows();
			const PxU32		nbVerts = nbRows * nbCols;	
			const PxU32     nbFaces = (nbCols - 1) * (nbRows - 1) * 2;

			PxHeightFieldSample* sampleBuffer = new PxHeightFieldSample[nbVerts];
			hf->saveCells(sampleBuffer, nbVerts * sizeof(PxHeightFieldSample));

			PxVec3* vertexes = new PxVec3[nbVerts];
			for(PxU32 i = 0; i < nbRows; i++) 
			{
				for(PxU32 j = 0; j < nbCols; j++) 
				{
					vertexes[i * nbCols + j] = PxVec3(PxReal(i) * rs, PxReal(sampleBuffer[j + (i*nbCols)].height) * hs, PxReal(j) * cs);
				}
			}

			PxU32* indices = (PxU32*)SAMPLE_ALLOC(sizeof(PxU32) * nbFaces * 3);
			for(PxU32 i = 0; i < (nbCols - 1); ++i) 
			{
				for(PxU32 j = 0; j < (nbRows - 1); ++j) 
				{
					PxU8 tessFlag = sampleBuffer[i+j*nbCols].tessFlag();
					PxU32 i0 = j*nbCols + i;
					PxU32 i1 = j*nbCols + i + 1;
					PxU32 i2 = (j+1) * nbCols + i;
					PxU32 i3 = (j+1) * nbCols + i+ 1;
					// i2---i3
					// |    |
					// |    |
					// i0---i1
					// this is really a corner vertex index, not triangle index
					PxU32 mat0 = hf->getTriangleMaterialIndex((j*nbCols+i)*2);
					PxU32 mat1 = hf->getTriangleMaterialIndex((j*nbCols+i)*2+1);
					bool hole0 = (mat0 == PxHeightFieldMaterial::eHOLE);
					bool hole1 = (mat1 == PxHeightFieldMaterial::eHOLE);
					// first triangle
					indices[6 * (i * (nbRows - 1) + j) + 0] = hole0 ? i0 : i2; // duplicate i0 to make a hole
					indices[6 * (i * (nbRows - 1) + j) + 1] = i0;
					indices[6 * (i * (nbRows - 1) + j) + 2] = tessFlag ? i3 : i1;
					// second triangle
					indices[6 * (i * (nbRows - 1) + j) + 3] = hole1 ? i1 : i3; // duplicate i1 to make a hole
					indices[6 * (i * (nbRows - 1) + j) + 4] = tessFlag ? i0 : i2;
					indices[6 * (i * (nbRows - 1) + j) + 5] = i1;
				}
			}
			
			PxU16* indices_16bit = (PxU16*)SAMPLE_ALLOC(sizeof(PxU16) * nbFaces * 3);
			for(PxU32 i=0;  i< nbFaces; i++)
			{
				indices_16bit[i*3+0] = indices[i*3+0];
				indices_16bit[i*3+1] = indices[i*3+2];
				indices_16bit[i*3+2] = indices[i*3+1];
			}
		
			shapeRenderActor = SAMPLE_NEW(RenderMeshActor)(renderer, vertexes, nbVerts, NULL, NULL, indices_16bit, NULL, nbFaces);
			SAMPLE_FREE(indices_16bit);
			SAMPLE_FREE(indices);
			DELETEARRAY(sampleBuffer);
			DELETEARRAY(vertexes);

			if (!material)				
				material = mManagedMaterials[MATERIAL_FLAT];
		}
		break;
		default: {}
	};

	if(shapeRenderActor)
	{
		link(shapeRenderActor, shape, actor);
		mRenderActors.push_back(shapeRenderActor);
		shapeRenderActor->setRenderMaterial(material);
		shapeRenderActor->setEnableCameraCull(true);
	}
	return shapeRenderActor;
}

void PhysXSample::updateRenderObjectsFromRigidActor(PxRigidActor& actor, RenderMaterial* mat)
{
	PxU32 nbShapes = actor.getNbShapes();
	if(nbShapes > 0)
	{
		const PxU32 nbShapesOnStack = 8;
		PxShape* shapesOnStack[nbShapesOnStack], **shapes = &shapesOnStack[0];
		if(nbShapes > nbShapesOnStack)
			shapes = new PxShape*[nbShapes];
		actor.getShapes(shapes, nbShapes);

		for(PxU32 i = 0; i < nbShapes; ++i)
		{
			RenderBaseActor* renderActor = getRenderActor(&actor, shapes[i]);
			if(renderActor != NULL)
			{
				renderActor->mActive = true;
				if (mat != NULL)
					renderActor->setRenderMaterial(mat);
			}
			else 
				createRenderObjectFromShape(&actor, shapes[i], mat);
		}

		if(nbShapes > nbShapesOnStack)
			delete[] shapes;
	}
}

void PhysXSample::createRenderObjectsFromScene()
{
	PxScene& scene = getActiveScene();

	PxActorTypeFlags types = PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC;

	PxU32 nbActors = scene.getNbActors(types);
	if(nbActors)
	{
		PxActor** actors = new PxActor* [nbActors];
		scene.getActors(types, actors, nbActors);
		for(PxU32 i = 0; i < nbActors; ++i) 
		{
			switch (actors[i]->getType())
			{
			case PxActorType::eRIGID_STATIC:
			case PxActorType::eRIGID_DYNAMIC:
				updateRenderObjectsFromRigidActor(*reinterpret_cast<PxRigidActor*>(actors[i]));
				break;
			default:
				break;
			}
		}
		delete[] actors;
	}

	PxU32 nbArticulations = scene.getNbArticulations();
	if(nbArticulations > 0)
	{
		PxArticulationBase** articulations = new PxArticulationBase* [nbArticulations];
		scene.getArticulations(articulations, nbArticulations);	
		for(PxU32 i=0; i < nbArticulations; i++)
		{
			updateRenderObjectsFromArticulation(*articulations[i]);
		}
		delete[] articulations;
	}	
}

void PhysXSample::updateRenderObjectsFromArticulation(PxArticulationBase& articulation)
{
	SampleInlineArray<PxArticulationLink*,20> links;
	links.resize(articulation.getNbLinks());
	articulation.getLinks(links.begin(), links.size());
	
	for(PxU32 i=0; i < links.size(); i++)
	{
		updateRenderObjectsFromRigidActor(*links[i]);
	}
}

void PhysXSample::createRenderObjectsFromActor(PxRigidActor* rigidActor, RenderMaterial* material)
{
	PX_ASSERT(rigidActor);

	PxU32 nbShapes = rigidActor->getNbShapes();
	if(!nbShapes)
		return;

	PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*nbShapes);
	PxU32 nb = rigidActor->getShapes(shapes, nbShapes);
	PX_ASSERT(nb==nbShapes);
	PX_UNUSED(nb);
	for(PxU32 i=0;i<nbShapes;i++)
	{
		createRenderObjectFromShape(rigidActor, shapes[i], material);
	}
	SAMPLE_FREE(shapes);
}

void PhysXSample::updateRenderObjectsDebug(float dtime)
{
	RenderPhysX3Debug* debugRenderer = getDebugRenderer();
	if(debugRenderer && mScene)
	{
		for(PxU32 i = 0; i < mRenderActors.size(); ++i)
		{
			if (mRenderActors[i]->getEnableDebugRender())
				mRenderActors[i]->drawDebug(debugRenderer);
		}
		getCamera().drawDebug(debugRenderer);

#ifdef VISUALIZE_PICKING_RAYS
		if(mPicking)
		{
			const std::vector<Picking::Ray>& rays = mPicking->getRays();
			PxU32 nbRays = rays.size();
			const RendererColor color(255, 0, 0);
			for(PxU32 i=0;i<nbRays;i++)
			{
				debugRenderer->addLine(rays[i].origin, rays[i].origin + rays[i].dir * 1000.0f, color);
			}
		}
#endif
#ifdef VISUALIZE_PICKING_TRIANGLES
		if(mPicking && mPicking->pickedTriangleIsValid())
		{
			PxTriangle touchedTri = mPicking->getPickedTriangle();
			RendererColor color(255, 255, 255);
			debugRenderer->addLine(touchedTri.verts[0], touchedTri.verts[1], color);
			debugRenderer->addLine(touchedTri.verts[1], touchedTri.verts[2], color);
			debugRenderer->addLine(touchedTri.verts[2], touchedTri.verts[0], color);
			for(PxU32 i=0;i<3;i++)
			{
				if(mPicking->pickedTriangleAdjacentIsValid(i))
				{
					touchedTri = mPicking->getPickedTriangleAdjacent(i);
					if(i==0)
						color = RendererColor(255, 0, 0);
					else if(i==1)
						color = RendererColor(0, 255, 0);
					else if(i==2)
						color = RendererColor(0, 0, 255);
					debugRenderer->addLine(touchedTri.verts[0], touchedTri.verts[1], color);
					debugRenderer->addLine(touchedTri.verts[1], touchedTri.verts[2], color);
					debugRenderer->addLine(touchedTri.verts[2], touchedTri.verts[0], color);
				}
			}
		}
#endif
	}
}

void PhysXSample::initRenderObjects()
{
	PxSceneWriteLock scopedLock(*mScene);
	for (PxU32 i = 0; i < mRenderActors.size(); ++i)
	{
		mRenderActors[i]->update(0.0f);
	}
}

void PhysXSample::updateRenderObjectsSync(float dtime)
{
	PxSceneWriteLock scopedLock(*mScene);
}

void PhysXSample::updateRenderObjectsAsync(float dtime)
{
	PX_PROFILE_ZONE("updateRenderObjectsAsync", 0);
	if(mActiveTransformCount)
	{
		PxSceneWriteLock scopedLock(*mScene);
		PxU32 nbActiveTransforms = mActiveTransformCount;
		PxActor** currentActor = mBufferedActiveActors;
		while(nbActiveTransforms--)
		{
			PxActor* actor = *currentActor++;

			// Check that actor is not a deleted actor
			bool isDeleted = false;
			for (PxU32 i=0; i < mDeletedActors.size(); i++)
			{
				if (mDeletedActors[i] == actor)
				{
					isDeleted = true;
					break;
				}
			}
			if (isDeleted) continue;

			const PxType actorType = actor->getConcreteType();
			if(actorType==PxConcreteType::eRIGID_DYNAMIC || actorType==PxConcreteType::eRIGID_STATIC 
				|| actorType == PxConcreteType::eARTICULATION_LINK || actorType == PxConcreteType::eARTICULATION_JOINT)
			{
				PxRigidActor* rigidActor = static_cast<PxRigidActor*>(actor);
				PxU32 nbShapes = rigidActor->getNbShapes();
				for(PxU32 i=0;i<nbShapes;i++)
				{
					PxShape* shape;
					PxU32 n = rigidActor->getShapes(&shape, 1, i);
					PX_ASSERT(n==1);
					PX_UNUSED(n);
					RenderBaseActor* renderActor = getRenderActor(rigidActor, shape);
					if (NULL != renderActor)
						renderActor->update(dtime);
				}
			}
		}
		mActiveTransformCount = 0;
		mDeletedActors.clear();
	}
}

void PhysXSample::bufferActiveTransforms()
{
	PxSceneReadLock scopedLock(*mScene);
	// buffer active transforms to perform render object update parallel to simulation

	PxActor** activeTransforms = mScene->getActiveActors(mActiveTransformCount);
	if(mActiveTransformCapacity < mActiveTransformCount)
	{
		SAMPLE_FREE(mBufferedActiveActors);
		mActiveTransformCapacity = (PxU32)(mActiveTransformCount * 1.5f);
		mBufferedActiveActors = (PxActor**)SAMPLE_ALLOC(sizeof(PxActor*) * mActiveTransformCapacity);
	}
	if(mActiveTransformCount)
		PxMemCopy(mBufferedActiveActors, activeTransforms, sizeof(PxActor*) * mActiveTransformCount);

}

///////////////////////////////////////////////////////////////////////////////
RenderMaterial* PhysXSample::createRenderMaterialFromTextureFile(const char* filename)
{
	RenderMaterial* material = NULL;
	if (!filename)
		return NULL;

	RAWTexture data;
	data.mName = filename;
	RenderTexture* texture = createRenderTextureFromRawTexture(data);	
	material = SAMPLE_NEW(RenderMaterial)(*getRenderer(), PxVec3(0.7f, 0.7f, 0.7f), 1.0f, true, MATERIAL_CLOTH, texture);
	mRenderMaterials.push_back(material);

	return material;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidActor* PhysXSample::createGrid(RenderMaterial* material)
{
	PxSceneWriteLock scopedLock(*mScene);
	Renderer* renderer = getRenderer();
	PX_ASSERT(renderer);

	PxRigidStatic* plane = PxCreatePlane(*mPhysics, PxPlane(PxVec3(0,1,0), 0), *mMaterial);
	if(!plane)
		fatalError("create plane failed!");

	mScene->addActor(*plane);

	PxShape* shape;
	plane->getShapes(&shape, 1);

	RenderGridActor* actor = SAMPLE_NEW(RenderGridActor)(*renderer, 20, 10.0f);
	link(actor, shape, plane);
	actor->setTransform(PxTransform(PxIdentity));
	mRenderActors.push_back(actor);
	actor->setRenderMaterial(material);
	return plane;
}

///////////////////////////////////////////////////////////////////////////////

void PhysXSample::spawnDebugObject()
{
	PxSceneWriteLock scopedLock(*mScene);
	PxU32 types = getDebugObjectTypes();

	if (!types)
		return;

	//select legal type
	while ((mDebugObjectType & types) == 0)
		mDebugObjectType = (mDebugObjectType << 1) ? 0 : 1;

	if ((mDebugObjectType & DEBUG_OBJECT_ALL) == 0)
		return; 

	const PxVec3 pos = getCamera().getPos();
	const PxVec3 vel = getCamera().getViewDir() * getDebugObjectsVelocity();

	PxRigidDynamic* actor = NULL;

	switch (mDebugObjectType)
	{
	case DEBUG_OBJECT_SPHERE:
		actor = createSphere(pos, getDebugSphereObjectRadius(), &vel, mManagedMaterials[MATERIAL_GREEN], mDefaultDensity);
		break;
	case DEBUG_OBJECT_BOX:
		actor = createBox(pos, getDebugBoxObjectExtents(), &vel, mManagedMaterials[MATERIAL_RED], mDefaultDensity);
		break;
	case DEBUG_OBJECT_CAPSULE:
		actor = createCapsule(pos, getDebugCapsuleObjectRadius(), getDebugCapsuleObjectHalfHeight(), &vel, mManagedMaterials[MATERIAL_BLUE], mDefaultDensity);
		break;
	case DEBUG_OBJECT_CONVEX:
		actor = createConvex(pos, &vel, mManagedMaterials[MATERIAL_YELLOW], mDefaultDensity);
		break;
	case DEBUG_OBJECT_COMPOUND:
		actor = createTestCompound(pos, 320, 0.1f, 2.0f, &vel, NULL, mDefaultDensity, true);
		break;
	}

	actor->setWakeCounter(1000000);
	
	if (actor)
		onDebugObjectCreation(actor);

	//switch type
	mDebugObjectType = mDebugObjectType << 1;
	while ((mDebugObjectType & types) == 0)
		mDebugObjectType = (mDebugObjectType << 1) ? 0 : 1;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXSample::createBox(const PxVec3& pos, const PxVec3& dims, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);
	PxRigidDynamic* box = PxCreateDynamic(*mPhysics, PxTransform(pos), PxBoxGeometry(dims), *mMaterial, density);
	PX_ASSERT(box);

	SetupDefaultRigidDynamic(*box);
	mScene->addActor(*box);
	addPhysicsActors(box);

	if(linVel)
		box->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(box, material);

	return box;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXSample::createSphere(const PxVec3& pos, PxReal radius, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);
	PxRigidDynamic* sphere = PxCreateDynamic(*mPhysics, PxTransform(pos), PxSphereGeometry(radius), *mMaterial, density);
	PX_ASSERT(sphere);

	SetupDefaultRigidDynamic(*sphere);
	mScene->addActor(*sphere);
	addPhysicsActors(sphere);

	if(linVel)
		sphere->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(sphere, material);

	return sphere;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXSample::createCapsule(const PxVec3& pos, PxReal radius, PxReal halfHeight, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);
	const PxQuat rot = PxQuat(PxIdentity);
	PX_UNUSED(rot);

	PxRigidDynamic* capsule = PxCreateDynamic(*mPhysics, PxTransform(pos), PxCapsuleGeometry(radius, halfHeight), *mMaterial, density);
	PX_ASSERT(capsule);

	SetupDefaultRigidDynamic(*capsule);
	mScene->addActor(*capsule);
	addPhysicsActors(capsule);

	if(linVel)
		capsule->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(capsule, material);

	return capsule;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXSample::createConvex(const PxVec3& pos, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);
	PxConvexMesh* convexMesh = GenerateConvex(*mPhysics, *mCooking, getDebugConvexObjectScale(), false, true);
	PX_ASSERT(convexMesh);

	PxRigidDynamic* convex = PxCreateDynamic(*mPhysics, PxTransform(pos), PxConvexMeshGeometry(convexMesh), *mMaterial, density); 
	PX_ASSERT(convex);

	SetupDefaultRigidDynamic(*convex);
	mScene->addActor(*convex);
	addPhysicsActors(convex);

	if(linVel)
		convex->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(convex, material);

	return convex;
}

///////////////////////////////////////////////////////////////////////////////

PxRigidDynamic* PhysXSample::createCompound(const PxVec3& pos, const std::vector<PxTransform>& localPoses, const std::vector<const PxGeometry*>& geometries, const PxVec3* linVel, RenderMaterial* material, PxReal density)
{
	PxSceneWriteLock scopedLock(*mScene);
	PxRigidDynamic* compound = GenerateCompound(*mPhysics, mScene, mMaterial, pos, PxQuat(PxIdentity), localPoses, geometries, false, density);

	addPhysicsActors(compound);
	if(linVel)
		compound->setLinearVelocity(*linVel);

	createRenderObjectsFromActor(compound, material);

	return compound;
}

PxRigidDynamic* PhysXSample::createTestCompound(const PxVec3& pos, PxU32 nbBoxes, float boxSize, float amplitude, const PxVec3* vel, RenderMaterial* material, PxReal density, bool makeSureVolumeEmpty)
{
	PxSceneWriteLock scopedLock(*mScene);
	if (makeSureVolumeEmpty)
	{
		// Kai: a little bigger than amplitude + boxSize * sqrt(3)
		PxSphereGeometry geometry(amplitude + boxSize + boxSize);
		PxTransform pose(pos);
		PxOverlapBuffer buf;
		getActiveScene().overlap(geometry, pose, buf,
			PxQueryFilterData(PxQueryFlag::eANY_HIT|PxQueryFlag::eSTATIC|PxQueryFlag::eDYNAMIC));
		if (buf.hasBlock) {
//			shdfnd::printFormatted("desination volume is not empty!!!\n");
			return NULL;
		}
	}

	std::vector<PxTransform> localPoses;
	std::vector<const PxGeometry*> geometries;

	PxToolkit::BasicRandom rnd(42);

	PxBoxGeometryAlloc* geoms = SAMPLE_NEW(PxBoxGeometryAlloc)[nbBoxes];

	for(PxU32 i=0;i<nbBoxes;i++)
	{
		geoms[i].halfExtents = PxVec3(boxSize);

		PxTransform localPose;
		rnd.unitRandomPt(localPose.p);
		localPose.p.normalize();
		localPose.p *= amplitude;
		rnd.unitRandomQuat(localPose.q);

		localPoses.push_back(localPose);
		geometries.push_back(&geoms[i]);
	}
	PxRigidDynamic* actor = createCompound(pos, localPoses, geometries, vel, material, density);
	DELETEARRAY(geoms);
	return actor;
}

///////////////////////////////////////////////////////////////////////////////

struct FindRenderActor
{
	FindRenderActor(PxShape* pxShape): mPxShape(pxShape) {}
	bool operator() (const RenderBaseActor* actor) { return actor->getPhysicsShape() == mPxShape; }
	PxShape* mPxShape;
};

void PhysXSample::removeRenderObject(RenderBaseActor* renderActor)
{
	std::vector<RenderBaseActor*>::iterator renderIter = std::find(mRenderActors.begin(), mRenderActors.end(), renderActor);
	if(renderIter != mRenderActors.end())
	{
		// ######### PT: TODO: unlink call missing here!!!
		mDeletedRenderActors.push_back((*renderIter));
		mRenderActors.erase(renderIter);
	}
}

//////////////////////////////////////////////////////////////////////////

void PhysXSample::removeRenderActorsFromPhysicsActor(const PxRigidActor* actor)
{
	const PxU32 numShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
	actor->getShapes(shapes, numShapes);
	for(PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = shapes[i];
		FindRenderActor findRenderActor(shape);
		std::vector<RenderBaseActor*>::iterator renderIter = std::find_if(mRenderActors.begin(), mRenderActors.end(), findRenderActor);
		if(renderIter != mRenderActors.end())
		{
			unlink((*renderIter), shape, const_cast<PxRigidActor*>(actor));
			mDeletedRenderActors.push_back((*renderIter));
			mRenderActors.erase(renderIter);
		}
	}

	// check if the actor is in the active transform list and remove
	if(actor->getType() == PxActorType::eRIGID_DYNAMIC)
	{
		for(PxU32 i=0; i < mActiveTransformCount; i++)
		{
			if(mBufferedActiveActors[i] == actor)
			{
				mBufferedActiveActors[i] = mBufferedActiveActors[mActiveTransformCount-1];
				mActiveTransformCount--;
				break;
			}
		}
		mDeletedActors.push_back(const_cast<PxRigidActor*>(actor));
	}

	SAMPLE_FREE(shapes);
}

//////////////////////////////////////////////////////////////////////////

void PhysXSample::removeActor(PxRigidActor* actor)
{
	removeRenderActorsFromPhysicsActor(actor);

	std::vector<PxRigidActor*>::iterator actorIter = std::find(mPhysicsActors.begin(), mPhysicsActors.end(), actor);
	if(actorIter != mPhysicsActors.end())
	{
		mPhysicsActors.erase(actorIter);
		actor->release();
	}
}

///////////////////////////////////////////////////////////////////////////////

SampleFramework::SampleAsset* PhysXSample::getAsset(const char* relativePath, SampleFramework::SampleAsset::Type type, bool abortOnError)
{
	SampleFramework::SampleAsset* asset = mApplication.getAssetManager()->getAsset(relativePath, type);
	if (NULL == asset && abortOnError)
	{
		std::string msg = "Error while getting material asset ";
		msg += relativePath;
		msg += "\n";
		fatalError(msg.c_str());
	}
	return asset;
}

void PhysXSample::importRAWFile(const char* relativePath, PxReal scale, bool recook)
{
	mMeshTag	= 0;
	mFilename	= relativePath;
	mScale		= scale;

	const bool saved = gRecook;
	if(!gRecook && recook)
		gRecook = true;

	bool status = loadRAWfile(getSampleMediaFilename(mFilename), *this, scale);
	if (!status)
	{
		std::string msg = "Sample can not load file ";
		msg += getSampleMediaFilename(mFilename);
		msg += "\n";
		fatalError(msg.c_str());
	}

	gRecook = saved;
}


