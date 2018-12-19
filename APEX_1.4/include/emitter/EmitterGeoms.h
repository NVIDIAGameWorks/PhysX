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



#ifndef EMITTER_GEOMS_H
#define EMITTER_GEOMS_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

///Emitter type
struct EmitterType
{
	///Enum of emitter types
	enum Enum
	{
		ET_RATE = 0,			///< use emittion rate to calculate the number of particles
		ET_DENSITY_ONCE,		///< emit density particles at once
		ET_DENSITY_BRUSH,		///< brush behavior: particles are emitted proportionally to the newly occupied vlume
		ET_FILL,				///< fill the volume of the emitter with particles
		ET_UNDEFINED,
	};
};

class EmitterGeomBox;
class EmitterGeomSphere;
class EmitterGeomSphereShell;
class EmitterGeomCylinder;
class EmitterGeomExplicit;

///Base class for all emitter shapes
class EmitterGeom
{
public:
	///Sets the type of the emitter
	virtual void							setEmitterType(EmitterType::Enum) = 0;
	
	///Gets the type of the emitter
	virtual EmitterType::Enum				getEmitterType() const = 0;

	///If it is a box, cast to box class, return NULL otherwise
	virtual const EmitterGeomBox* 			isBoxGeom() const
	{
		return NULL;
	}
	
	///If it is a sphere, cast to sphere class, return NULL otherwise
	virtual const EmitterGeomSphere* 		isSphereGeom() const
	{
		return NULL;
	}
	
	///If it is a sphere shell, cast to sphere shell class, return NULL otherwise
	virtual const EmitterGeomSphereShell* 	isSphereShellGeom() const
	{
		return NULL;
	}
	
	///If it is a cylinder shell, cast to cylinder class, return NULL otherwise
	virtual const EmitterGeomCylinder* 		isCylinderGeom() const
	{
		return NULL;
	}
	
	///If it is an explicit geom, cast to explicit geom class, return NULL otherwise
	virtual const EmitterGeomExplicit* 		isExplicitGeom() const
	{
		return NULL;
	}
};

///Sphere shell shape for an emitter. It's a shape formed by the difference of two cocentered spheres.
class EmitterGeomSphereShell : public EmitterGeom
{
public:
	
	///Set sphere shell radius
	virtual void 	setRadius(float radius) = 0;
	
	///Get sphere shell radius
	virtual float 	getRadius() const = 0;

	///Set sphere shell thickness
	virtual void 	setShellThickness(float thickness) = 0;
	
	///Get sphere shell thickness
	virtual float 	getShellThickness() const = 0;
};

///Spherical shape for an emitter.
class EmitterGeomSphere : public EmitterGeom
{
public:

	///Set sphere radius
	virtual void	setRadius(float radius) = 0;
	
	///Get sphere radius
	virtual float	getRadius() const = 0;
};

///Cylindrical shape for an emitter.
class EmitterGeomCylinder : public EmitterGeom
{
public:

	///Set cylinder radius
	virtual void	setRadius(float radius) = 0;
	
	///Get cylinder radius
	virtual float	getRadius() const = 0;

	///Set cylinder height
	virtual void	setHeight(float height) = 0;
	
	///Get cylinder height
	virtual float	getHeight() const = 0;
};

///Box shape for an emitter.
class EmitterGeomBox : public EmitterGeom
{
public:

	///Set box extents
	virtual void	setExtents(const PxVec3& extents) = 0;
	
	///Get box extents
	virtual PxVec3	getExtents() const = 0;
};

///Explicit geometry. Coordinates of each particle are given explicitly.
class EmitterGeomExplicit : public EmitterGeom
{
public:
	
	///Point parameters
	struct PointParams
	{
		PxVec3 position; ///< Point position
		bool doDetectOverlaps; ///< Should detect overlap
	};

	///Sphere prameters
	struct SphereParams
	{
		PxVec3 center; ///< Sphere center
		float radius; ///< Sphere radius
		bool doDetectOverlaps; ///< Should detect overlap
	};

	///Ellipsoid parameters
	struct EllipsoidParams
	{
		PxVec3 center; ///< Ellipsoid center
		float radius; ///< Ellipsoid radius
		PxVec3 normal; ///< Ellipsoid normal
		float polarRadius; ///< Ellipsoid polar radius
		bool doDetectOverlaps; ///< Should detect overlap
	};

	///Remove all shapes
	virtual void    resetParticleList() = 0;

	/**
	\brief Add particles to geometry to be emitted
	\param [in] count - number of particles being added by this call
	\param [in] params must be specified.  When emitted, these relative positions are added to emitter actor position
	\param [in] velocities if NULL, the geometry's velocity list will be padded with zero velocities and the asset's velocityRange will be used for velocity
	*/
	virtual void    addParticleList(uint32_t count,
	                                const PointParams* params,
	                                const PxVec3* velocities = 0) = 0;

	/**
	\brief Add particles to geometry to be emitted
	\param [in] count - number of particles being added by this call
	\param [in] positions must be specified.  When emitted, these relative positions are added to emitter actor position
	\param [in] velocities if NULL, the geometry's velocity list will be padded with zero velocities and the asset's velocityRange will be used for velocity
	*/
	virtual void    addParticleList(uint32_t count,
	                                const PxVec3* positions,
	                                const PxVec3* velocities = 0) = 0;

	///Structure contains positions, velocities and user data for particles
	struct PointListData
	{
		const void* positionStart; 			///< Pointer to first position
		uint32_t 	positionStrideBytes; 	///< The stride between position vectors
		const void* velocityStart;			///< Pointer to first velocity
		uint32_t 	velocityStrideBytes;	///< The stride between velocity vectors
		const void* userDataStart;			///< Pointer to first instance of user data
		uint32_t 	userDataStrideBytes;	///< The stride between instances of user data
	};
	
	/**
	\brief Add particles to geometry to be emitted
	\param [in] count - number of particles being added by this call
	\param [in] data - particles data
	\see EmitterGeomExplicit::PointListData
	*/
	virtual void    addParticleList(uint32_t count, const PointListData& data) = 0;

	/**
	\brief Add spheres to geometry to be emitted
	\param [in] count - number of spheres being added by this call
	\param [in] params - spheres parameters
	\param [in] velocities if NULL, the geometry's velocity list will be padded with zero velocities and the asset's velocityRange will be used for velocity
	\see EmitterGeomExplicit::SphereParams
	*/
	virtual void    addSphereList(uint32_t count,
	                              const SphereParams* params,
	                              const PxVec3* velocities = 0) = 0;

	/**
	\brief Add ellipsoids to geometry to be emitted
	\param [in] count - number of ellipsoids being added by this call
	\param [in] params - ellipsoids parameters
	\param [in] velocities if NULL, the geometry's velocity list will be padded with zero velocities and the asset's velocityRange will be used for velocity
	\see EmitterGeomExplicit::EellipsoidParams
	*/
	virtual void    addEllipsoidList(uint32_t count,
	                                 const EllipsoidParams*  params,
	                                 const PxVec3* velocities = 0) = 0;

	/**
	\brief Access particles list
	\param [out] params - particles coordinates
	\param [out] numPoints - number of particles in list
	\param [out] velocities - particles velocities
	\param [out] numVelocities - number of particles velocities in list
	\see EmitterGeomExplicit::PointParams
	*/
	virtual void	getParticleList(const PointParams* &params,
	                                uint32_t& numPoints,
	                                const PxVec3* &velocities,
	                                uint32_t& numVelocities) const = 0;

	/**
	\brief Access spheres list
	\param [out] params - spheres parameters
	\param [out] numSpheres - number of spheres in list
	\param [out] velocities - spheres velocities
	\param [out] numVelocities - number of spheres velocities in list
	\see EmitterGeomExplicit::SphereParams
	*/
	virtual void	getSphereList(const SphereParams* &params,
	                              uint32_t& numSpheres,
	                              const PxVec3* &velocities,
	                              uint32_t& numVelocities) const = 0;

	/**
	\brief Access ellipsoids list
	\param [out] params - ellipsoids parameters
	\param [out] numEllipsoids - number of ellipsoids in list
	\param [out] velocities - ellipsoids velocities
	\param [out] numVelocities - number of ellipsoids velocities in list
	\see EmitterGeomExplicit::EllipsoidParams
	*/
	virtual void	getEllipsoidList(const EllipsoidParams* &params,
	                                 uint32_t& numEllipsoids,
	                                 const PxVec3* &velocities,
	                                 uint32_t& numVelocities) const = 0;

	///Get the number of points
	virtual uint32_t	getParticleCount() const = 0;

	///Get the position of point
	virtual PxVec3  	getParticlePos(uint32_t index) const = 0;

	///Get the number of spheres
	virtual uint32_t   	getSphereCount() const = 0;

	///Get the center of the sphere
	virtual PxVec3 		getSphereCenter(uint32_t index) const = 0;

	///Get the radius of the sphere
	virtual float 		getSphereRadius(uint32_t index) const = 0;

	///Get the number of ellipsoids
	virtual uint32_t   	getEllipsoidCount() const = 0;

	///Get the center of the ellipsoid
	virtual PxVec3 		getEllipsoidCenter(uint32_t index) const = 0;

	///Get the radius of the ellipsoid
	virtual float 		getEllipsoidRadius(uint32_t index) const = 0;

	///Get the normal of the ellipsoid
	virtual PxVec3 		getEllipsoidNormal(uint32_t index) const = 0;

	///Get the polar radius of the ellipsoid
	virtual float 		getEllipsoidPolarRadius(uint32_t index) const = 0;

	///Get average distance between particles
	virtual float 		getDistance() const = 0;
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // EMITTER_GEOMS_H
