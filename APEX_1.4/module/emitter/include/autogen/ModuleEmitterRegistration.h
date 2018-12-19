/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef MODULE_MODULEEMITTERREGISTRATIONH_H
#define MODULE_MODULEEMITTERREGISTRATIONH_H

#include "PsAllocator.h"
#include "NvRegistrationsForTraitsBase.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PxAssert.h"
#include <stdint.h>

// INCLUDE GENERATED FACTORIES
#include "ApexEmitterAssetParameters.h"
#include "EmitterGeomBoxParams.h"
#include "EmitterGeomExplicitParams.h"
#include "EmitterGeomSphereShellParams.h"
#include "EmitterGeomSphereParams.h"
#include "EmitterGeomCylinderParams.h"
#include "ApexEmitterActorParameters.h"
#include "ImpactEmitterActorParameters.h"
#include "GroundEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"
#include "EmitterDebugRenderParams.h"
#include "GroundEmitterAssetParameters.h"
#include "ImpactEmitterAssetParameters.h"
#include "ImpactExplosionEvent.h"
#include "ImpactObjectEvent.h"
#include "EmitterModuleParameters.h"


// INCLUDE GENERATED CONVERSION


namespace nvidia {
namespace emitter {


class ModuleEmitterRegistration : public NvParameterized::RegistrationsForTraitsBase
{
public:
	static void invokeRegistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleEmitterRegistration().registerAll(*parameterizedTraits);
		}
	}

	static void invokeUnregistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleEmitterRegistration().unregisterAll(*parameterizedTraits);
		}
	}

	void registerAvailableFactories(NvParameterized::Traits& parameterizedTraits)
	{
		::NvParameterized::Factory* factoriesToRegister[] = {
// REGISTER GENERATED FACTORIES
			new nvidia::emitter::ApexEmitterAssetParametersFactory(),
			new nvidia::emitter::EmitterGeomBoxParamsFactory(),
			new nvidia::emitter::EmitterGeomExplicitParamsFactory(),
			new nvidia::emitter::EmitterGeomSphereShellParamsFactory(),
			new nvidia::emitter::EmitterGeomSphereParamsFactory(),
			new nvidia::emitter::EmitterGeomCylinderParamsFactory(),
			new nvidia::emitter::ApexEmitterActorParametersFactory(),
			new nvidia::emitter::ImpactEmitterActorParametersFactory(),
			new nvidia::emitter::GroundEmitterActorParametersFactory(),
			new nvidia::emitter::EmitterAssetPreviewParametersFactory(),
			new nvidia::emitter::EmitterDebugRenderParamsFactory(),
			new nvidia::emitter::GroundEmitterAssetParametersFactory(),
			new nvidia::emitter::ImpactEmitterAssetParametersFactory(),
			new nvidia::emitter::ImpactExplosionEventFactory(),
			new nvidia::emitter::ImpactObjectEventFactory(),
			new nvidia::emitter::EmitterModuleParametersFactory(),

		};

		for (size_t i = 0; i < sizeof(factoriesToRegister)/sizeof(factoriesToRegister[0]); ++i)
		{
			parameterizedTraits.registerFactory(*factoriesToRegister[i]);
		}
	}

	virtual void registerAvailableConverters(NvParameterized::Traits& parameterizedTraits)
	{
// REGISTER GENERATED CONVERSION
PX_UNUSED(parameterizedTraits);

	}

	void unregisterAvailableFactories(NvParameterized::Traits& parameterizedTraits)
	{
		struct FactoryDesc
		{
			const char* name;
			uint32_t version;
		};

		::NvParameterized::Factory* factoriesToUnregister[] = {
// UNREGISTER GENERATED FACTORIES
			new nvidia::emitter::ApexEmitterAssetParametersFactory(),
			new nvidia::emitter::EmitterGeomBoxParamsFactory(),
			new nvidia::emitter::EmitterGeomExplicitParamsFactory(),
			new nvidia::emitter::EmitterGeomSphereShellParamsFactory(),
			new nvidia::emitter::EmitterGeomSphereParamsFactory(),
			new nvidia::emitter::EmitterGeomCylinderParamsFactory(),
			new nvidia::emitter::ApexEmitterActorParametersFactory(),
			new nvidia::emitter::ImpactEmitterActorParametersFactory(),
			new nvidia::emitter::GroundEmitterActorParametersFactory(),
			new nvidia::emitter::EmitterAssetPreviewParametersFactory(),
			new nvidia::emitter::EmitterDebugRenderParamsFactory(),
			new nvidia::emitter::GroundEmitterAssetParametersFactory(),
			new nvidia::emitter::ImpactEmitterAssetParametersFactory(),
			new nvidia::emitter::ImpactExplosionEventFactory(),
			new nvidia::emitter::ImpactObjectEventFactory(),
			new nvidia::emitter::EmitterModuleParametersFactory(),

		};

		for (size_t i = 0; i < sizeof(factoriesToUnregister)/sizeof(factoriesToUnregister[0]); ++i)
		{
			::NvParameterized::Factory* removedFactory = parameterizedTraits.removeFactory(factoriesToUnregister[i]->getClassName(), factoriesToUnregister[i]->getVersion());
			if (!removedFactory) 
			{
				PX_ASSERT_WITH_MESSAGE(0, "Factory can not be removed!");
			}
			else
			{
				removedFactory->freeParameterDefinitionTable(&parameterizedTraits);
				delete removedFactory;
				delete factoriesToUnregister[i];
			}
		}
	}

	virtual void unregisterAvailableConverters(NvParameterized::Traits& parameterizedTraits)
	{
// UNREGISTER GENERATED CONVERSION
PX_UNUSED(parameterizedTraits);

	}

};


}
} //nvidia::emitter

#endif
