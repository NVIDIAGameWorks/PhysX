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



#include "Cooking.h"

#include <PsArray.h>
#include <PsMathUtils.h>
#include <PsSort.h>
#include <Ps.h>

#include <ApexSDKIntl.h>

#include "ClothingCookedPhysX3Param.h"

#include "ExtClothFabricCooker.h"
#include "ExtClothMeshQuadifier.h"

#include "ModuleClothingHelpers.h"

#include <ctime>

namespace
{
	using namespace nvidia;
	
	struct VirtualParticle
	{
		VirtualParticle(uint32_t i0, uint32_t i1, uint32_t i2)
		{
			indices[0] = i0;
			indices[1] = i1;
			indices[2] = i2;
			tableIndex = 0;
		}

		void rotate(uint32_t count)
		{
			while (count--)
			{
				const uint32_t temp = indices[2];
				indices[2] = indices[1];
				indices[1] = indices[0];
				indices[0] = temp;
			}
		}

		uint32_t indices[3];
		uint32_t tableIndex;
	};

	struct EdgeAndLength
	{
		EdgeAndLength(uint32_t edgeNumber, float length) : mEdgeNumber(edgeNumber), mLength(length) {}
		uint32_t mEdgeNumber;
		float mLength;

		bool operator<(const EdgeAndLength& other) const
		{
			return mLength < other.mLength;
		}
	};
}

namespace nvidia
{
namespace clothing
{

bool Cooking::mTetraWarning = false;

NvParameterized::Interface* Cooking::execute()
{
	ClothingCookedPhysX3Param* rootCookedData = NULL;

	for (uint32_t meshIndex = 0; meshIndex < mPhysicalMeshes.size(); meshIndex++)
	{
		if (mPhysicalMeshes[meshIndex].isTetrahedral)
		{
			if (!mTetraWarning)
			{
				mTetraWarning = true;
				APEX_INVALID_OPERATION("Tetrahedral meshes are not (yet) supported with the 3.x solver");
			}
			continue;
		}

		ClothingCookedPhysX3Param* cookedData = NULL;

		cookedData = fiberCooker(meshIndex);

		computeVertexWeights(cookedData, meshIndex);
		fillOutSetsDesc(cookedData);

		createVirtualParticles(cookedData, meshIndex);
		createSelfcollisionIndices(cookedData, meshIndex);

		if (rootCookedData == NULL)
		{
			rootCookedData = cookedData;
		}
		else
		{
			ClothingCookedPhysX3Param* addCookedData = rootCookedData;
			while (addCookedData != NULL && addCookedData->nextCookedData != NULL)
			{
				addCookedData = static_cast<ClothingCookedPhysX3Param*>(addCookedData->nextCookedData);
			}
			addCookedData->nextCookedData = cookedData;
		}
	}

	return rootCookedData;
}



ClothingCookedPhysX3Param* Cooking::fiberCooker(uint32_t meshIndex) const
{
	const uint32_t numSimulatedVertices = mPhysicalMeshes[meshIndex].numSimulatedVertices;
	const uint32_t numAttached = mPhysicalMeshes[meshIndex].numMaxDistance0Vertices;

	shdfnd::Array<PxVec4> vertices(numSimulatedVertices);
	for (uint32_t i = 0; i < numSimulatedVertices; i++)
		vertices[i] = PxVec4(mPhysicalMeshes[meshIndex].vertices[i], 1.0f);

	if (numAttached > 0)
	{
		const uint32_t start = numSimulatedVertices - numAttached;
		for (uint32_t i = start; i < numSimulatedVertices; i++)
			vertices[i].w = 0.0f;
	}

	PxClothMeshDesc desc;

	desc.points.data = vertices.begin();
	desc.points.count = numSimulatedVertices;
	desc.points.stride = sizeof(PxVec4);

	desc.invMasses.data = &vertices.begin()->w;
	desc.invMasses.count = numSimulatedVertices;
	desc.invMasses.stride = sizeof(PxVec4);

	desc.triangles.data = mPhysicalMeshes[meshIndex].indices;
	desc.triangles.count = mPhysicalMeshes[meshIndex].numSimulatedIndices / 3;
	desc.triangles.stride = sizeof(uint32_t) * 3;

	PxClothMeshQuadifier quadifier(desc);

	PxClothFabricCooker cooker(quadifier.getDescriptor(), mGravityDirection);
	
	PxClothFabricDesc fabric = cooker.getDescriptor(); 

	int32_t nbConstraints = (int32_t)fabric.sets[fabric.nbSets - 1];

	ClothingCookedPhysX3Param* cookedData = NULL;

	bool success = true;
	if (success)
	{
		cookedData = static_cast<ClothingCookedPhysX3Param*>(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingCookedPhysX3Param::staticClassName()));

		NvParameterized::Handle arrayHandle(cookedData);
		arrayHandle.getParameter("deformableIndices");
		arrayHandle.resizeArray(nbConstraints * 2);
		arrayHandle.setParamU32Array(fabric.indices, nbConstraints * 2);

		arrayHandle.getParameter("deformableRestLengths");
		arrayHandle.resizeArray(nbConstraints);
		arrayHandle.setParamF32Array(fabric.restvalues, nbConstraints);

		arrayHandle.getParameter("deformableSets");
		const int32_t numSets = (int32_t)fabric.nbSets;
		arrayHandle.resizeArray(numSets);
		for (int32_t i = 0; i < numSets; i++)
		{
			arrayHandle.set(i);
			arrayHandle.set(0);
			arrayHandle.setParamU32(fabric.sets[(uint32_t)i]);
			arrayHandle.popIndex();
			arrayHandle.popIndex();
		}

		arrayHandle.getParameter("deformablePhaseDescs");
		arrayHandle.resizeArray((int32_t)fabric.nbPhases);

		for (uint32_t i = 0; i < fabric.nbPhases; i++)
		{
			PxClothFabricPhase phase = fabric.phases[i];
			cookedData->deformablePhaseDescs.buf[i].phaseType = phase.phaseType;
			cookedData->deformablePhaseDescs.buf[i].setIndex = phase.setIndex;
		}

		arrayHandle.getParameter("tetherAnchors");
		arrayHandle.resizeArray((int32_t)fabric.nbTethers);
		arrayHandle.setParamU32Array(fabric.tetherAnchors, (int32_t)fabric.nbTethers);

		arrayHandle.getParameter("tetherLengths");
		arrayHandle.resizeArray((int32_t)fabric.nbTethers);
		arrayHandle.setParamF32Array(fabric.tetherLengths, (int32_t)fabric.nbTethers);

		cookedData->physicalMeshId = meshIndex;
		cookedData->numVertices = numSimulatedVertices;

		//dumpObj("c:\\lastCooked.obj", meshIndex);
		//dumpApx("c:\\lastCooked.apx", cookedData);

		cookedData->cookedDataVersion = getCookingVersion();
	}
	else
	{
#if PX_WINDOWS_FAMILY
		static int failureCount = 0;
		char buf[64];
		sprintf_s(buf, 64, "c:\\cookingFailure_%d.obj", failureCount++);
		dumpObj(buf, meshIndex);

		APEX_INTERNAL_ERROR("Fiber cooking failure (mesh %d), the failing mesh has been dumped to \'%s\'", meshIndex, buf);
#else
		APEX_INTERNAL_ERROR("Fiber cooking failure (mesh %d)", meshIndex);
#endif

	}


	return cookedData;
}

void Cooking::computeVertexWeights(ClothingCookedPhysX3Param* cookedData, uint32_t meshIndex) const
{
	const uint32_t* indices					= mPhysicalMeshes[cookedData->physicalMeshId].indices;
	const PxVec3*	positions				= mPhysicalMeshes[cookedData->physicalMeshId].vertices;
	const uint32_t	numSimulatedIndices		= mPhysicalMeshes[meshIndex].numSimulatedIndices;
	const uint32_t	numSimulatedVertices	= mPhysicalMeshes[meshIndex].numSimulatedVertices;

	nvidia::Array<float> weights(numSimulatedVertices, 0.0f);

	PX_ASSERT(numSimulatedIndices % 3 == 0);
	for (uint32_t i = 0; i < numSimulatedIndices; i += 3)
	{
		const PxVec3 v1 = positions[indices[i + 1]] - positions[indices[i]];
		const PxVec3 v2 = positions[indices[i + 2]] - positions[indices[i]];
		const float area = v1.cross(v2).magnitude();

		for (uint32_t j = 0; j < 3; j++)
		{
			weights[indices[i + j]] += area;
		}
	}

	float weightSum = 0.0f;
	for (uint32_t i = 0; i < numSimulatedVertices; i++)
	{
		weightSum += weights[i];
	}

	const float weightScale = (float)numSimulatedVertices / weightSum;

	for (uint32_t i = 0; i < numSimulatedVertices; i++)
	{
		weights[i] *= weightScale;
	}

	NvParameterized::Handle handle(*cookedData, "deformableInvVertexWeights");
	if (handle.resizeArray((int32_t)numSimulatedVertices) == NvParameterized::ERROR_NONE)
	{
		for (uint32_t i = 0; i < numSimulatedVertices; i++)
		{
			cookedData->deformableInvVertexWeights.buf[i] = 1.0f / weights[i];
		}
	}
}



void Cooking::createVirtualParticles(ClothingCookedPhysX3Param* cookedData, uint32_t meshIndex)
{
	const PxVec3*	positions	= mPhysicalMeshes[cookedData->physicalMeshId].vertices;
	const uint32_t* indices		= mPhysicalMeshes[cookedData->physicalMeshId].indices;
	const uint32_t	numIndices	= mPhysicalMeshes[meshIndex].numSimulatedIndices;

	nvidia::Array<VirtualParticle> particles;

	const float minTriangleArea = mVirtualParticleDensity * mPhysicalMeshes[cookedData->physicalMeshId].smallestTriangleArea / 2.0f +
	                              (1.0f - mVirtualParticleDensity) * mPhysicalMeshes[cookedData->physicalMeshId].largestTriangleArea;
	const float coveredTriangleArea = minTriangleArea;

	for (uint32_t i = 0; i < numIndices; i += 3)
	{
		VirtualParticle particle(indices[i], indices[i + 1], indices[i + 2]);

		const PxVec3 edge1 = positions[particle.indices[1]] - positions[particle.indices[0]];
		const PxVec3 edge2 = positions[particle.indices[2]] - positions[particle.indices[0]];
		const float triangleArea = edge1.cross(edge2).magnitude();

		const float numSpheres = triangleArea / coveredTriangleArea;

		if (numSpheres <= 1.0f)
		{
			// do nothing
		}
		else if (numSpheres < 2.0f)
		{
			// add one virtual particle
			particles.pushBack(particle);
		}
		else
		{
			// add two or three, depending on whether it's a slim triangle.
			EdgeAndLength eal0(0, edge1.magnitude());
			EdgeAndLength eal1(1, (positions[particle.indices[2]] - positions[particle.indices[1]]).magnitude());
			EdgeAndLength eal2(2, edge2.magnitude());
			EdgeAndLength middle = eal0 < eal1 ? eal0 : eal1; // technically this does not have to be the middle of the three, but for the test below it suffices.
			EdgeAndLength smallest = middle < eal2 ? middle : eal2;
			if (smallest.mLength * 2.0f < middle.mLength)
			{
				// two
				particle.rotate(smallest.mEdgeNumber);
				particle.tableIndex = 2;
				particles.pushBack(particle);
				particle.tableIndex = 3;
				particles.pushBack(particle);
			}
			else
			{
				// three
				particle.tableIndex = 1;
				particles.pushBack(particle);
				particle.rotate(1);
				particles.pushBack(particle);
				particle.rotate(1);
				particles.pushBack(particle);
			}
		}
	}

	if (!particles.empty())
	{
		NvParameterized::Handle handle(cookedData);
		handle.getParameter("virtualParticleIndices");
		handle.resizeArray((int32_t)particles.size() * 4);
		handle.getParameter("virtualParticleWeights");
		handle.resizeArray(3 * 4);

		// table index 0, the center particle
		cookedData->virtualParticleWeights.buf[0] = 1.0f / 3.0f;
		cookedData->virtualParticleWeights.buf[1] = 1.0f / 3.0f;
		cookedData->virtualParticleWeights.buf[2] = 1.0f / 3.0f;

		// table index 1, three particles
		cookedData->virtualParticleWeights.buf[3] = 0.1f;
		cookedData->virtualParticleWeights.buf[4] = 0.3f;
		cookedData->virtualParticleWeights.buf[5] = 0.6f;

		// table index 2, the pointy particle
		cookedData->virtualParticleWeights.buf[6] = 0.7f;
		cookedData->virtualParticleWeights.buf[7] = 0.15f;
		cookedData->virtualParticleWeights.buf[8] = 0.15f;

		// table index 3, the flat particle
		cookedData->virtualParticleWeights.buf[9] = 0.3f;
		cookedData->virtualParticleWeights.buf[10] = 0.35f;
		cookedData->virtualParticleWeights.buf[11] = 0.35f;

		for (uint32_t i = 0; i < particles.size(); i++)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				cookedData->virtualParticleIndices.buf[4 * i + j] = particles[i].indices[j];
			}
			cookedData->virtualParticleIndices.buf[4 * i + 3] = particles[i].tableIndex; // the table index
		}
	}
}


void Cooking::createSelfcollisionIndices(ClothingCookedPhysX3Param* cookedData, uint32_t meshIndex) const
{
	const PxVec3*	positions	= mPhysicalMeshes[cookedData->physicalMeshId].vertices;
	const uint32_t	numVertices = mPhysicalMeshes[meshIndex].numSimulatedVertices;


	// we'll start with a full set of indices, and eliminate the ones we don't want. selfCollisionIndices
	//  is an array of indices, i.e. a second layer of indirection
	Array<uint32_t> selfCollisionIndices;
	for (uint32_t i = 0; i < numVertices; ++i)
	{
		selfCollisionIndices.pushBack(i);
	}

	float selfcollisionThicknessSq = mSelfcollisionRadius * mSelfcollisionRadius;
	for (uint32_t v0ii = 0; v0ii < selfCollisionIndices.size(); ++v0ii)
	{
		// ii suffix means "index into indices array", i suffix just means "index into vertex array"

		// load the first vertex
		uint32_t v0i = selfCollisionIndices[v0ii];
		const PxVec3& v0 = positions[v0i];

		// no need to start at the beginning of the array, those comparisons have already been made.
		// don't autoincrement the sequence index. if we eliminate an index, we'll replace it with one from
		//  the end, and reevaluate that element
		for (uint32_t v1ii = v0ii + 1; v1ii < selfCollisionIndices.size(); ) // don't autoincrement iteratorsee if/else
		{
			uint32_t v1i = selfCollisionIndices[v1ii];
			const PxVec3& v1 = positions[v1i];

			// how close is this particle?
			float v0v1DistanceSq = (v0 - v1).magnitudeSquared();
			if (v0v1DistanceSq < selfcollisionThicknessSq )
			{
				// too close for missiles
				selfCollisionIndices.replaceWithLast(v1ii);

				// don't move on to the next - replaceWithLast put a fresh index at v1ii, so reevaluate it
			}
			else
			{
				// it's comfortably distant, so we'll keep it around (for now). 

				// we need to be mindful of which element we visit next in the outer loop. we want to minimize the distance between 
				//  self colliding particles and not unnecessarily introduce large gaps between them. the easiest way is to pick
				//  the closest non-eliminated particle to the one currently being evaluated, and evaluate it next. if we find one
				//  that's closer than what's currently next in the list, swap it. both of these elements are prior to the next 
				//  sinner-loop element, so this doesn't impact the inner loop traversal

				// if we assume the index of the closest known particle is always v0ii + 1, we can just reevaluate it's distance to 
				//  v0ii every iteration. slightly expensive, but it eliminates the need to maintain redundant 
				//  ClosestDistance/ClosestIndex variables
				uint32_t vNexti = selfCollisionIndices[v0ii + 1];
				const PxVec3& nextVertexToEvaluate = positions[vNexti];

				float v0vNextDistanceSq = (v0 - nextVertexToEvaluate).magnitudeSquared();
				if (v0v1DistanceSq < v0vNextDistanceSq)
				{
					nvidia::swap(selfCollisionIndices[v0ii + 1], selfCollisionIndices[v1ii]);
				}

				// move on to the next
				++v1ii;
			}
		}
	}

	NvParameterized::Handle arrayHandle(cookedData);
	arrayHandle.getParameter("selfCollisionIndices");
	arrayHandle.resizeArray((int32_t)selfCollisionIndices.size());
	arrayHandle.setParamU32Array(selfCollisionIndices.begin(), (int32_t)selfCollisionIndices.size());
}


bool Cooking::verifyValidity(const ClothingCookedPhysX3Param* cookedData, uint32_t meshIndex)
{
	if (cookedData == NULL)
	{
		return false;
	}

	const char* errorMessage = NULL;

	const uint32_t numSetsDescs				= (uint32_t)cookedData->deformableSets.arraySizes[0];
	const uint32_t numDeformableVertices	= mPhysicalMeshes[meshIndex].numSimulatedVertices;

	for (uint32_t validSetsDescs = 0; validSetsDescs < numSetsDescs && errorMessage == NULL; ++validSetsDescs)
	{
		const uint32_t fromIndex = validSetsDescs ? cookedData->deformableSets.buf[validSetsDescs - 1].fiberEnd : 0;
		const uint32_t toIndex = cookedData->deformableSets.buf[validSetsDescs].fiberEnd;
		if (toIndex <= fromIndex)
		{
			errorMessage = "Set without fibers";
		}

		for (uint32_t f = fromIndex; f < toIndex && errorMessage == NULL; ++f)
		{
			uint32_t	posIndex1	= cookedData->deformableIndices.buf[2 * f];
			uint32_t	posIndex2	= cookedData->deformableIndices.buf[2 * f + 1];

			if (posIndex2 > (uint32_t)cookedData->deformableIndices.arraySizes[0])
			{
				errorMessage = "Fiber index out of bounds";
			}
	
			if (posIndex1 >= numDeformableVertices)
			{
				errorMessage = "Deformable index out of bounds";
			}
		}
	}

	if (errorMessage != NULL)
	{
		APEX_INTERNAL_ERROR("Invalid cooked data: %s", errorMessage);
	}

	return (errorMessage == NULL);
}




void Cooking::fillOutSetsDesc(ClothingCookedPhysX3Param* cookedData)
{
	const PxVec3* vertices = mPhysicalMeshes[cookedData->physicalMeshId].vertices;
	for (int32_t sd = 0; sd < cookedData->deformableSets.arraySizes[0]; sd++)
	{
		const uint32_t firstFiber = sd ? cookedData->deformableSets.buf[sd - 1].fiberEnd : 0;
		const uint32_t lastFiber = cookedData->deformableSets.buf[sd].fiberEnd;

		uint32_t numEdges = 0;
		float avgEdgeLength = 0.0f;

		for (uint32_t f = firstFiber; f < lastFiber; f++)
		{
			uint32_t from = cookedData->deformableIndices.buf[f * 2];
			uint32_t to = cookedData->deformableIndices.buf[f*2+1];
			numEdges ++;
			avgEdgeLength += (vertices[to] - vertices[from]).magnitude();
		}

		if (numEdges > 0)
		{
			cookedData->deformableSets.buf[sd].longestFiber = 0;
			cookedData->deformableSets.buf[sd].shortestFiber = 0;
			cookedData->deformableSets.buf[sd].numEdges = numEdges;
			cookedData->deformableSets.buf[sd].avgFiberLength = 0;
			cookedData->deformableSets.buf[sd].avgEdgeLength = avgEdgeLength / (float)numEdges;
		}
	}
}



void Cooking::groupPhases(ClothingCookedPhysX3Param* cookedData, uint32_t meshIndex, uint32_t startIndex, uint32_t endIndex, Array<uint32_t>& phaseEnds) const
{
	shdfnd::Array<bool> usedInPhase(mPhysicalMeshes[meshIndex].numSimulatedVertices, false);
	for (uint32_t f = startIndex; f < endIndex; f++)
	{
		uint32_t index1 = cookedData->deformableIndices.buf[2 * f + 0];
		uint32_t index2 = cookedData->deformableIndices.buf[2 * f + 1];

		if (usedInPhase[index1] || usedInPhase[index2])
		{
			bool swapped = false;

			// need to replace this with one further ahead
			for (uint32_t scanAhead = f + 1; scanAhead < endIndex; scanAhead++)
			{
				const uint32_t i1 = cookedData->deformableIndices.buf[2 * scanAhead + 0];
				const uint32_t i2 = cookedData->deformableIndices.buf[2 * scanAhead + 1];
				if (!usedInPhase[i1] && !usedInPhase[i2])
				{
					// swap
					cookedData->deformableIndices.buf[2 * f + 0] = i1;
					cookedData->deformableIndices.buf[2 * f + 1] = i2;

					cookedData->deformableIndices.buf[2 * scanAhead + 0] = index1;
					cookedData->deformableIndices.buf[2 * scanAhead + 1] = index2;

					nvidia::swap(cookedData->deformableRestLengths.buf[2 * f], cookedData->deformableRestLengths.buf[2 * scanAhead]);

					index1 = i1;
					index2 = i2;

					swapped = true;

					break;
				}
			}

			if (!swapped)
			{
				phaseEnds.pushBack(f);
				f--;

				for (uint32_t i = 0; i < usedInPhase.size(); i++)
				{
					usedInPhase[i] = false;
				}

				continue;
			}
		}

		usedInPhase[index1] = true;
		usedInPhase[index2] = true;
	}
	phaseEnds.pushBack(endIndex);
}



void Cooking::dumpObj(const char* filename, uint32_t meshIndex) const
{
	PX_UNUSED(filename);

#if PX_WINDOWS_FAMILY
	FILE* outputFile = NULL;
	fopen_s(&outputFile, filename, "w");

	if (outputFile == NULL)
	{
		return;
	}

	fprintf(outputFile, "# PhysX3 Cooking input mesh\n");
	fprintf(outputFile, "# Mesh %d\n", meshIndex);

	{
		time_t rawtime;
		struct tm* timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		fprintf(outputFile, "# File Created: %s", asctime(timeinfo));
	}

	fprintf(outputFile, "\n\n\n");

	const uint32_t numVertices = mPhysicalMeshes[meshIndex].numVertices;
	const uint32_t numIndices = mPhysicalMeshes[meshIndex].numIndices;
//	const uint32_t numSimulatedVertices = mPhysicalMeshes[meshIndex].numSimulatedVertices;
//	const uint32_t numSimulatedIndices = mPhysicalMeshes[meshIndex].numSimulatedIndices;

	const PxVec3* vert = mPhysicalMeshes[meshIndex].vertices;
	for (uint32_t i = 0; i < numVertices; i++)
	{
		fprintf(outputFile, "v %f %f %f\n", vert[i].x, vert[i].y, vert[i].z);
	}

	fprintf(outputFile, "\n\n\n");

	const uint32_t* indices = mPhysicalMeshes[meshIndex].indices;
	for (uint32_t i = 0; i < numIndices; i += 3)
	{
		fprintf(outputFile, "f %d %d %d\n", indices[i] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
	}

	fclose(outputFile);
#endif
}



void Cooking::dumpApx(const char* filename, const NvParameterized::Interface* data) const
{
	NvParameterized::Serializer::SerializeType serType = NvParameterized::Serializer::NST_XML;

	if (data == NULL)
	{
		return;
	}

	PxFileBuf* filebuffer = GetInternalApexSDK()->createStream(filename, PxFileBuf::OPEN_WRITE_ONLY);

	if (filebuffer != NULL)
	{
		if (filebuffer->isOpen())
		{
			NvParameterized::Serializer* serializer = GetInternalApexSDK()->createSerializer(serType);
			serializer->serialize(*filebuffer, &data, 1);

			serializer->release();
		}

		filebuffer->release();
		filebuffer = NULL;
	}
}

}
}
