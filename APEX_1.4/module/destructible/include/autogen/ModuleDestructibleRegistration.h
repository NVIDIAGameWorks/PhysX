/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef MODULE_MODULEDESTRUCTIBLEREGISTRATIONH_H
#define MODULE_MODULEDESTRUCTIBLEREGISTRATIONH_H

#include "PsAllocator.h"
#include "NvRegistrationsForTraitsBase.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PxAssert.h"
#include <stdint.h>

// INCLUDE GENERATED FACTORIES
#include "DestructibleActorParam.h"
#include "DestructibleActorChunks.h"
#include "DestructibleActorState.h"
#include "SurfaceTraceParameters.h"
#include "SurfaceTraceSetParameters.h"
#include "CachedOverlaps.h"
#include "MeshCookedCollisionStream.h"
#include "MeshCookedCollisionStreamsAtScale.h"
#include "DestructibleAssetCollisionDataSet.h"
#include "DestructibleAssetParameters.h"
#include "DestructiblePreviewParam.h"
#include "DestructibleDebugRenderParams.h"
#include "DestructibleModuleParameters.h"


// INCLUDE GENERATED CONVERSION


namespace nvidia {
namespace destructible {


class ModuleDestructibleRegistration : public NvParameterized::RegistrationsForTraitsBase
{
public:
	static void invokeRegistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleDestructibleRegistration().registerAll(*parameterizedTraits);
		}
	}

	static void invokeUnregistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleDestructibleRegistration().unregisterAll(*parameterizedTraits);
		}
	}

	void registerAvailableFactories(NvParameterized::Traits& parameterizedTraits)
	{
		::NvParameterized::Factory* factoriesToRegister[] = {
// REGISTER GENERATED FACTORIES
			new nvidia::destructible::DestructibleActorParamFactory(),
			new nvidia::destructible::DestructibleActorChunksFactory(),
			new nvidia::destructible::DestructibleActorStateFactory(),
			new nvidia::destructible::SurfaceTraceParametersFactory(),
			new nvidia::destructible::SurfaceTraceSetParametersFactory(),
			new nvidia::destructible::CachedOverlapsFactory(),
			new nvidia::destructible::MeshCookedCollisionStreamFactory(),
			new nvidia::destructible::MeshCookedCollisionStreamsAtScaleFactory(),
			new nvidia::destructible::DestructibleAssetCollisionDataSetFactory(),
			new nvidia::destructible::DestructibleAssetParametersFactory(),
			new nvidia::destructible::DestructiblePreviewParamFactory(),
			new nvidia::destructible::DestructibleDebugRenderParamsFactory(),
			new nvidia::destructible::DestructibleModuleParametersFactory(),

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
			new nvidia::destructible::DestructibleActorParamFactory(),
			new nvidia::destructible::DestructibleActorChunksFactory(),
			new nvidia::destructible::DestructibleActorStateFactory(),
			new nvidia::destructible::SurfaceTraceParametersFactory(),
			new nvidia::destructible::SurfaceTraceSetParametersFactory(),
			new nvidia::destructible::CachedOverlapsFactory(),
			new nvidia::destructible::MeshCookedCollisionStreamFactory(),
			new nvidia::destructible::MeshCookedCollisionStreamsAtScaleFactory(),
			new nvidia::destructible::DestructibleAssetCollisionDataSetFactory(),
			new nvidia::destructible::DestructibleAssetParametersFactory(),
			new nvidia::destructible::DestructiblePreviewParamFactory(),
			new nvidia::destructible::DestructibleDebugRenderParamsFactory(),
			new nvidia::destructible::DestructibleModuleParametersFactory(),

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
} //nvidia::destructible

#endif
