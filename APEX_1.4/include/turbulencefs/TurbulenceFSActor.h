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


#ifndef TURBULENCE_FSACTOR_H
#define TURBULENCE_FSACTOR_H

#include "Apex.h"
#include "Shape.h"

namespace nvidia
{
namespace apex
{

class TurbulenceFSAsset;
class TurbulenceRenderable;

/**
 \brief Turbulence FieldSampler Actor class
 */
class TurbulenceFSActor : public Actor, public Renderable
{
protected:
	virtual ~TurbulenceFSActor() {}

public:

	///Returns the asset the instance has been created from.
	virtual TurbulenceFSAsset* getTurbulenceFSAsset() const = 0;

	///enable/disable the fluid simulation
	virtual void setEnabled(bool enable) = 0;

	/**
	 \brief set the pose of the grid - this includes only the position and rotation.
	 
	 the position is that of the center of the grid, as is determined as (pose.column3.x, pose.column3.y, pose.column3.z)<BR>
	 the rotation is the rotation of the object that the grid is centered on <BR>
	 (the grid does not rotate, but we use this pose for rotating the collision obstacle, the jets and imparting angular momentum)
	 */
	virtual void setPose(PxMat44 pose) = 0;

	/**
	 \brief get the pose of the grid - this includes only the position and rotation.
	 
	 the position is that of the center of the grid, as is determined as (pose.column3.x, pose.column3.y, pose.column3.z)<BR>
	 the rotation is the rotation of the object that the grid is centered on <BR>
	 (the grid does not rotate, but we use this pose for rotating the collision obstacle, the jets and imparting angular momentum)
	 */
	virtual PxMat44 getPose() const = 0;

	///get the grid bounding box min point
	virtual PxVec3 getGridBoundingBoxMin() = 0;
	///get the grid bounding box max point
	virtual PxVec3 getGridBoundingBoxMax() = 0;

	///get the grid size vector
	virtual PxVec3 getGridSize() = 0;

	///get the grid dimensions
	virtual void getGridDimensions(uint32_t &gridX,uint32_t &gridY,uint32_t &gridZ) = 0;

	///set the grid dimensions
	virtual void setGridDimensions(const uint32_t &gridX, const uint32_t &gridY, const uint32_t &gridZ) = 0;

	/**
	 \brief force the current updates per frame to a particular value.

	 Range is 0-1:<BR>
	 1.0f is maximum simulation quality<BR>
	 0.0f is minimum simulation quality
	 */
	virtual void setUpdatesPerFrame(float upd) = 0;
	///get the current value of the updates per frame
	virtual float getUpdatesPerFrame() const = 0;

	/**
	 \brief methods to get the velocity field sampled at grid centers.
	 
	 call setSampleVelocityFieldEnabled(true) to enable the sampling and call getVelocityField to get back the sampled results
	 */
	virtual void getVelocityField(void** x, void** y, void** z, uint32_t& sizeX, uint32_t& sizeY, uint32_t& sizeZ) = 0;

	///enable/disable sample velocity field 
	virtual void setSampleVelocityFieldEnabled(bool enabled) = 0;

	///set a multiplier and a clamp on the total angular velocity induced in the system by the internal collision obstacle or by external collision objects
	virtual void setAngularVelocityMultiplierAndClamp(float angularVelocityMultiplier, float angularVelocityClamp) = 0;

	///set a multiplier and a clamp on the total linear velocity induced in the system by a collision obstacle
	virtual void setLinearVelocityMultiplierAndClamp(float linearVelocityMultiplier, float linearVelocityClamp) = 0;

	///set velocity field fade. All cells in the field multiplies by (1 - fade) on each frame
	virtual void setVelocityFieldFade(float fade) = 0;

	///set fluid viscosity (diffusion) for velocity
	virtual void setFluidViscosity(float viscosity) = 0;

	///set time of velocity field cleaning process [sec]
	virtual void setVelocityFieldCleaningTime(float time) = 0;

	///set time without activity before velocity field cleaning process starts [sec]. 
	virtual void setVelocityFieldCleaningDelay(float time) = 0;

	/**
	 set parameter which correspond to 'a' in erf(a*(cleaning_timer/velocityFieldCleaningTime)). 
	 for full cleaning it should be greater then 2. If you want just decrease velocity magitude use smaller value
	 */
	virtual void setVelocityFieldCleaningIntensity(float a) = 0;

	/**
	 \brief enable whether or not to use heat in the simulation (enabling heat reduces performance).<BR>
	 \note If you are enabling heat then you also need to add temperature sources (without temperature sources you will see no effect of heat on the simulation, except a drop in performance)
	 */
	virtual	void setUseHeat(bool enable) = 0;

	///set heat specific parameters for the simulation
	virtual void setHeatBasedParameters(float forceMultiplier, float ambientTemperature, PxVec3 heatForceDirection, float thermalConductivity) = 0;

	/**
	 \brief enable whether or not to use density in the simulation (enabling density reduces performance).<BR>
	 \note If you are enabling density then you also need to add substance sources (without substance sources you will see no effect of density on the simulation, except a drop in performance)
	 */
	virtual	void setUseDensity(bool enable) = 0;

	///Returns true if turbulence actor is in density mode.
	virtual bool getUseDensity(void) const = 0;

	///set density specific parameters for the simulation
	virtual void setDensityBasedParameters(float diffusionCoef, float densityFieldFade) = 0;

	///get the density grid dimensions
	virtual void getDensityGridDimensions(uint32_t &gridX,uint32_t &gridY,uint32_t &gridZ) = 0;

	/**
	 \brief allows external actors like wind or explosion to add a single directional velocity to the grid.<BR>
	 \note if multiple calls to this function are made only the last call is honored (i.e. the velocities are not accumulated)
	 */
	virtual	void setExternalVelocity(PxVec3 vel) = 0;

	///set a multiplier for the field velocity
	virtual void setFieldVelocityMultiplier(float value) = 0;

	///set a weight for the field velocity
	virtual void setFieldVelocityWeight(float value) = 0;

	///set noise parameters
	virtual void setNoiseParameters(float noiseStrength, PxVec3 noiseSpacePeriod, float noiseTimePeriod, uint32_t noiseOctaves) = 0;

	///set density texture range
	virtual void setDensityTextureRange(float minValue, float maxValue) = 0;

	///Returns the optional volume render material name specified for this turbulence actor.
	virtual const char *getVolumeRenderMaterialName(void) const = 0;

	///Sets the uniform overall object scale
	virtual void setCurrentScale(float scale) = 0;

	///Retrieves the uniform overall object scale
	virtual float getCurrentScale(void) const = 0;

	///Returns true if turbulence actor is in flame mode.
	virtual bool getUseFlame(void) const = 0;

	///Returns flame grid dimensions.
	virtual void getFlameGridDimensions(uint32_t &gridX, uint32_t &gridY, uint32_t &gridZ) const = 0;

	/**
	 \brief Acquire a pointer to the Turbulence renderable proxy and increment its reference count.
	 
	 The TurbulenceRenderable will only be deleted when its reference count is zero.  
	 Calls to TurbulenceRenderable::release decrement the reference count, as does a call to TurbulenceFSActor::release().
	*/
	virtual TurbulenceRenderable* acquireRenderableReference() = 0;

};

}
} // end namespace nvidia

#endif // TURBULENCE_FSACTOR_H
