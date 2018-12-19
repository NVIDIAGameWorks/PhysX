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

#include "PxPhysXConfig.h"

#if PX_USE_CLOTH_API

#include "TestClothHelpers.h"

#include "geometry/PxSphereGeometry.h"
#include "cloth/PxCloth.h"
#include "cloth/PxClothParticleData.h"
#include "extensions/PxDefaultStreams.h"
#include "extensions/PxShapeExt.h"
#include "PxToolkit.h"
#include "SampleArray.h"
#include "PsHashSet.h"
#include "geometry/PxGeometryQuery.h"

bool Test::ClothHelpers::attachBorder(PxClothParticle* particles, PxU32 numParticles, PxBorderFlags borderFlag)
{
	// compute bounds in x and z
	PxBounds3 bounds = PxBounds3::empty();

	for(PxU32 i = 0; i < numParticles; i++)
		bounds.include(particles[i].pos);

	PxVec3 skin = bounds.getExtents() * 0.01f;
	bounds.minimum += skin;
	bounds.maximum -= skin;

	if (borderFlag & BORDER_LEFT)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.x <= bounds.minimum.x)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_RIGHT)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.x >= bounds.maximum.x)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_BOTTOM)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.y <= bounds.minimum.y)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_TOP)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.y >= bounds.maximum.y)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_NEAR)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.z <= bounds.minimum.z)
				particles[i].invWeight = 0.0f;
	}
	if (borderFlag & BORDER_FAR)
	{
		for (PxU32 i = 0; i < numParticles; i++)
			if (particles[i].pos.z >= bounds.maximum.z)
				particles[i].invWeight = 0.0f;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::attachBorder(PxCloth& cloth, PxBorderFlags borderFlag)
{
	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
    if (!particleData)
        return false;

	PxU32 numParticles = cloth.getNbParticles();
	PxClothParticle* particles = particleData->previousParticles;

	attachBorder(particles, numParticles, borderFlag);

	particleData->particles = 0;
	particleData->unlock();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::attachClothOverlapToShape(PxCloth& cloth, PxShape& shape,PxReal radius)
{
	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
	if (!particleData)
		return false;

	PxU32 numParticles = cloth.getNbParticles();
	PxClothParticle* particles = particleData->previousParticles;

	PxSphereGeometry sphere(radius);
	PxTransform position = PxTransform(PxIdentity);
	PxTransform pose = cloth.getGlobalPose();
    for (PxU32 i = 0; i < numParticles; i++)
	{
        // check if particle overlaps shape volume
        position.p = pose.transform(particles[i].pos);
		if (PxGeometryQuery::overlap(shape.getGeometry().any(), PxShapeExt::getGlobalPose(shape, *shape.getActor()), sphere, position))
			particles[i].invWeight = 0.0f;
    }

	particleData->particles = 0;
    particleData->unlock();

    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::createCollisionCapsule(
	const PxTransform &pose,
	const PxVec3 &center0, PxReal r0, const PxVec3 &center1, PxReal r1, 
	SampleArray<PxClothCollisionSphere> &spheres, SampleArray<PxU32> &indexPairs)
{
	PxTransform invPose = pose.getInverse();

	spheres.resize(2);

	spheres[0].pos = invPose.transform(center0);
	spheres[0].radius = r0;
	spheres[1].pos = invPose.transform(center1);
	spheres[1].radius = r1;

	indexPairs.resize(2);
	indexPairs[0] = 0;
	indexPairs[1] = 1;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
PxClothMeshDesc Test::ClothHelpers::createMeshGrid(PxVec3 dirU, PxVec3 dirV, PxU32 numU, PxU32 numV,
	SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords)
{
	int numVertices = numU * numV;
	int numQuads = (numU-1) * (numV-1);

	vertices.resize(numVertices);
	indices.resize(numQuads * 4);
	texcoords.resize(numVertices);

	// fill in point data
	PxReal scaleU = 1 / PxReal(numU-1);
	PxReal scaleV = 1 / PxReal(numV-1);

	PxVec4* posIt = vertices.begin();
	PxVec2* texIt = texcoords.begin();
	for (PxU32 i = 0; i < numV; i++) 
	{
		PxReal texV = i * scaleV;
		PxVec3 posV = (texV - 0.5f) * dirV;
		for (PxU32 j = 0; j < numU; j++) 
		{
			PxReal texU = j * scaleU;
			PxVec3 posU = (texU - 0.5f) * dirU;
			*posIt++ = PxVec4(posU + posV, 1.0f);
			*texIt++ = PxVec2(texU, 1.0f - texV);
		}
	}

    // fill in quad index data
	PxU32 *idxIt = indices.begin();
	for (PxU32 i = 0; i < numV-1; i++) 
	{
		for (PxU32 j = 0; j < numU-1; j++) 
		{
			PxU32 i0 = i * numU + j; 
			*idxIt++ = i0;
			*idxIt++ = i0 + 1;
			*idxIt++ = i0 + numU + 1;
			*idxIt++ = i0 + numU;
		}
	}

	PxClothMeshDesc meshDesc;

	// convert vertex array to PxBoundedData (desc.points)
	meshDesc.points.data = vertices.begin();
	meshDesc.points.count = static_cast<PxU32>(numVertices);
	meshDesc.points.stride = sizeof(PxVec4);

	meshDesc.invMasses.data = &vertices.begin()->w;
	meshDesc.invMasses.count = static_cast<PxU32>(numVertices);
	meshDesc.invMasses.stride = sizeof(PxVec4);

	// convert index array to PxBoundedData (desc.quads)
	meshDesc.quads.data = indices.begin();
	meshDesc.quads.count = static_cast<PxU32>(numQuads); 
	meshDesc.quads.stride = sizeof(PxU32) * 4; // <- stride per quad

	return meshDesc;
}


////////////////////////////////////////////////////////////////////////////////
// merge two meshes into a single mesh, works only for 4 stride vertex data
PxClothMeshDesc Test::ClothHelpers::mergeMeshDesc(PxClothMeshDesc &desc1, PxClothMeshDesc &desc2,  
	SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices,
	SampleArray<PxVec2>& texcoords1, SampleArray<PxVec2>& texcoords2, SampleArray<PxVec2>& texcoords)
{
	PxClothMeshDesc meshDesc;

	PxU32 numVertices = desc1.points.count + desc2.points.count; 
	vertices.resize(numVertices);
	memcpy(vertices.begin(), desc1.points.data, sizeof(PxVec4) * desc1.points.count);
	memcpy(vertices.begin()+desc1.points.count, desc2.points.data, sizeof(PxVec4) * desc2.points.count);

	indices.resize(desc1.quads.count * 4 + desc2.quads.count * 4 + desc1.triangles.count * 3 + desc2.triangles.count * 3);

	PxU32* indexBase = indices.begin();
	memcpy(indexBase, desc1.quads.data, sizeof(PxU32) * desc1.quads.count * 4);
	indexBase += desc1.quads.count * 4;

	memcpy(indexBase, desc2.quads.data, sizeof(PxU32) * desc2.quads.count * 4);
	for (PxU32 i = 0; i < desc2.quads.count * 4; i++)
		indexBase[i] += desc1.points.count;
	indexBase += desc2.quads.count * 4;

	memcpy(indexBase, desc1.triangles.data, sizeof(PxU32) * desc1.triangles.count * 3);
	indexBase += desc1.triangles.count * 3;
	
	memcpy(indexBase, desc2.triangles.data, sizeof(PxU32) * desc2.triangles.count * 3);
	for (PxU32 i = 0; i < desc2.triangles.count * 3; i++)
		indexBase[i] += desc1.points.count;

	texcoords.resize(desc1.points.count + desc2.points.count);
	memcpy(texcoords.begin(), texcoords1.begin(), sizeof(PxVec2) * desc1.points.count);
	memcpy(texcoords.begin()+desc1.points.count, texcoords2.begin(), sizeof(PxVec2) * desc2.points.count);

	meshDesc.points.count = numVertices;
	meshDesc.points.data = vertices.begin();
	meshDesc.points.stride = sizeof(PxVec4);

	meshDesc.invMasses.data = &vertices.begin()->w;
	meshDesc.invMasses.count = static_cast<PxU32>(numVertices);
	meshDesc.invMasses.stride = sizeof(PxVec4);

	// convert index array to PxBoundedData (desc.quads)
	PxU32 numQuads = desc1.quads.count + desc2.quads.count;
	meshDesc.quads.data = indices.begin();
	meshDesc.quads.count = static_cast<PxU32>(numQuads); 
	meshDesc.quads.stride = sizeof(PxU32) * 4; // <- stride per quad

	PxU32 numTriangles = desc1.triangles.count + desc2.triangles.count;
	meshDesc.triangles.data = indices.begin() + numQuads;
	meshDesc.triangles.count = static_cast<PxU32>(numTriangles); 
	meshDesc.triangles.stride = sizeof(PxU32) * 3; // <- stride per triangle

	return meshDesc;
}

////////////////////////////////////////////////////////////////////////////////
// remove duplicate vertices
PxClothMeshDesc Test::ClothHelpers::removeDuplicateVertices(PxClothMeshDesc &inMesh,  SampleArray<PxVec2> &inTexcoords,
	SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords)
{
	PxU32 numVertices = inMesh.points.count;
	SampleArray<PxU32> inputToOutput, outputToInput;

	inputToOutput.resize(inMesh.points.count, PxU32(-1));
	outputToInput.reserve(inMesh.points.count);

	const float eps = 0.00001f;
	const PxVec4* ptr = static_cast<const PxVec4*>(inMesh.points.data);
	for (PxU32 i = 0; i < numVertices; ++i)
	{
		if (inputToOutput[i] != PxU32(-1))
			continue; // already visited

		inputToOutput[i] = outputToInput.size();
		outputToInput.pushBack(i);

		for (PxU32 j = i+1; j < numVertices; ++j)
		{
			const PxVec3& p0 = (const PxVec3&)ptr[i];
			const PxVec3& p1 = (const PxVec3&)ptr[j];

			float diff = (p0-p1).magnitudeSquared();
			if (diff < eps)
			{
				inputToOutput[j] = inputToOutput[i];
			}
		}
	}

	// copy vertex data
	vertices.resize(outputToInput.size());
	for (PxU32 i = 0; i < outputToInput.size(); ++i)
	{
		PxU32 oi = outputToInput[i];
		vertices[i] = ptr[oi];
	}

	// handle indices
	indices.resize(inMesh.quads.count*4 + inMesh.triangles.count*3);

	PxU32* indexBase = indices.begin();
	memcpy(indexBase, inMesh.quads.data, sizeof(PxU32) * inMesh.quads.count * 4);
	indexBase += inMesh.quads.count * 4;
	memcpy(indexBase, inMesh.triangles.data, sizeof(PxU32) * inMesh.triangles.count * 3);

	for (PxU32 i = 0; i < indices.size(); i++)
		indices[i] = inputToOutput[indices[i]];

	// fill the mesh descriptor
	numVertices = vertices.size();
	PxClothMeshDesc meshDesc;
	meshDesc.points.data = vertices.begin();
	meshDesc.points.count = numVertices;
	meshDesc.points.stride = sizeof(PxVec4);

	meshDesc.invMasses.data = &vertices.begin()->w;
	meshDesc.invMasses.count = static_cast<PxU32>(numVertices);
	meshDesc.invMasses.stride = sizeof(PxVec4);

	PxU32 numQuads = inMesh.quads.count;
	meshDesc.quads.data = indices.begin();
	meshDesc.quads.count = static_cast<PxU32>(numQuads); 
	meshDesc.quads.stride = sizeof(PxU32) * 4; // <- stride per quad

	PxU32 numTriangles = inMesh.triangles.count;
	meshDesc.triangles.data = indices.begin() + numQuads;
	meshDesc.triangles.count = static_cast<PxU32>(numTriangles); 
	meshDesc.triangles.stride = sizeof(PxU32) * 3; // <- stride per triangle

	return meshDesc;
}

#include "wavefront.h"

////////////////////////////////////////////////////////////////////////////////
// create cloth mesh from obj file (user must provide vertex, primitive, and optionally texture coord buffer)
PxClothMeshDesc Test::ClothHelpers::createMeshFromObj(const char* filename, PxReal scale, PxQuat rot, PxVec3 offset, 
	SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords)
{
	WavefrontObj wo;
	wo.loadObj(filename, true);

	int numVertices	= wo.mVertexCount;
	int numTriangles = wo.mTriCount;	

	vertices.resize(numVertices);
	indices.resize(numTriangles*3);
	texcoords.resize(numVertices);

	PxVec3 *vSrc = (PxVec3*)wo.mVertices;
	PxVec4 *vDest = (PxVec4*)vertices.begin();
	for (int i = 0; i < numVertices; i++, vDest++, vSrc++) 
		*vDest = PxVec4(scale * rot.rotate(*vSrc) + offset, 1.0f);

	memcpy((PxU32*)indices.begin(), 
		wo.mIndices, sizeof(PxU32)*numTriangles*3);

	texcoords.resize(numVertices);
	memcpy(&texcoords.begin()->x, 
		wo.mTexCoords, sizeof(PxVec2) * numVertices);

	PxClothMeshDesc meshDesc;

	// convert vertex array to PxBoundedData (desc.points)
	meshDesc.points.data = vertices.begin();
	meshDesc.points.count = static_cast<PxU32>(numVertices);
	meshDesc.points.stride = sizeof(PxVec4);

	meshDesc.invMasses.data = &vertices.begin()->w;
	meshDesc.invMasses.count = static_cast<PxU32>(numVertices);
	meshDesc.invMasses.stride = sizeof(PxVec4);

	// convert face index array to PxBoundedData (desc.triangles)
	meshDesc.triangles.data = indices.begin();
	meshDesc.triangles.count = static_cast<PxU32>(numTriangles); 
	meshDesc.triangles.stride = sizeof(PxU32) * 3; // <- stride per triangle

	return meshDesc;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::setMotionConstraints(PxCloth &cloth, PxReal radius)
{
	PxU32 numParticles = cloth.getNbParticles();

	PxClothParticleData* readData = cloth.lockParticleData();
    if (!readData)
        return false;

	const PxClothParticle* particles = readData->particles;

	SampleArray<PxClothParticleMotionConstraint> constraints(numParticles);

	for (PxU32 i = 0; i < numParticles; i++) {
		constraints[i].pos = particles[i].pos;
		constraints[i].radius = radius;
	}

	readData->unlock();

	cloth.setMotionConstraints(constraints.begin());

	return true;
}

bool Test::ClothHelpers::setStiffness(PxCloth& cloth, PxReal newStiffness)
{
	PxClothStretchConfig stretchConfig;
	stretchConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eVERTICAL);
	stretchConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eVERTICAL, stretchConfig);

	stretchConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eHORIZONTAL);
	stretchConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eHORIZONTAL, stretchConfig);

	PxClothStretchConfig shearingConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eSHEARING);
	shearingConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eSHEARING, shearingConfig);

	PxClothStretchConfig bendingConfig = cloth.getStretchConfig(PxClothFabricPhaseType::eBENDING);
	bendingConfig.stiffness = newStiffness;
	cloth.setStretchConfig(PxClothFabricPhaseType::eBENDING, bendingConfig);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
namespace
{
	static PxVec3 gVirtualParticleWeights[] = 
	{ 
		// center point
		PxVec3(1.0f / 3, 1.0f / 3, 1.0f / 3),

		// off-center point
		PxVec3(4.0f / 6, 1.0f / 6, 1.0f / 6),

		// edge point
		PxVec3(1.0f / 2, 1.0f / 2, 0.0f),
	};

	shdfnd::Pair<PxU32, PxU32> makeEdge(PxU32 v0, PxU32 v1)
	{
		if(v0 < v1)
			return shdfnd::Pair<PxU32, PxU32>(v0, v1);
		else
			return shdfnd::Pair<PxU32, PxU32>(v1, v0);
	}
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::createVirtualParticles(PxCloth& cloth, PxClothMeshDesc& meshDesc, int level)
{
	if(level < 1 || level > 5)
		return false;

	PxU32 edgeSampleCount[] = { 0, 0, 1, 1, 0, 1 };
	PxU32 triSampleCount[] = { 0, 1, 0, 1, 3, 3 };
	PxU32 quadSampleCount[] = { 0, 1, 0, 1, 4, 4 };

	PxU32 numEdgeSamples = edgeSampleCount[level];
	PxU32 numTriSamples = triSampleCount[level];
	PxU32 numQuadSamples = quadSampleCount[level];

	PxU32 numTriangles = meshDesc.triangles.count;
    PxU8* triangles = (PxU8*)meshDesc.triangles.data;

	PxU32 numQuads = meshDesc.quads.count;
	PxU8* quads = (PxU8*)meshDesc.quads.data;

	SampleArray<PxU32> indices;
	indices.reserve(numTriangles * (numTriSamples + 3*numEdgeSamples) 
		+ numQuads * (numQuadSamples + 4*numEdgeSamples));

	typedef shdfnd::Pair<PxU32, PxU32> Edge;
	shdfnd::HashSet<Edge> edges;

    for (PxU32 i = 0; i < numTriangles; i++)
    {
		PxU32 v0, v1, v2;

		if (meshDesc.flags & PxMeshFlag::e16_BIT_INDICES)
		{
			PxU16* triangle = (PxU16*)triangles;
			v0 = triangle[0];
			v1 = triangle[1];
			v2 = triangle[2];
		}
		else
		{
			PxU32* triangle = (PxU32*)triangles;
			v0 = triangle[0];
			v1 = triangle[1];
			v2 = triangle[2];
		}

		if(numTriSamples == 1)
		{
			indices.pushBack(v0);
			indices.pushBack(v1);
			indices.pushBack(v2);
			indices.pushBack(0);
		}

		if(numTriSamples == 3)
		{
			indices.pushBack(v0);
			indices.pushBack(v1);
			indices.pushBack(v2);
			indices.pushBack(1);

			indices.pushBack(v1);
			indices.pushBack(v2);
			indices.pushBack(v0);
			indices.pushBack(1);

			indices.pushBack(v2);
			indices.pushBack(v0);
			indices.pushBack(v1);
			indices.pushBack(1);
		}

		if(numEdgeSamples == 1)
		{
			if(edges.insert(makeEdge(v0, v1)))
			{
				indices.pushBack(v0);
				indices.pushBack(v1);
				indices.pushBack(v2);
				indices.pushBack(2);
			}

			if(edges.insert(makeEdge(v1, v2)))
			{
				indices.pushBack(v1);
				indices.pushBack(v2);
				indices.pushBack(v0);
				indices.pushBack(2);
			}

			if(edges.insert(makeEdge(v2, v0)))
			{
				indices.pushBack(v2);
				indices.pushBack(v0);
				indices.pushBack(v1);
				indices.pushBack(2);
			}
		}

		triangles += meshDesc.triangles.stride;
	}

	for (PxU32 i = 0; i < numQuads; i++)
	{
		PxU32 v0, v1, v2, v3;

		if (meshDesc.flags & PxMeshFlag::e16_BIT_INDICES)
		{
			PxU16* quad = (PxU16*)quads;
			v0 = quad[0];
			v1 = quad[1];
			v2 = quad[2];
			v3 = quad[3];
		}
		else
		{
			PxU32* quad = (PxU32*)quads;
			v0 = quad[0];
			v1 = quad[1];
			v2 = quad[2];
			v3 = quad[3];
		}

		if(numQuadSamples == 1)
		{
			indices.pushBack(v0);
			indices.pushBack(v2);
			indices.pushBack(v3);
			indices.pushBack(2);
		}

		if(numQuadSamples == 4)
		{
			indices.pushBack(v0);
			indices.pushBack(v1);
			indices.pushBack(v2);
			indices.pushBack(1);

			indices.pushBack(v1);
			indices.pushBack(v2);
			indices.pushBack(v3);
			indices.pushBack(1);

			indices.pushBack(v2);
			indices.pushBack(v3);
			indices.pushBack(v0);
			indices.pushBack(1);

			indices.pushBack(v3);
			indices.pushBack(v0);
			indices.pushBack(v1);
			indices.pushBack(1);
		}

		if(numEdgeSamples == 1)
		{
			if(edges.insert(makeEdge(v0, v1)))
			{
				indices.pushBack(v0);
				indices.pushBack(v1);
				indices.pushBack(v2);
				indices.pushBack(2);
			}

			if(edges.insert(makeEdge(v1, v2)))
			{
				indices.pushBack(v1);
				indices.pushBack(v2);
				indices.pushBack(v3);
				indices.pushBack(2);
			}

			if(edges.insert(makeEdge(v2, v3)))
			{
				indices.pushBack(v2);
				indices.pushBack(v3);
				indices.pushBack(v0);
				indices.pushBack(2);
			}

			if(edges.insert(makeEdge(v3, v0)))
			{
				indices.pushBack(v3);
				indices.pushBack(v0);
				indices.pushBack(v1);
				indices.pushBack(2);
			}
		}

		quads += meshDesc.quads.stride;
	}

    cloth.setVirtualParticles(indices.size()/4, 
		indices.begin(), 3, gVirtualParticleWeights);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
PxBounds3 Test::ClothHelpers::getAllWorldBounds(PxCloth& cloth)
{
	PxBounds3 totalBounds = cloth.getWorldBounds();

	PxU32 numSpheres = cloth.getNbCollisionSpheres();

	SampleArray<PxClothCollisionSphere> spheres(numSpheres);
	/*
	SampleArray<PxU32> pairs(numCapsules*2);
	SampleArray<PxClothCollisionPlane> planes(numPlanes);
	SampleArray<PxU32> convexMasks(numConvexes);
	SampleArray<PxClothCollisionTriangle> triangles(numTriangles);
	*/

	cloth.getCollisionData(spheres.begin(), 0, 0, 0, 0
		/*pairs.begin(), planes.begin(), convexMasks.begin(), triangles.begin()*/);

	PxTransform clothPose = cloth.getGlobalPose();

	for (PxU32 i = 0; i < numSpheres; i++)
	{
		PxVec3 p = clothPose.transform(spheres[i].pos);
		PxBounds3 bounds = PxBounds3::centerExtents(p, PxVec3(spheres[i].radius));
		totalBounds.minimum = totalBounds.minimum.minimum(bounds.minimum);
		totalBounds.maximum = totalBounds.maximum.maximum(bounds.maximum);
	}

	return totalBounds;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::getParticlePositions(PxCloth &cloth, SampleArray<PxVec3> &positions)
{
	PxClothParticleData* readData = cloth.lockParticleData();
    if (!readData)
        return false;

	const PxClothParticle* particles = readData->particles;
	if (!particles)
		return false;

	PxU32 nbParticles = cloth.getNbParticles();
	positions.resize(nbParticles);
	for (PxU32 i = 0; i < nbParticles; i++) 
		positions[i] = particles[i].pos;

	readData->unlock();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Test::ClothHelpers::setParticlePositions(PxCloth &cloth, const SampleArray<PxVec3> &positions, bool useConstrainedOnly, bool useCurrentOnly)
{
	PxU32 nbParticles = cloth.getNbParticles();
	if (nbParticles != positions.size())
		return false;

	PxClothParticleData* particleData = cloth.lockParticleData(PxDataAccessFlag::eWRITABLE);
    if (!particleData)
        return false;

	PxClothParticle* particles = particleData->particles;
	for (PxU32 i = 0; i < nbParticles; i++) 
	{		
		bool constrained = particles[i].invWeight == 0.0f;
		if (!useConstrainedOnly || constrained)
			particles[i].pos = positions[i];
	}

	if(!useCurrentOnly)
		particleData->previousParticles = particleData->particles;

	particleData->unlock();

	return true;
}

#endif // PX_USE_CLOTH_API
