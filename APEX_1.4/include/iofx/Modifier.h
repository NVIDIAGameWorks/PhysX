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



#ifndef MODIFIER_H
#define MODIFIER_H

#include "Apex.h"
#include "Curve.h"
#include "IofxAsset.h"
#include "IofxRenderCallback.h"

#include "ModifierDefs.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/// Converts ModifierStage to bitmap
inline uint32_t ModifierUsageFromStage(ModifierStage stage)
{
	return 1u << stage;
}

// Disable the unused arguments warning for this header.
#pragma warning( disable: 4100 )

/**
\brief Modifier contains all of the data necessary to apply a single modifier type to a particle system.

Generally this combines some physical transformation with parameters specified at authoring time
to modify the look of the final effect.
*/
class Modifier
{
public:

	/// getModifierType returns the enumerated type associated with this class.
	virtual ModifierTypeEnum getModifierType() const = 0;

	/// getModifierUsage returns the usage scenarios allowed for a particular modifier.
	virtual uint32_t getModifierUsage() const = 0;

	/// returns a bitmap that includes every sprite semantic that the modifier updates
	virtual uint32_t getModifierSpriteSemantics()
	{
		return 0;
	}
	
	/// returns a bitmap that includes every mesh(instance) semantic that the modifier updates
	virtual uint32_t getModifierMeshSemantics()
	{
		return 0;
	}

	virtual ~Modifier() { }

};

/**
\brief ModifierT is a helper class to handle the mapping of Type->Enum and Enum->Type.

This imposes some structure on the subclasses--they all now
expect to have a const static field called ModifierType.
*/
template <typename T>
class ModifierT : public Modifier
{
public:
	
	/// Returns ModifierType for typename T
	virtual ModifierTypeEnum getModifierType() const
	{
		return T::ModifierType;
	}
	
	/// Returns ModifierUsage for typename T
	virtual uint32_t getModifierUsage() const
	{
		return T::ModifierUsage;
	}
};


/**
\brief RotationModifier applies rotation to the particles using one of several rotation models.
*/
class RotationModifier : public ModifierT<RotationModifier>
{
public:
	
	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_Rotation;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn | ModifierUsage_Continuous
	        | ModifierUsage_Mesh;


	/// get the roll model
	virtual ApexMeshParticleRollType::Enum getRollType() const = 0;
	
	/// set the roll model
	virtual void setRollType(ApexMeshParticleRollType::Enum rollType) = 0;
	
	/// get the maximum allowed settle rate per second
	virtual float getMaxSettleRate() const = 0;
	
	/// set the maximum allowed settle rate per second
	virtual void setMaxSettleRate(float settleRate) = 0;
	
	/// get the maximum allowed rotation rate per second
	virtual float getMaxRotationRate() const = 0;
	
	/// set the maximum allowed rotation rate per second
	virtual void setMaxRotationRate(float rotationRate) = 0;

};

/**
 \brief SimpleScaleModifier just applies a simple scale factor to each of the X, Y and Z aspects of the model. Each scalefactor can be
		applied independently.
*/
class SimpleScaleModifier : public ModifierT<SimpleScaleModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_SimpleScale;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn | ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the vector of scale factors along the three axes
	virtual PxVec3 getScaleFactor() const = 0;
	
	/// set the vector of scale factors along the three axes
	virtual void setScaleFactor(const PxVec3& s) = 0;
};

/**
\brief ScaleByMassModifier scales by mass of the particle.
*/
class ScaleByMassModifier : public ModifierT<ScaleByMassModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ScaleByMass;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn | ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;
};

/**
 \brief RandomScaleModifier applies a random scale uniformly to all three dimensions. Currently, the
		scale is a uniform in the range specified.
*/
class RandomScaleModifier : public ModifierT<RandomScaleModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_RandomScale;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the range of scale factors along the three axes
	virtual Range<float> getScaleFactor() const = 0;
	
	/// set the range of scale factors along the three axes
	virtual void setScaleFactor(const Range<float>& s) = 0;
};

/**
 \brief ColorVsLifeModifier modifies the color constants associated with a particle
		depending on the life remaining of the particle.
*/
class ColorVsLifeModifier : public ModifierT<ColorVsLifeModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ColorVsLife;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn | ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the affected color channel
	virtual ColorChannel getColorChannel() const = 0;
	
	/// set the affected color channel
	virtual void setColorChannel(ColorChannel colorChannel) = 0;
	
	/// get the curve that sets the dependency between the lifetime and the color
	virtual const Curve* getFunction() const = 0;
	
	/// set the curve that sets the dependency between the lifetime and the color
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief ColorVsDensityModifier modifies the color constants associated with a particle
		depending on the density of the particle.
*/
class ColorVsDensityModifier : public ModifierT<ColorVsDensityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ColorVsDensity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the affected color channel
	virtual ColorChannel getColorChannel() const = 0;
	
	/// set the affected color channel
	virtual void setColorChannel(ColorChannel colorChannel) = 0;
	
	/// get the curve that sets the dependency between the density and the color
	virtual const Curve* getFunction() const = 0;
	
	/// set the curve that sets the dependency between the density and the color
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief SubtextureVsLifeModifier is a modifier to adjust the subtexture id versus the life remaining of a particular particle.

 Interpretation of the subtexture id over time is up to the application.
 */
class SubtextureVsLifeModifier : public ModifierT<SubtextureVsLifeModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_SubtextureVsLife;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite;


	/// get the curve that sets the dependency between the life remaining and the subtexture id
	virtual const Curve* getFunction() const = 0;
	
	/// set the curve that sets the dependency between the life remaining and the subtexture id
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief OrientAlongVelocity is a modifier to orient a mesh so that a particular axis coincides with the velocity vector.
 */
class OrientAlongVelocityModifier : public ModifierT<OrientAlongVelocityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_OrientAlongVelocity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Mesh;


	/// get the object-space vector that will coincide with the velocity vector
	virtual PxVec3 getModelForward() const = 0;
	
	/// set the object-space vector that will coincide with the velocity vector
	virtual void setModelForward(const PxVec3& s) = 0;
};


/**
 \brief ScaleAlongVelocityModifier is a modifier to apply a scale factor along the current velocity vector.

 Note that without applying an OrientAlongVelocity modifier first, the results for this will be 'odd.'
 */
class ScaleAlongVelocityModifier : public ModifierT<ScaleAlongVelocityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ScaleAlongVelocity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Mesh;


	/// get the scale factor
	virtual float getScaleFactor() const = 0;
	
	/// set the scale factor
	virtual void setScaleFactor(const float& s) = 0;
};

/**
 \brief RandomSubtextureModifier generates a random subtexture ID and places it in the subTextureId field.
 */
class RandomSubtextureModifier : public ModifierT<RandomSubtextureModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_RandomSubtexture;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn
	        | ModifierUsage_Sprite;


	///get the range for subtexture values
	virtual Range<float> getSubtextureRange() const = 0;
	
	///set the range for subtexture values
	virtual void setSubtextureRange(const Range<float>& s) = 0;
};

/**
 \brief RandomRotationModifier will choose a random orientation for a sprite particle within the range as specified below.

 The values in the range are interpreted as radians. Please keep in mind that all the sprites are coplanar to the screen.
 */
class RandomRotationModifier : public ModifierT<RandomRotationModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_RandomRotation;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn
	        | ModifierUsage_Sprite;


	///get the range of orientations, in radians.
	virtual Range<float> getRotationRange() const = 0;
	
	///set the range of orientations, in radians.
	virtual void setRotationRange(const Range<float>& s) = 0;
};

/**
 \brief ScaleVsLifeModifier applies a scale factor function against a single axis versus the life remaining.
 */
class ScaleVsLifeModifier : public ModifierT<ScaleVsLifeModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ScaleVsLife;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the axis along which the scale factor will be applied
	virtual ScaleAxis getScaleAxis() const = 0;
	
	/// set the axis along which the scale factor will be applied
	virtual void setScaleAxis(ScaleAxis a) = 0;
	
	/// get the the curve that defines the dependency between the life remaining and the scale factor
	virtual const Curve* getFunction() const = 0;
	
	/// set the the curve that defines the dependency between the life remaining and the scale factor
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief ScaleVsDensityModifier applies a scale factor function against a single axis versus the density of the particle.
 */
class ScaleVsDensityModifier : public ModifierT<ScaleVsDensityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ScaleVsDensity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the axis along which the scale factor will be applied
	virtual ScaleAxis getScaleAxis() const = 0;
	
	/// set the axis along which the scale factor will be applied
	virtual void setScaleAxis(ScaleAxis a) = 0;
	
	/// get the the curve that defines the dependency between the density and the scale factor
	virtual const Curve* getFunction() const = 0;
	
	/// set the the curve that defines the dependency between the density and the scale factor
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief ScaleVsCameraDistance applies a scale factor against a specific axis based on distance from the camera to the particle.
 */
class ScaleVsCameraDistanceModifier : public ModifierT<ScaleVsCameraDistanceModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ScaleVsCameraDistance;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the axis along which the scale factor will be applied
	virtual ScaleAxis getScaleAxis() const = 0;
	
	/// set the axis along which the scale factor will be applied
	virtual void setScaleAxis(ScaleAxis a) = 0;
	
	/// get the the curve that defines the dependency between the camera distance and the scale factor
	virtual const Curve* getFunction() const = 0;
	
	/// set the the curve that defines the dependency between the camera distance and the scale factor
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief ViewDirectionSortingModifier sorts sprite particles along view direction back to front.
 */
class ViewDirectionSortingModifier : public ModifierT<ViewDirectionSortingModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ViewDirectionSorting;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite;

};

/**
 \brief RotationRateModifier is a modifier to apply a continuous rotation for sprites.
 */
class RotationRateModifier : public ModifierT<RotationRateModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_RotationRate;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite;


	/// set the rotation rate
	virtual float getRotationRate() const = 0;
	
	/// get the rotation rate
	virtual void setRotationRate(const float& rate) = 0;
};

/**
 \brief RotationRateVsLifeModifier is a modifier to adjust the rotation rate versus the life remaining of a particular particle.
 */
class RotationRateVsLifeModifier : public ModifierT<RotationRateVsLifeModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_RotationRateVsLife;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite;


	/// get the curve that sets the dependency between the life remaining and the rotation rate
	virtual const Curve* getFunction() const = 0;
	
	/// set the curve that sets the dependency between the life remaining and the rotation rate
	virtual void setFunction(const Curve* f) = 0;
};

/**
 \brief OrientScaleAlongScreenVelocityModifier is a modifier to orient & scale sprites along the current screen velocity vector.
 */
class OrientScaleAlongScreenVelocityModifier : public ModifierT<OrientScaleAlongScreenVelocityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_OrientScaleAlongScreenVelocity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Continuous
	        | ModifierUsage_Sprite;


	/// get the scale per velocity
	virtual float getScalePerVelocity() const = 0;
	
	/// set the scale per velocity
	virtual void setScalePerVelocity(const float& s) = 0;

	/// get the scale change limit
	virtual float getScaleChangeLimit() const = 0;
	
	/// set the scale change limit
	virtual void setScaleChangeLimit(const float& s) = 0;

	/// get the scale change delay
	virtual float getScaleChangeDelay() const = 0;
	
	/// set the scale change delay
	virtual void setScaleChangeDelay(const float& s) = 0;
};

/**
 \brief ColorVsVelocityModifier modifies the color constants associated with a particle
		depending on the velocity of the particle.
*/
class ColorVsVelocityModifier : public ModifierT<ColorVsVelocityModifier>
{
public:

	/// ModifierType
	static const ModifierTypeEnum ModifierType = ModifierType_ColorVsVelocity;
	
	/// ModifierUsage
	static const uint32_t ModifierUsage = ModifierUsage_Spawn | ModifierUsage_Continuous
	        | ModifierUsage_Sprite | ModifierUsage_Mesh;


	/// get the affected color channel
	virtual ColorChannel getColorChannel() const = 0;
	
	/// set the affected color channel
	virtual void setColorChannel(ColorChannel colorChannel) = 0;
	
	/// get the curve that sets the dependency between the lifetime and the color
	virtual const Curve* getFunction() const = 0;
	
	/// set the curve that sets the dependency between the lifetime and the color
	virtual void setFunction(const Curve* f) = 0;

	/// get velocity0
	virtual float getVelocity0() const = 0;
	
	/// set velocity0
	virtual void setVelocity0(float value) = 0;

	/// get velocity1
	virtual float getVelocity1() const = 0;
	
	/// set velocity1
	virtual void setVelocity1(float value) = 0;
};

#pragma warning( default: 4100 )

PX_POP_PACK

}
} // namespace nvidia

#endif // MODIFIER_H
