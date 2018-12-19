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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxPhysicsAPI.h"
#include "PxMetaDataObjects.h"
#include "CmIO.h"
#include "SnPxStreamOperators.h"
#include "PsUtilities.h"
#include "SnXmlImpl.h"			
#include "SnXmlSerializer.h"		
#include "SnXmlDeserializer.h"		
#include "SnRepXCoreSerializer.h"

using namespace physx::Sn;
namespace physx { 
	typedef PxReadOnlyPropertyInfo<PxPropertyInfoName::PxArticulationLink_InboundJoint, PxArticulationLink, PxArticulationJoint *> TIncomingJointPropType;
		
	//*************************************************************
	//	Actual RepXSerializer implementations for PxMaterial
	//*************************************************************
	PxMaterial* PxMaterialRepXSerializer::allocateObject( PxRepXInstantiationArgs& inArgs )
	{
		return inArgs.physics.createMaterial(0, 0, 0);
	}

	PxRepXObject PxShapeRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* inCollection )
	{
		PxProfileAllocatorWrapper wrapper( inAllocator.getAllocator() );
		TReaderNameStack names( wrapper );
		PxProfileArray<PxU32> contexts( wrapper );
		bool hadError = false;
		RepXVisitorReader<PxShape> theVisitor( names, contexts, inArgs, inReader, NULL, inAllocator, *inCollection, hadError );

		Ps::Array<PxMaterial*> materials;
		PxGeometry* geometry = NULL; 
		parseShape( theVisitor, geometry, materials );
		if(hadError)
			return PxRepXObject();
		PxShape *theShape = inArgs.physics.createShape( *geometry, materials.begin(), Ps::to16(materials.size()) );

		switch(geometry->getType())
		{
		case PxGeometryType::eSPHERE :
			static_cast<PxSphereGeometry*>(geometry)->~PxSphereGeometry();
			break;
		case PxGeometryType::ePLANE :
			static_cast<PxPlaneGeometry*>(geometry)->~PxPlaneGeometry();
			break;
		case PxGeometryType::eCAPSULE :
			static_cast<PxCapsuleGeometry*>(geometry)->~PxCapsuleGeometry();
			break;
		case PxGeometryType::eBOX :
			static_cast<PxBoxGeometry*>(geometry)->~PxBoxGeometry();
			break;
		case PxGeometryType::eCONVEXMESH :
			static_cast<PxConvexMeshGeometry*>(geometry)->~PxConvexMeshGeometry();
			break;
		case PxGeometryType::eTRIANGLEMESH :
			static_cast<PxTriangleMeshGeometry*>(geometry)->~PxTriangleMeshGeometry();
			break;
		case PxGeometryType::eHEIGHTFIELD :
			static_cast<PxHeightFieldGeometry*>(geometry)->~PxHeightFieldGeometry();
			break;

		case PxGeometryType::eGEOMETRY_COUNT:
		case PxGeometryType::eINVALID:
			PX_ASSERT(0);			
		}		
		inAllocator.getAllocator().deallocate(geometry);

		bool ret = readAllProperties( inArgs, inReader, theShape, inAllocator, *inCollection );

		return ret ? PxCreateRepXObject(theShape) : PxRepXObject();
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxTriangleMesh
	//*************************************************************

	template<typename TTriIndexElem>
	inline void writeTriangle( MemoryBuffer& inTempBuffer, const Triangle<TTriIndexElem>& inTriangle )
	{
		inTempBuffer << inTriangle.mIdx0 
			<< " " << inTriangle.mIdx1
			<< " " << inTriangle.mIdx2;
	}

	PxU32 materialAccess( const PxTriangleMesh* inMesh, PxU32 inIndex ) { return inMesh->getTriangleMaterialIndex( inIndex ); }
	template<typename TDataType>
	void writeDatatype( MemoryBuffer& inTempBuffer, const TDataType& inType ) { inTempBuffer << inType; }

	void PxBVH33TriangleMeshRepXSerializer::objectToFileImpl( const PxBVH33TriangleMesh* mesh, PxCollection* /*inCollection*/, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& inArgs )
	{
		bool hasMatIndex = mesh->getTriangleMaterialIndex(0) != 0xffff;
		PxU32 numVertices = mesh->getNbVertices();
		const PxVec3* vertices = mesh->getVertices();
		writeBuffer( inWriter, inTempBuffer, 2, vertices, numVertices, "Points", writePxVec3 );
		bool isU16 = mesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
		PxU32 triCount = mesh->getNbTriangles();
		const void* indices = mesh->getTriangles();
		if ( isU16 )
			writeBuffer( inWriter, inTempBuffer, 2, reinterpret_cast<const Triangle<PxU16>* >( indices ), triCount, "Triangles", writeTriangle<PxU16> );
		else
			writeBuffer( inWriter, inTempBuffer, 2, reinterpret_cast<const Triangle<PxU32>* >( indices ), triCount, "Triangles", writeTriangle<PxU32> );
		if ( hasMatIndex )
			writeBuffer( inWriter, inTempBuffer, 6, mesh, materialAccess, triCount, "materialIndices", writeDatatype<PxU32> );

		//Cooked stream			
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = numVertices;
		meshDesc.points.data = vertices;
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.triangles.count = triCount;
		meshDesc.triangles.data = indices;
		meshDesc.triangles.stride = isU16?3*sizeof(PxU16):3*sizeof(PxU32);

		if(isU16)
		{
			meshDesc.triangles.stride = sizeof(PxU16)*3;
			meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
		}
		else
		{
			meshDesc.triangles.stride = sizeof(PxU32)*3;
		}

		if(hasMatIndex)
		{
			PxMaterialTableIndex* materialIndices = new PxMaterialTableIndex[triCount];
			for(PxU32 i = 0; i < triCount; i++)
				materialIndices[i] = mesh->getTriangleMaterialIndex(i);

			meshDesc.materialIndices.data = materialIndices;
			meshDesc.materialIndices.stride = sizeof(PxMaterialTableIndex);

		}

		if(inArgs.cooker != NULL)
		{
			TMemoryPoolManager theManager(mAllocator);
			MemoryBuffer theTempBuf( &theManager );
			theTempBuf.clear();
			inArgs.cooker->cookTriangleMesh( meshDesc, theTempBuf );

			writeBuffer( inWriter, inTempBuffer, 16, theTempBuf.mBuffer, theTempBuf.mWriteOffset, "CookedData", writeDatatype<PxU8> );
		}

		delete []meshDesc.materialIndices.data;
	}

	PxRepXObject PxBVH33TriangleMeshRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* /*inCollection*/ )
	{
		//We can't do a simple inverse; we *have* to cook data to get a mesh.
		PxTriangleMeshDesc theDesc;
		readStridedBufferProperty<PxVec3>( inReader, "points", theDesc.points, inAllocator);
		readStridedBufferProperty<Triangle<PxU32> >( inReader, "triangles", theDesc.triangles, inAllocator);
		PxU32 triCount;
		readStridedBufferProperty<PxMaterialTableIndex>( inReader, "materialIndices", theDesc.materialIndices, triCount, inAllocator);
		PxStridedData cookedData;
		cookedData.stride = sizeof(PxU8);
		PxU32 dataSize;
		readStridedBufferProperty<PxU8>( inReader, "CookedData", cookedData, dataSize, inAllocator);

		TMemoryPoolManager theManager(inAllocator.getAllocator());	
		MemoryBuffer theTempBuf( &theManager );

//		PxTriangleMesh* theMesh = NULL;			
		PxBVH33TriangleMesh* theMesh = NULL;			

		if(dataSize != 0)
		{				
			theTempBuf.write(cookedData.data, dataSize*sizeof(PxU8));
//			theMesh = inArgs.physics.createTriangleMesh( theTempBuf );
			theMesh = static_cast<PxBVH33TriangleMesh*>(inArgs.physics.createTriangleMesh( theTempBuf ));
		}

		if(theMesh == NULL)
		{
			PX_ASSERT(inArgs.cooker);
			theTempBuf.clear();

			{
				PxCookingParams params = inArgs.cooker->getParams();
				params.midphaseDesc = PxMeshMidPhase::eBVH33;
				inArgs.cooker->setParams(params);
			}

			inArgs.cooker->cookTriangleMesh( theDesc, theTempBuf );
//			theMesh = inArgs.physics.createTriangleMesh( theTempBuf );
			theMesh = static_cast<PxBVH33TriangleMesh*>(inArgs.physics.createTriangleMesh( theTempBuf ));
		}					

		return PxCreateRepXObject( theMesh );
	}

	void PxBVH34TriangleMeshRepXSerializer::objectToFileImpl( const PxBVH34TriangleMesh* mesh, PxCollection* /*inCollection*/, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& inArgs )
	{
		bool hasMatIndex = mesh->getTriangleMaterialIndex(0) != 0xffff;
		PxU32 numVertices = mesh->getNbVertices();
		const PxVec3* vertices = mesh->getVertices();
		writeBuffer( inWriter, inTempBuffer, 2, vertices, numVertices, "Points", writePxVec3 );
		bool isU16 = mesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
		PxU32 triCount = mesh->getNbTriangles();
		const void* indices = mesh->getTriangles();
		if ( isU16 )
			writeBuffer( inWriter, inTempBuffer, 2, reinterpret_cast<const Triangle<PxU16>* >( indices ), triCount, "Triangles", writeTriangle<PxU16> );
		else
			writeBuffer( inWriter, inTempBuffer, 2, reinterpret_cast<const Triangle<PxU32>* >( indices ), triCount, "Triangles", writeTriangle<PxU32> );
		if ( hasMatIndex )
			writeBuffer( inWriter, inTempBuffer, 6, mesh, materialAccess, triCount, "materialIndices", writeDatatype<PxU32> );

		//Cooked stream			
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = numVertices;
		meshDesc.points.data = vertices;
		meshDesc.points.stride = sizeof(PxVec3);
		meshDesc.triangles.count = triCount;
		meshDesc.triangles.data = indices;
		meshDesc.triangles.stride = isU16?3*sizeof(PxU16):3*sizeof(PxU32);

		if(isU16)
		{
			meshDesc.triangles.stride = sizeof(PxU16)*3;
			meshDesc.flags |= PxMeshFlag::e16_BIT_INDICES;
		}
		else
		{
			meshDesc.triangles.stride = sizeof(PxU32)*3;
		}

		if(hasMatIndex)
		{
			PxMaterialTableIndex* materialIndices = new PxMaterialTableIndex[triCount];
			for(PxU32 i = 0; i < triCount; i++)
				materialIndices[i] = mesh->getTriangleMaterialIndex(i);

			meshDesc.materialIndices.data = materialIndices;
			meshDesc.materialIndices.stride = sizeof(PxMaterialTableIndex);

		}

		if(inArgs.cooker != NULL)
		{
			TMemoryPoolManager theManager(mAllocator);
			MemoryBuffer theTempBuf( &theManager );
			theTempBuf.clear();
			inArgs.cooker->cookTriangleMesh( meshDesc, theTempBuf );

			writeBuffer( inWriter, inTempBuffer, 16, theTempBuf.mBuffer, theTempBuf.mWriteOffset, "CookedData", writeDatatype<PxU8> );
		}

		delete []meshDesc.materialIndices.data;
	}

	PxRepXObject PxBVH34TriangleMeshRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* /*inCollection*/ )
	{
		//We can't do a simple inverse; we *have* to cook data to get a mesh.
		PxTriangleMeshDesc theDesc;
		readStridedBufferProperty<PxVec3>( inReader, "points", theDesc.points, inAllocator);
		readStridedBufferProperty<Triangle<PxU32> >( inReader, "triangles", theDesc.triangles, inAllocator);
		PxU32 triCount;
		readStridedBufferProperty<PxMaterialTableIndex>( inReader, "materialIndices", theDesc.materialIndices, triCount, inAllocator);
		PxStridedData cookedData;
		cookedData.stride = sizeof(PxU8);
		PxU32 dataSize;
		readStridedBufferProperty<PxU8>( inReader, "CookedData", cookedData, dataSize, inAllocator);

		TMemoryPoolManager theManager(inAllocator.getAllocator());	
		MemoryBuffer theTempBuf( &theManager );

//		PxTriangleMesh* theMesh = NULL;			
		PxBVH34TriangleMesh* theMesh = NULL;			

		if(dataSize != 0)
		{				
			theTempBuf.write(cookedData.data, dataSize*sizeof(PxU8));
//			theMesh = inArgs.physics.createTriangleMesh( theTempBuf );
			theMesh = static_cast<PxBVH34TriangleMesh*>(inArgs.physics.createTriangleMesh( theTempBuf ));
		}

		if(theMesh == NULL)
		{
			PX_ASSERT(inArgs.cooker);
			theTempBuf.clear();

			{
				PxCookingParams params = inArgs.cooker->getParams();
				params.midphaseDesc = PxMeshMidPhase::eBVH34;
				inArgs.cooker->setParams(params);
			}

			inArgs.cooker->cookTriangleMesh( theDesc, theTempBuf );
//			theMesh = inArgs.physics.createTriangleMesh( theTempBuf );
			theMesh = static_cast<PxBVH34TriangleMesh*>(inArgs.physics.createTriangleMesh( theTempBuf ));
		}					

		return PxCreateRepXObject(theMesh);
	}


	//*************************************************************
	//	Actual RepXSerializer implementations for PxHeightField
	//*************************************************************
	void PxHeightFieldRepXSerializer::objectToFileImpl( const PxHeightField* inHeightField, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{
		PxHeightFieldDesc theDesc;

		theDesc.nbRows					= inHeightField->getNbRows();
		theDesc.nbColumns				= inHeightField->getNbColumns();
		theDesc.format					= inHeightField->getFormat();
		theDesc.samples.stride			= inHeightField->getSampleStride();
		theDesc.samples.data			= NULL;
		theDesc.thickness				= inHeightField->getThickness();
		theDesc.convexEdgeThreshold		= inHeightField->getConvexEdgeThreshold();
		theDesc.flags					= inHeightField->getFlags();

		PxU32 theCellCount = inHeightField->getNbRows() * inHeightField->getNbColumns();
		PxU32 theSampleStride = sizeof( PxHeightFieldSample );
		PxU32 theSampleBufSize = theCellCount * theSampleStride;
		PxHeightFieldSample* theSamples = reinterpret_cast< PxHeightFieldSample*> ( inTempBuffer.mManager->allocate( theSampleBufSize ) );
		inHeightField->saveCells( theSamples, theSampleBufSize );
		theDesc.samples.data = theSamples;
		writeAllProperties( &theDesc, inWriter, inTempBuffer, *inCollection );
		writeStridedBufferProperty<PxHeightFieldSample>( inWriter, inTempBuffer, "samples", theDesc.samples, theDesc.nbRows * theDesc.nbColumns, 6, writeHeightFieldSample);
		inTempBuffer.mManager->deallocate( reinterpret_cast<PxU8*>(theSamples) );
	}

	PxRepXObject PxHeightFieldRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* inCollection )
	{
		PX_ASSERT(inArgs.cooker);
		PxHeightFieldDesc theDesc;
		readAllProperties( inArgs, inReader, &theDesc, inAllocator, *inCollection );
		//Now read the data...
		PxU32 count = 0; //ignored becaues numRows and numColumns tells the story
		readStridedBufferProperty<PxHeightFieldSample>( inReader, "samples", theDesc.samples, count, inAllocator);
		PxHeightField* retval = inArgs.cooker->createHeightField( theDesc, inArgs.physics.getPhysicsInsertionCallback() );
		return PxCreateRepXObject(retval);
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxConvexMesh
	//*************************************************************
	void PxConvexMeshRepXSerializer::objectToFileImpl( const PxConvexMesh* mesh, PxCollection* /*inCollection*/, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& inArgs )
	{
		writeBuffer( inWriter, inTempBuffer, 2, mesh->getVertices(), mesh->getNbVertices(), "points", writePxVec3 );

		if(inArgs.cooker != NULL)
		{
			//Cache cooked Data
			PxConvexMeshDesc theDesc;
			theDesc.points.data = mesh->getVertices();
			theDesc.points.stride = sizeof(PxVec3);
			theDesc.points.count = mesh->getNbVertices();

			theDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
			TMemoryPoolManager theManager(mAllocator);
			MemoryBuffer theTempBuf( &theManager );
			inArgs.cooker->cookConvexMesh( theDesc, theTempBuf );

			writeBuffer( inWriter, inTempBuffer, 16, theTempBuf.mBuffer, theTempBuf.mWriteOffset, "CookedData", writeDatatype<PxU8> );
		}

	}

	//Conversion from scene object to descriptor.
	PxRepXObject PxConvexMeshRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* /*inCollection*/)
	{
		PxConvexMeshDesc theDesc;
		readStridedBufferProperty<PxVec3>( inReader, "points", theDesc.points, inAllocator);
		theDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

		PxStridedData cookedData;
		cookedData.stride = sizeof(PxU8);
		PxU32 dataSize;
		readStridedBufferProperty<PxU8>( inReader, "CookedData", cookedData, dataSize, inAllocator);

		TMemoryPoolManager theManager(inAllocator.getAllocator());
		MemoryBuffer theTempBuf( &theManager );

		PxConvexMesh* theMesh = NULL;

		if(dataSize != 0)
		{
			theTempBuf.write(cookedData.data, dataSize*sizeof(PxU8));
			theMesh = inArgs.physics.createConvexMesh( theTempBuf );
		}

		if(theMesh == NULL)
		{
			PX_ASSERT(inArgs.cooker);
			theTempBuf.clear();

			inArgs.cooker->cookConvexMesh( theDesc, theTempBuf );
			theMesh = inArgs.physics.createConvexMesh( theTempBuf );
		}					

		return PxCreateRepXObject(theMesh);
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxRigidStatic
	//*************************************************************
	PxRigidStatic* PxRigidStaticRepXSerializer::allocateObject( PxRepXInstantiationArgs& inArgs )
	{
		return inArgs.physics.createRigidStatic( PxTransform(PxIdentity) );
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxRigidDynamic
	//*************************************************************
	PxRigidDynamic* PxRigidDynamicRepXSerializer::allocateObject( PxRepXInstantiationArgs& inArgs )
	{
		return inArgs.physics.createRigidDynamic( PxTransform(PxIdentity) );
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxArticulation
	//*************************************************************
	void PxArticulationRepXSerializer::objectToFileImpl( const PxArticulation* inObj, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{
		TNameStack nameStack( inTempBuffer.mManager->mWrapper );
		Sn::TArticulationLinkLinkMap linkMap( inTempBuffer.mManager->mWrapper );
		RepXVisitorWriter<PxArticulation> writer( nameStack, inWriter, inObj, inTempBuffer, *inCollection, &linkMap );
		RepXPropertyFilter<RepXVisitorWriter<PxArticulation> > theOp( writer );
		visitAllProperties<PxArticulation>( theOp );
	}
	PxArticulation* PxArticulationRepXSerializer::allocateObject( PxRepXInstantiationArgs& inArgs ) { return inArgs.physics.createArticulation(); }

	//*************************************************************
	//	Actual RepXSerializer implementations for PxAggregate
	//*************************************************************
	void PxAggregateRepXSerializer::objectToFileImpl( const PxAggregate* data, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{
		PxArticulationLink *link = NULL;
		inWriter.addAndGotoChild( "Actors" );
		for(PxU32 i = 0; i < data->getNbActors(); ++i)
		{
			PxActor* actor;

			if(data->getActors(&actor, 1, i))
			{
				link = actor->is<PxArticulationLink>();
			}

			if(link && !link->getInboundJoint() )
			{
				writeProperty( inWriter, *inCollection, inTempBuffer, "PxArticulationRef",  &link->getArticulation());			
			}
			else if( !link )
			{
				PxSerialObjectId theId = 0;

				theId = inCollection->getId( *actor );
				if( theId == 0 )
					theId = PX_PROFILE_POINTER_TO_U64( actor );

				writeProperty( inWriter, *inCollection, inTempBuffer, "PxActorRef", theId );			
			}
		}

		inWriter.leaveChild( );

		writeProperty( inWriter, *inCollection, inTempBuffer, "NumActors", data->getNbActors() );
		writeProperty( inWriter, *inCollection, inTempBuffer, "MaxNbActors", data->getMaxNbActors() );
		writeProperty( inWriter, *inCollection, inTempBuffer, "SelfCollision", data->getSelfCollision() );

		writeAllProperties( data, inWriter, inTempBuffer, *inCollection );
	}

	PxRepXObject PxAggregateRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* inCollection )
	{
		PxU32 numActors;
		readProperty( inReader, "NumActors", numActors );
		PxU32 maxNbActors;
		readProperty( inReader, "MaxNbActors", maxNbActors );
		
		bool selfCollision;
		bool ret = readProperty( inReader, "SelfCollision", selfCollision );

		PxAggregate* theAggregate = inArgs.physics.createAggregate(maxNbActors, selfCollision);
		ret &= readAllProperties( inArgs, inReader, theAggregate, inAllocator, *inCollection );

		inReader.pushCurrentContext();
		if ( inReader.gotoChild( "Actors" ) )
		{
			inReader.pushCurrentContext();
			for( bool matSuccess = inReader.gotoFirstChild(); matSuccess;
				matSuccess = inReader.gotoNextSibling() )
			{
				const char* actorType = inReader.getCurrentItemName();
				if ( 0 == physx::shdfnd::stricmp( actorType, "PxActorRef" ) ) 
				{
					PxActor *actor = NULL;
					ret &= readReference<PxActor>( inReader, *inCollection, actor );

					if(actor)
					{
						PxScene *currScene = actor->getScene();
						if(currScene)
						{
							currScene->removeActor(*actor);
						}
						theAggregate->addActor(*actor);
					}
				}
				else if ( 0 == physx::shdfnd::stricmp( actorType, "PxArticulationRef" ) ) 
				{
					PxArticulation* articulation = NULL;
					ret &= readReference<PxArticulation>( inReader, *inCollection, articulation );
					if(articulation)
					{
						PxScene *currScene = articulation->getScene();
						if(currScene)
						{
							currScene->removeArticulation(*articulation);
						}
						theAggregate->addArticulation(*articulation);
					}
				}	
			}
			inReader.popCurrentContext();
			inReader.leaveChild();
		}
		inReader.popCurrentContext();

		return ret ? PxCreateRepXObject(theAggregate) : PxRepXObject();
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxClothFabric
	//*************************************************************
#if PX_USE_CLOTH_API

	void writeFabricPhase( PxOutputStream& stream, const PxClothFabricPhase& phase )
	{
		const PxU32ToName* conv = PxEnumTraits<PxClothFabricPhaseType::Enum>().NameConversion;
		for ( ; conv->mName != NULL; ++conv )
			if ( conv->mValue == PxU32(phase.phaseType) ) 
				stream << conv->mName;
		stream << " " << phase.setIndex;
	}

	namespace Sn {
		template<> struct StrToImpl<PxClothFabricPhase> 
		{
			void strto( PxClothFabricPhase& datatype, const char*& ioData )
			{
				char buffer[512];
				eatwhite( ioData );
				nullTerminateWhite(ioData, buffer, 512 );

				const PxU32ToName* conv = PxEnumTraits<PxClothFabricPhaseType::Enum>().NameConversion;
				for ( ; conv->mName != NULL; ++conv )
					if ( 0 == physx::shdfnd::stricmp( buffer, conv->mName ) )
						datatype = static_cast<PxClothFabricPhaseType::Enum>( conv->mValue );

				StrToImpl<PxU32>().strto( datatype.setIndex, ioData );
			}
		};
	}

	void PxClothFabricRepXSerializer::objectToFileImpl( const PxClothFabric* data, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{

		PxProfileAllocatorWrapper& wrapper( inTempBuffer.mManager->getWrapper() );
		PxU32 numParticles = data->getNbParticles();
		PxU32 numPhases = data->getNbPhases();
		PxU32 numRestvalues = data->getNbRestvalues();
		PxU32 numSets = data->getNbSets();
		PxU32 numIndices = data->getNbParticleIndices();
		PxU32 numTethers = data->getNbTethers();

		PxProfileArray<PxU8> dataBuffer( wrapper );
		PxU32 numInts = PxMax(PxMax(numIndices, numRestvalues), numTethers);
		dataBuffer.resize( PxU32(PxMax( PxMax( numInts * sizeof(PxU32), numTethers * sizeof(PxReal) ), 
			numPhases * sizeof(PxClothFabricPhase)) ) );

		PxClothFabricPhase* phasePtr( reinterpret_cast<PxClothFabricPhase*>( dataBuffer.begin() ) );
		PxU32* indexPtr( reinterpret_cast<PxU32*>( dataBuffer.begin() ) );

		writeProperty( inWriter, *inCollection, inTempBuffer, "NbParticles", numParticles );

		data->getPhases( phasePtr, numPhases );
		writeBuffer( inWriter, inTempBuffer, 18, phasePtr, PtrAccess<PxClothFabricPhase>, numPhases, "Phases", writeFabricPhase );

		PX_COMPILE_TIME_ASSERT( sizeof( PxReal ) == sizeof( PxU32 ) );
		PxReal* realPtr = reinterpret_cast< PxReal* >( indexPtr );
		data->getRestvalues( realPtr, numRestvalues );
		writeBuffer( inWriter, inTempBuffer, 18, realPtr, PtrAccess<PxReal>, numRestvalues, "Restvalues", BasicDatatypeWrite<PxReal> );

		data->getSets( indexPtr, numSets );
		writeBuffer( inWriter, inTempBuffer, 18, indexPtr, PtrAccess<PxU32>, numSets, "Sets", BasicDatatypeWrite<PxU32> );

		data->getParticleIndices( indexPtr, numIndices );
		writeBuffer( inWriter, inTempBuffer, 18, indexPtr, PtrAccess<PxU32>, numIndices, "ParticleIndices", BasicDatatypeWrite<PxU32> );

		data->getTetherAnchors( indexPtr, numTethers );
		writeBuffer( inWriter, inTempBuffer, 18, indexPtr, PtrAccess<PxU32>, numTethers, "TetherAnchors", BasicDatatypeWrite<PxU32> );

		data->getTetherLengths( realPtr, numTethers );
		writeBuffer( inWriter, inTempBuffer, 18, realPtr, PtrAccess<PxReal>, numTethers, "TetherLengths", BasicDatatypeWrite<PxReal> );
	}

	//Conversion from scene object to descriptor.
	PxRepXObject PxClothFabricRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* /*inCollection*/)
	{
		PxU32 strideIgnored = 0;

		PxClothFabricDesc desc;	

		readProperty( inReader, "NbParticles", desc.nbParticles );

		readStridedBufferProperty<PxClothFabricPhase>( inReader, "Phases", const_cast<PxClothFabricPhase*&>(desc.phases), strideIgnored, desc.nbPhases, inAllocator );

		PxU32 numRestvalues = 0;
		readStridedBufferProperty<PxF32>( inReader, "Restvalues", const_cast<PxF32*&>(desc.restvalues), strideIgnored, numRestvalues, inAllocator );

		readStridedBufferProperty<PxU32>( inReader, "Sets", const_cast<PxU32*&>(desc.sets), strideIgnored, desc.nbSets, inAllocator );

		PxU32 numIndices = 0;
		readStridedBufferProperty<PxU32>( inReader, "ParticleIndices", const_cast<PxU32*&>(desc.indices), strideIgnored, numIndices, inAllocator );

		readStridedBufferProperty<PxU32>( inReader, "TetherAnchors", const_cast<PxU32*&>(desc.tetherAnchors), strideIgnored, desc.nbTethers, inAllocator );
		readStridedBufferProperty<PxF32>( inReader, "TetherLengths", const_cast<PxF32*&>(desc.tetherLengths), strideIgnored, desc.nbTethers, inAllocator );

		PxClothFabric* newFabric = inArgs.physics.createClothFabric( desc );

		PX_ASSERT( newFabric );
		return PxCreateRepXObject(newFabric);
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxCloth
	//*************************************************************
	void clothParticleWriter( PxOutputStream& stream, const PxClothParticle& particle )
	{
		stream << particle.pos << " " << particle.invWeight;
	}

	namespace Sn {
		template<> struct StrToImpl<PxClothParticle> 
		{
			void strto( PxClothParticle& datatype, const char*& ioData )
			{
				StrToImpl<PxF32>().strto( datatype.pos[0], ioData );
				StrToImpl<PxF32>().strto( datatype.pos[1], ioData );
				StrToImpl<PxF32>().strto( datatype.pos[2], ioData );
				StrToImpl<PxF32>().strto( datatype.invWeight, ioData );
			}
		};

		template<> struct StrToImpl<PxClothCollisionSphere> 
		{
			void strto( PxClothCollisionSphere& datatype, const char*& ioData )
			{
				StrToImpl<PxF32>().strto( datatype.pos[0], ioData );
				StrToImpl<PxF32>().strto( datatype.pos[1], ioData );
				StrToImpl<PxF32>().strto( datatype.pos[2], ioData );
				StrToImpl<PxF32>().strto( datatype.radius, ioData );
			}
		};

		template<> struct StrToImpl<PxClothCollisionPlane> 
		{
			void strto( PxClothCollisionPlane& datatype, const char*& ioData )
			{
				StrToImpl<PxF32>().strto( datatype.normal[0], ioData );
				StrToImpl<PxF32>().strto( datatype.normal[1], ioData );
				StrToImpl<PxF32>().strto( datatype.normal[2], ioData );
				StrToImpl<PxF32>().strto( datatype.distance, ioData );
			}
		};

		template<> struct StrToImpl<PxClothCollisionTriangle> 
		{
			void strto( PxClothCollisionTriangle& datatype, const char*& ioData )
			{
				StrToImpl<PxVec3>().strto( datatype.vertex0, ioData );
				StrToImpl<PxVec3>().strto( datatype.vertex1, ioData );
				StrToImpl<PxVec3>().strto( datatype.vertex2, ioData );
			}
		};

		template<> struct StrToImpl<PxVec4> 
		{
			void strto( PxVec4& datatype, const char*& ioData )
			{
				StrToImpl<PxF32>().strto( datatype.x, ioData );
				StrToImpl<PxF32>().strto( datatype.y, ioData );
				StrToImpl<PxF32>().strto( datatype.z, ioData );
				StrToImpl<PxF32>().strto( datatype.w, ioData );
			}
		};

		template<> struct StrToImpl<PxClothParticleMotionConstraint> 
		{
			void strto( PxClothParticleMotionConstraint& datatype, const char*& ioData )
			{
				StrToImpl<PxVec3>().strto( datatype.pos, ioData );
				StrToImpl<PxF32>().strto( datatype.radius, ioData );
			}
		};
		
		template<> struct StrToImpl<PxClothParticleSeparationConstraint> 
		{
			void strto( PxClothParticleSeparationConstraint& datatype, const char*& ioData )
			{
				StrToImpl<PxVec3>().strto( datatype.pos, ioData );
				StrToImpl<PxF32>().strto( datatype.radius, ioData );
			}
		};
		
	}

	void clothSphereWriter( PxOutputStream& stream, const PxClothCollisionSphere& sphere )
	{
		stream << sphere.pos << " " << sphere.radius;
	}

	void clothPlaneWriter( PxOutputStream& stream, const PxClothCollisionPlane& plane )
	{
		stream << plane.normal << " " << plane.distance;
	}

	void clothTriangleWriter( PxOutputStream& stream, const PxClothCollisionTriangle& triangle )
	{
		stream << triangle.vertex0 << " " << triangle.vertex1 << " " << triangle.vertex2;
	}

	void PxClothRepXSerializer::objectToFileImpl( const PxCloth* data, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{
		PxClothParticleData* readData( data->lockParticleData() );
		writeBuffer( inWriter, inTempBuffer, 4, readData->particles, PtrAccess<PxClothParticle>, data->getNbParticles(), "Particles", clothParticleWriter );
		readData->unlock();

		writeReference( inWriter, *inCollection, "Fabric", data->getFabric() );

		PxProfileArray<PxU8> dataBuffer( inTempBuffer.mManager->getWrapper() );

		// collision data
		const PxCloth& cloth = *data;
		PxU32 numSpheres = cloth.getNbCollisionSpheres();
		PxU32 numIndices = 2*cloth.getNbCollisionCapsules();
		PxU32 numPlanes = cloth.getNbCollisionPlanes();
		PxU32 numConvexes = cloth.getNbCollisionConvexes();
		PxU32 numTriangles = cloth.getNbCollisionTriangles();

		PxU32 sphereBytes = numSpheres * sizeof( PxClothCollisionSphere );
		PxU32 pairBytes = numIndices * sizeof( PxU32 );
		PxU32 planesBytes = numPlanes * sizeof(PxClothCollisionPlane);
		PxU32 convexBytes = numConvexes * sizeof(PxU32);
		PxU32 triangleBytes = numTriangles * sizeof(PxClothCollisionTriangle);

		dataBuffer.resize( sphereBytes + pairBytes + planesBytes + convexBytes + triangleBytes);
		PxClothCollisionSphere* spheresBuffer = reinterpret_cast<PxClothCollisionSphere*>( dataBuffer.begin() );
		PxU32* indexBuffer = reinterpret_cast<PxU32*>(spheresBuffer + numSpheres);
		PxClothCollisionPlane* planeBuffer = reinterpret_cast<PxClothCollisionPlane*>(indexBuffer + numIndices);
		PxU32* convexBuffer = reinterpret_cast<PxU32*>(planeBuffer + numPlanes);
		PxClothCollisionTriangle* trianglesBuffer = reinterpret_cast<PxClothCollisionTriangle*>(convexBuffer + numConvexes);

		data->getCollisionData( spheresBuffer, indexBuffer, planeBuffer, convexBuffer, trianglesBuffer );
		writeBuffer( inWriter, inTempBuffer, 4, spheresBuffer, PtrAccess<PxClothCollisionSphere>, numSpheres, "CollisionSpheres", clothSphereWriter );
		writeBuffer( inWriter, inTempBuffer, 18, indexBuffer, PtrAccess<PxU32>, numIndices, "CollisionSpherePairs", BasicDatatypeWrite<PxU32> );
		writeBuffer( inWriter, inTempBuffer, 4, planeBuffer, PtrAccess<PxClothCollisionPlane>, numPlanes, "CollisionPlanes", clothPlaneWriter );
		writeBuffer( inWriter, inTempBuffer, 18, convexBuffer, PtrAccess<PxU32>, numConvexes, "CollisionConvexMasks", BasicDatatypeWrite<PxU32> );
		writeBuffer( inWriter, inTempBuffer, 4, trianglesBuffer, PtrAccess<PxClothCollisionTriangle>, numTriangles, "CollisionTriangles", clothTriangleWriter );

		// particle accelerations
		PxU32 numParticleAccelerations = cloth.getNbParticleAccelerations();
		PxU32 sizeParticleAccelerations = sizeof(PxVec4)*numParticleAccelerations;
		if (dataBuffer.size() < sizeParticleAccelerations)
			dataBuffer.resize(sizeof(PxClothParticle)*numParticleAccelerations);

		PxVec4* particleAccelerations = reinterpret_cast<PxVec4*>(dataBuffer.begin());
		cloth.getParticleAccelerations(particleAccelerations);
		writeBuffer (inWriter, inTempBuffer, 4, reinterpret_cast<PxClothParticle*>(particleAccelerations), PtrAccess<PxClothParticle>, numParticleAccelerations, "ParticleAccelerations", clothParticleWriter );

		//self collision indices
		PxU32* particleCollisionIndices = reinterpret_cast<PxU32*>(dataBuffer.begin());
		PxU32 numSelfCollisionIndices = cloth.getNbSelfCollisionIndices();
		PxU32 sizeSelfCollisionIndices = sizeof(PxU32)*numSelfCollisionIndices;
		if (dataBuffer.size() < sizeSelfCollisionIndices)
			dataBuffer.resize(sizeSelfCollisionIndices);
		data->getSelfCollisionIndices( particleCollisionIndices );
		writeBuffer( inWriter, inTempBuffer, 18, particleCollisionIndices, PtrAccess<PxU32>, numSelfCollisionIndices, "SelfCollisionIndices", BasicDatatypeWrite<PxU32> );

		// motion constraints
		PxClothParticleMotionConstraint* motionConstraints = reinterpret_cast<PxClothParticleMotionConstraint*>(dataBuffer.begin());
		PxU32 numMotionConstraints = cloth.getNbMotionConstraints();
		PxU32 sizeMotionConstraints = sizeof(PxVec4)*numMotionConstraints;
		if (dataBuffer.size() < sizeMotionConstraints)
			dataBuffer.resize(sizeof(PxClothParticle)*numMotionConstraints);
		data->getMotionConstraints( motionConstraints );
		writeBuffer (inWriter, inTempBuffer, 4, reinterpret_cast<PxClothParticle*>(motionConstraints), PtrAccess<PxClothParticle>, numMotionConstraints, "MotionConstraints", clothParticleWriter );

		// separation positions
		PxU32 numSeparationConstraints= cloth.getNbSeparationConstraints();
		PxU32 restSeparationConstraints = sizeof(PxVec4)*numSeparationConstraints;
		if (dataBuffer.size() < restSeparationConstraints)
			dataBuffer.resize(sizeof(PxClothParticle)*numSeparationConstraints);
		PxClothParticleSeparationConstraint* separationConstraints = reinterpret_cast<PxClothParticleSeparationConstraint*>(dataBuffer.begin());
		cloth.getSeparationConstraints(separationConstraints);
		writeBuffer (inWriter, inTempBuffer, 4, reinterpret_cast<PxClothParticle*>(separationConstraints), PtrAccess<PxClothParticle>, numSeparationConstraints, "SeparationConstraints", clothParticleWriter );

		// rest positions
		PxU32 numRestPositions = cloth.getNbRestPositions();
		PxU32 restPositionSize = sizeof(PxVec4)*numRestPositions;
		if (dataBuffer.size() < restPositionSize)
			dataBuffer.resize(sizeof(PxClothParticle)*numRestPositions);

		PxVec4* restPositions = reinterpret_cast<PxVec4*>(dataBuffer.begin());
		cloth.getRestPositions(restPositions);
		writeBuffer (inWriter, inTempBuffer, 4, reinterpret_cast<PxClothParticle*>(restPositions), PtrAccess<PxClothParticle>, numRestPositions, "RestPositions", clothParticleWriter );

		// virtual particles
		PxU32 numVirtualParticles = data->getNbVirtualParticles();
		PxU32 numWeightTableEntries = data->getNbVirtualParticleWeights();

		PxU32 totalNeeded = static_cast<PxU32>( PxMax( numWeightTableEntries * sizeof( PxVec3 ), numVirtualParticles * 4  * sizeof( PxU32 ) ) );
		if ( dataBuffer.size() < totalNeeded )
			dataBuffer.resize( totalNeeded );

		PxVec3* weightTableEntries = reinterpret_cast<PxVec3*>( dataBuffer.begin() );
		data->getVirtualParticleWeights( weightTableEntries );
		writeBuffer( inWriter, inTempBuffer, 6, weightTableEntries, PtrAccess<PxVec3>, numWeightTableEntries, "VirtualParticleWeights", BasicDatatypeWrite<PxVec3> );
		PxU32* virtualParticles = reinterpret_cast<PxU32*>( dataBuffer.begin() );
		data->getVirtualParticles( virtualParticles );
		writeBuffer( inWriter, inTempBuffer, 18, virtualParticles, PtrAccess<PxU32>, numVirtualParticles * 4, "VirtualParticles", BasicDatatypeWrite<PxU32> );

		// Now write the rest of the object data that the meta data generator got.
		writeAllProperties( data, inWriter, inTempBuffer, *inCollection );
	}

	PxRepXObject PxClothRepXSerializer::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* inCollection )
	{
		PxU32 strideIgnored;
		PxU32 numParticles;
		PxClothParticle* particles = NULL;

		PxU32 numSpheres;
		PxClothCollisionSphere* spheres = NULL;
		PxU32 numCapsules;
		PxU32* capsules = NULL;
		PxU32 numPlanes;
		PxClothCollisionPlane* planes = NULL;
		PxU32 numConvexMasks;
		PxU32* convexMasks = NULL;
		PxU32 numTriangles;
		PxClothCollisionTriangle* triangles = NULL;

		PxVec4* particleAccelerations = NULL;
		PxU32 numParticleAccelerations = 0;
		PxU32* selfCollisionIndices = NULL;
		PxU32 numSelfCollisionIndices = 0;
		PxClothParticleMotionConstraint* particleMotionConstraint = NULL;
		PxU32 numParticleMotionConstraint = 0;
		PxClothParticleSeparationConstraint* separationConstraints = NULL;
		PxU32 numSeparationConstraints = 0;

		PxVec4* restPositions = NULL;
		PxU32 numRestPositions;
		PxU32 numVirtualParticleWeights;
		PxU32 numParticleIndices = 0;
		PxVec3* virtualParticleWeights = NULL;
		PxU32* virtualParticles = NULL;
		PxClothFlags flags;
		PxClothFabric* fabric = NULL;

		readReference<PxClothFabric>( inReader, *inCollection, "Fabric", fabric );

		if ( fabric == NULL )
			return PxRepXObject();

		readStridedBufferProperty<PxClothParticle>( inReader, "Particles", particles, strideIgnored, numParticles, inAllocator );

		readStridedBufferProperty<PxClothCollisionSphere>( inReader, "CollisionSpheres", spheres, strideIgnored, numSpheres, inAllocator );
		readStridedBufferProperty<PxU32>( inReader, "CollisionSpherePairs", capsules, strideIgnored, numCapsules, inAllocator );
		readStridedBufferProperty<PxClothCollisionPlane>( inReader, "CollisionPlanes", planes, strideIgnored, numPlanes, inAllocator );
		readStridedBufferProperty<PxU32>( inReader, "CollisionConvexMasks", convexMasks, strideIgnored, numConvexMasks, inAllocator );
		readStridedBufferProperty<PxClothCollisionTriangle>( inReader, "CollisionTriangles", triangles, strideIgnored, numTriangles, inAllocator );

		readFlagsProperty( inReader, inAllocator, "ClothFlags", PxEnumTraits<PxClothFlag::Enum>().NameConversion, flags );

		readStridedBufferProperty<PxVec4>( inReader, "ParticleAccelerations", particleAccelerations, strideIgnored, numParticleAccelerations, inAllocator );
		readStridedBufferProperty<PxU32>( inReader, "SelfCollisionIndices", selfCollisionIndices, strideIgnored, numSelfCollisionIndices, inAllocator );
		readStridedBufferProperty<PxClothParticleMotionConstraint>( inReader, "MotionConstraints", particleMotionConstraint, strideIgnored, numParticleMotionConstraint, inAllocator );
		readStridedBufferProperty<PxClothParticleSeparationConstraint>( inReader, "SeparationConstraints", separationConstraints, strideIgnored, numSeparationConstraints, inAllocator );
		readStridedBufferProperty<PxVec4>( inReader, "RestPositions", restPositions, strideIgnored, numRestPositions, inAllocator );

		readStridedBufferProperty<PxVec3>( inReader, "VirtualParticleWeights", virtualParticleWeights, strideIgnored, numVirtualParticleWeights, inAllocator );
		readStridedBufferProperty<PxU32>( inReader, "VirtualParticles", virtualParticles, strideIgnored, numParticleIndices, inAllocator );

		PxTransform initialPose( PxIdentity );
		PxCloth* cloth = inArgs.physics.createCloth( initialPose, *fabric, reinterpret_cast<PxClothParticle*>( particles ), flags );
		bool ret = readAllProperties( inArgs, inReader, cloth, inAllocator, *inCollection );

		if( numSelfCollisionIndices )
		{
			cloth->setSelfCollisionIndices( selfCollisionIndices, numSelfCollisionIndices );
		}
		if( numParticleMotionConstraint )
		{
			cloth->setMotionConstraints( particleMotionConstraint );
		}
		if( numSeparationConstraints )
		{
			cloth->setSeparationConstraints( separationConstraints );
		}
		if( numParticleAccelerations )
		{
			cloth->setParticleAccelerations(particleAccelerations);
		}

		cloth->setCollisionSpheres(spheres, numSpheres);
		while(numCapsules > 0)
		{
			PxU32 first = *capsules++, second = *capsules++;
			cloth->addCollisionCapsule(first, second);
			numCapsules -= 2;
		}

		cloth->setCollisionPlanes(planes, numPlanes);
		while(numConvexMasks-- > 0)
		{
			cloth->addCollisionConvex(*convexMasks++);
		}

		cloth->setCollisionTriangles(triangles, numTriangles);

		if (numRestPositions)
			cloth->setRestPositions(restPositions);

		PxU32 numVirtualParticles = numParticleIndices /4;
		if ( numVirtualParticles && numVirtualParticleWeights )
		{
			cloth->setVirtualParticles( numVirtualParticles, reinterpret_cast<PxU32*>( virtualParticles ), 
				numVirtualParticleWeights, reinterpret_cast<PxVec3*>( virtualParticleWeights ) );
		}

		return ret ? PxCreateRepXObject(cloth) : PxRepXObject();
	}
#endif // PX_USE_CLOTH_API
	
#if PX_USE_PARTICLE_SYSTEM_API
	template<typename TParticleType>
	inline TParticleType* createParticles( PxPhysics& physics, PxU32 maxParticles, bool perParticleRestOffset )
	{
		PX_UNUSED(physics);
		PX_UNUSED(maxParticles);
		PX_UNUSED(perParticleRestOffset);
		return NULL;
	}

	template<>
	inline PxParticleSystem* createParticles<PxParticleSystem>(PxPhysics& physics, PxU32 maxParticles, bool perParticleRestOffset)
	{
		return physics.createParticleSystem( maxParticles, perParticleRestOffset );
	}
	
	template<>
	inline PxParticleFluid* createParticles<PxParticleFluid>(PxPhysics& physics, PxU32 maxParticles, bool perParticleRestOffset)
	{
		return physics.createParticleFluid( maxParticles, perParticleRestOffset );
	}

	//*************************************************************
	//	Actual RepXSerializer implementations for PxParticleSystem & PxParticleFluid
	//*************************************************************
	template<typename TParticleType>
	void PxParticleRepXSerializer<TParticleType>::objectToFileImpl( const TParticleType* data, PxCollection* inCollection, XmlWriter& inWriter, MemoryBuffer& inTempBuffer, PxRepXInstantiationArgs& /*inArgs*/)
	{
		PxParticleReadData* readData( const_cast<TParticleType*>( data )->lockParticleReadData() );
		writeProperty( inWriter, *inCollection, inTempBuffer, "NbParticles",			readData->nbValidParticles );
		writeProperty( inWriter, *inCollection, inTempBuffer, "ValidParticleRange",	readData->validParticleRange );
		PxParticleReadDataFlags readFlags(data->getParticleReadDataFlags());

		if(readData->validParticleRange > 0)
		{
			writeBuffer( inWriter, inTempBuffer, 8 , readData->validParticleBitmap, PtrAccess<PxU32>, ((readData->validParticleRange-1) >> 5) + 1 ,"ValidParticleBitmap",	BasicDatatypeWrite<PxU32> );

			writeStridedBufferProperty<PxVec3>( inWriter, inTempBuffer, "Positions", readData->positionBuffer, readData->nbValidParticles, 6, writePxVec3);

			if(readFlags & PxParticleReadDataFlag::eVELOCITY_BUFFER)
			{
				writeStridedBufferProperty<PxVec3>( inWriter, inTempBuffer, "Velocities", readData->velocityBuffer, readData->nbValidParticles, 6, writePxVec3);
			}
			if(readFlags & PxParticleReadDataFlag::eREST_OFFSET_BUFFER)
			{
				writeStridedBufferProperty<PxF32>( inWriter, inTempBuffer, "RestOffsets", readData->restOffsetBuffer, readData->nbValidParticles, 6, BasicDatatypeWrite<PxF32>);
			}
			if(readFlags & PxParticleReadDataFlag::eFLAGS_BUFFER)
			{
				writeStridedFlagsProperty<PxParticleFlags>( inWriter, inTempBuffer, "Flags", readData->flagsBuffer, readData->nbValidParticles, 6, PxEnumTraits<PxParticleFlag::Enum>().NameConversion);
			}
		}
		readData->unlock();

		writeProperty( inWriter, *inCollection, inTempBuffer, "MaxParticles", data->getMaxParticles() );	

		PxParticleBaseFlags baseFlags(data->getParticleBaseFlags());
		writeFlagsProperty( inWriter, inTempBuffer, "ParticleBaseFlags", baseFlags, PxEnumTraits<PxParticleBaseFlag::Enum>().NameConversion );	

		writeFlagsProperty( inWriter, inTempBuffer, "ParticleReadDataFlags", readFlags, PxEnumTraits<PxParticleReadDataFlag::Enum>().NameConversion );	

		PxVec3	normal;
		PxReal	distance;
		data->getProjectionPlane(normal, distance);
		PxMetaDataPlane plane(normal, distance);
		writeProperty( inWriter, *inCollection, inTempBuffer, "ProjectionPlane",			plane);		

		writeAllProperties( data, inWriter, inTempBuffer, *inCollection );
	}	

	template<typename TParticleType>
	PxRepXObject PxParticleRepXSerializer<TParticleType>::fileToObject( XmlReader& inReader, XmlMemoryAllocator& inAllocator, PxRepXInstantiationArgs& inArgs, PxCollection* inCollection )
	{
		PxU32 strideIgnored = 0;
		PxU32 numParticles = 0;
		readProperty( inReader, "NbParticles", numParticles );

		PxU32 validParticleRange = 0;
		readProperty( inReader, "ValidParticleRange", validParticleRange );

		PxU32 numWrite;
		PxU32* tempValidParticleBitmap = NULL;
		readStridedBufferProperty<PxU32>( inReader, "ValidParticleBitmap", tempValidParticleBitmap, strideIgnored, numWrite, inAllocator );
		PxU32 *validParticleBitmap = reinterpret_cast<PxU32*>( tempValidParticleBitmap );

		PxVec3* tempPosBuf = NULL;
		readStridedBufferProperty<PxVec3>( inReader, "Positions", tempPosBuf, strideIgnored, numWrite, inAllocator );
		PxStrideIterator<const PxVec3> posBuffer(reinterpret_cast<const PxVec3*> (tempPosBuf));

		PxVec3* tempVelBuf = NULL;
		readStridedBufferProperty<PxVec3>( inReader, "Velocities", tempVelBuf, strideIgnored, numWrite, inAllocator );
		PxStrideIterator<const PxVec3> velBuffer(reinterpret_cast<const PxVec3*> (tempVelBuf));

		PxF32* tempRestBuf = NULL;
		readStridedBufferProperty<PxF32>( inReader, "RestOffsets", tempRestBuf, strideIgnored, numWrite, inAllocator );
		PxStrideIterator<const PxF32> restBuffer(reinterpret_cast<const PxF32*> (tempRestBuf));

		PxU32* tempFlagBuf = NULL;
		readStridedFlagsProperty<PxU32>( inReader, "Flags", tempFlagBuf, strideIgnored, numWrite, inAllocator , PxEnumTraits<PxParticleFlag::Enum>().NameConversion);
		PxStrideIterator<const PxU32> flagBuffer(reinterpret_cast<const PxU32*> (tempFlagBuf));

		Ps::Array<PxU32>	validIndexBuf;
		Ps::Array<PxVec3>	validPosBuf, validVelBuf;
		Ps::Array<PxF32>	validRestBuf;
		Ps::Array<PxU32>	validFlagBuf;

		bool perParticleRestOffset = !!tempRestBuf;
		bool bVelBuff = !!tempVelBuf;
		bool bFlagBuff = !!tempFlagBuf;

		if (validParticleRange > 0)
		{
			for (PxU32 w = 0; w <= (validParticleRange-1) >> 5; w++)
			{
				for (PxU32 b = validParticleBitmap[w]; b; b &= b-1)
				{
					PxU32	index = (w<<5|physx::shdfnd::lowestSetBit(b));
					validIndexBuf.pushBack(index);
					validPosBuf.pushBack(posBuffer[index]);
					if(bVelBuff)
						validVelBuf.pushBack(velBuffer[index]);
					if(perParticleRestOffset)
						validRestBuf.pushBack(restBuffer[index]);
					if(bFlagBuff)
						validFlagBuf.pushBack(flagBuffer[index]);
				}			
			} 

			PX_ASSERT(validIndexBuf.size() == numParticles);

			PxU32 maxParticleNum;
			bool ret = readProperty( inReader, "MaxParticles", maxParticleNum );
			TParticleType* theParticle( createParticles<TParticleType>( inArgs.physics, maxParticleNum, perParticleRestOffset ) );
			PX_ASSERT( theParticle );
			ret &= readAllProperties( inArgs, inReader, theParticle, inAllocator, *inCollection );

			PxParticleBaseFlags baseFlags;
			ret &= readFlagsProperty( inReader, inAllocator, "ParticleBaseFlags", PxEnumTraits<PxParticleBaseFlag::Enum>().NameConversion, baseFlags );

			PxU32 flagData = 1;
			for(PxU32 i = 0; i < 16; i++)
			{		
				flagData = PxU32(1 << i);
				if( !(flagData & PxParticleBaseFlag::ePER_PARTICLE_REST_OFFSET) && (!!(PxU32(baseFlags)  & flagData)) )
				{
					theParticle->setParticleBaseFlag(PxParticleBaseFlag::Enum(flagData), true);
				}					
			}

			PxParticleReadDataFlags readFlags;
			ret &= readFlagsProperty( inReader, inAllocator, "ParticleReadDataFlags", PxEnumTraits<PxParticleReadDataFlag::Enum>().NameConversion, readFlags );
			for(PxU32 i = 0; i < 16; i++)
			{
				flagData = PxU32(1 << i);
				if( !!(PxU32(readFlags)  & flagData) )
				{
					theParticle->setParticleReadDataFlag(PxParticleReadDataFlag::Enum(flagData), true);
				}					
			}

			PxParticleCreationData creationData;
			creationData.numParticles = numParticles;
			creationData.indexBuffer = PxStrideIterator<const PxU32>(validIndexBuf.begin());
			creationData.positionBuffer = PxStrideIterator<const PxVec3>(validPosBuf.begin());
			if(bVelBuff)
				creationData.velocityBuffer = PxStrideIterator<const PxVec3>(validVelBuf.begin());
			if(perParticleRestOffset)
				creationData.restOffsetBuffer = PxStrideIterator<const PxF32>(validRestBuf.begin());
			if(bFlagBuff)
				creationData.flagBuffer = PxStrideIterator<const PxU32>(validFlagBuf.begin());

			theParticle->createParticles(creationData);	

			return ret ? PxCreateRepXObject(theParticle) : PxRepXObject();
		}
		else
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, 
				"PxSerialization::createCollectionFromXml: PxParticleRepXSerializer: "
				"Xml field \"ValidParticleRange\" is zero!");
			return PxRepXObject();
		}
	}
	// explicit instantiations
	template struct PxParticleRepXSerializer<PxParticleSystem>;
	template struct PxParticleRepXSerializer<PxParticleFluid>;	
#endif // #if PX_USE_PARTICLE_SYSTEM_API
}
