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



#ifndef MODULE_FIELD_SAMPLER_H
#define MODULE_FIELD_SAMPLER_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief This is an optional callback interface for collision filtering between field samplers and other objects
	   If this interface is not provided, then collision filtering happens normally through the default scene simulationfiltershader callback
	   However, if this is provided, then the user can no only return true/false to indicate whether or not the two objects should interact but
	   can also assign a weighted multiplier value to control how strongly the two objects should interact.
*/
class FieldSamplerWeightedCollisionFilterCallback
{
public:
	/**
	\brief This is an optional callback interface for collision filtering between field samplers and other objects
		   If this interface is not provided, then collision filtering happens normally through the default scene simulationfiltershader callback
		   However, if this is provided, then the user can no only return true/false to indicate whether or not the two objects should interact but
		   can also assign a weighted multiplier value to control how strongly the two objects should interact.
	*/
	virtual bool fieldSamplerWeightedCollisionFilter(const physx::PxFilterData &objectA,const physx::PxFilterData &objectB,float &multiplierValue) = 0;
};


/**
 \brief FieldSampler module class
 */
class ModuleFieldSampler : public Module
{
protected:
	virtual					~ModuleFieldSampler() {}

public:

	/**
	\brief Sets the optional weighted collision filter callback for this scene. If not provided, it will use the default SimulationFilterShader on the current scene
	*/
	virtual bool			setFieldSamplerWeightedCollisionFilterCallback(const Scene& apexScene,FieldSamplerWeightedCollisionFilterCallback *callback) = 0;

	
#if PX_PHYSICS_VERSION_MAJOR == 3
	/**
		Set flag to toggle PhysXMonitor for ForceFields.
	*/
	virtual void			enablePhysXMonitor(const Scene& apexScene, bool enable) = 0;
	/**
	\brief Add filter data (collision group) to PhysXMonitor. 
		
	\param apexScene [in] - Apex scene for which to submit the force sample batch.
	\param filterData [in] - PhysX 3.0 collision mask for PhysXMonitor
	*/
	virtual void			setPhysXMonitorFilterData(const Scene& apexScene, physx::PxFilterData filterData) = 0;
#endif

	/**
	\brief Initialize a query for a batch of sampling points

	\param apexScene [in] - Apex scene for which to create the force sample batch query.
	\param maxCount [in] - Maximum number of indices (active samples)
	\param filterData [in] - PhysX 3.0 collision mask for data
	\return the ID of created query
	*/
	virtual uint32_t		createForceSampleBatch(const Scene& apexScene, uint32_t maxCount, const physx::PxFilterData filterData) = 0;

	/**
	\brief Release a query for a batch of sampling points

	\param apexScene [in] - Apex scene for which to create the force sample batch.
	\param batchId [in] - ID of query that should be released
	*/
	virtual void			releaseForceSampleBatch(const Scene& apexScene, uint32_t batchId) = 0;

	/**
	\brief Submits a batch of sampling points to be evaluated during the simulation step.
	
	\param apexScene [in] - Apex scene for which to submit the force sample batch.
	\param batchId [in] - ID of query for force sample batch.
	\param forces [out] - Buffer to which computed forces are written to. The buffer needs to be persistent between calling this function and the next PxApexScene::fetchResults.
	\param forcesStride [in] - Stride between consecutive force vectors within the forces output.
	\param positions [in] - Buffer containing the positions of the input samples.
	\param positionsStride [in] - Stride between consecutive position vectors within the positions input.
	\param velocities [in] - Buffer containing the velocities of the input samples.
	\param velocitiesStride [in] - Stride between consecutive velocity vectors within the velocities input.
	\param mass [in] - Buffer containing the mass of the input samples.
	\param massStride [in] - Stride between consecutive mass values within the mass input.
	\param indices [in] - Buffer containing the indices of the active samples that are considered for the input and output buffers.
	\param numIndices [in] - Number of indices (active samples). 
	*/
	virtual void			submitForceSampleBatch(	const Scene& apexScene, uint32_t batchId,
													PxVec4* forces, const uint32_t forcesStride, 
													const PxVec3* positions, const uint32_t positionsStride, 
													const PxVec3* velocities, const uint32_t velocitiesStride,
													const float* mass, const uint32_t massStride,
													const uint32_t* indices, const uint32_t numIndices) = 0;

};



PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // MODULE_FIELD_SAMPLER_H
