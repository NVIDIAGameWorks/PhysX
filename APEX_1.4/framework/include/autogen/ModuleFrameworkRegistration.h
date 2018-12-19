/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef MODULE_MODULEFRAMEWORKREGISTRATIONH_H
#define MODULE_MODULEFRAMEWORKREGISTRATIONH_H

#include "PsAllocator.h"
#include "NvRegistrationsForTraitsBase.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PxAssert.h"
#include <stdint.h>

// INCLUDE GENERATED FACTORIES
#include "VertexFormatParameters.h"
#include "VertexBufferParameters.h"
#include "SurfaceBufferParameters.h"
#include "SubmeshParameters.h"
#include "RenderMeshAssetParameters.h"
#include "BufferU8x1.h"
#include "BufferU8x2.h"
#include "BufferU8x3.h"
#include "BufferU8x4.h"
#include "BufferU16x1.h"
#include "BufferU16x2.h"
#include "BufferU16x3.h"
#include "BufferU16x4.h"
#include "BufferU32x1.h"
#include "BufferU32x2.h"
#include "BufferU32x3.h"
#include "BufferU32x4.h"
#include "BufferF32x1.h"
#include "BufferF32x2.h"
#include "BufferF32x3.h"
#include "BufferF32x4.h"


// INCLUDE GENERATED CONVERSION


namespace nvidia {
namespace apex {


class ModuleFrameworkRegistration : public NvParameterized::RegistrationsForTraitsBase
{
public:
	static void invokeRegistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleFrameworkRegistration().registerAll(*parameterizedTraits);
		}
	}

	static void invokeUnregistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleFrameworkRegistration().unregisterAll(*parameterizedTraits);
		}
	}

	void registerAvailableFactories(NvParameterized::Traits& parameterizedTraits)
	{
		::NvParameterized::Factory* factoriesToRegister[] = {
// REGISTER GENERATED FACTORIES
			new nvidia::apex::VertexFormatParametersFactory(),
			new nvidia::apex::VertexBufferParametersFactory(),
			new nvidia::apex::SurfaceBufferParametersFactory(),
			new nvidia::apex::SubmeshParametersFactory(),
			new nvidia::apex::RenderMeshAssetParametersFactory(),
			new nvidia::apex::BufferU8x1Factory(),
			new nvidia::apex::BufferU8x2Factory(),
			new nvidia::apex::BufferU8x3Factory(),
			new nvidia::apex::BufferU8x4Factory(),
			new nvidia::apex::BufferU16x1Factory(),
			new nvidia::apex::BufferU16x2Factory(),
			new nvidia::apex::BufferU16x3Factory(),
			new nvidia::apex::BufferU16x4Factory(),
			new nvidia::apex::BufferU32x1Factory(),
			new nvidia::apex::BufferU32x2Factory(),
			new nvidia::apex::BufferU32x3Factory(),
			new nvidia::apex::BufferU32x4Factory(),
			new nvidia::apex::BufferF32x1Factory(),
			new nvidia::apex::BufferF32x2Factory(),
			new nvidia::apex::BufferF32x3Factory(),
			new nvidia::apex::BufferF32x4Factory(),

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
			new nvidia::apex::VertexFormatParametersFactory(),
			new nvidia::apex::VertexBufferParametersFactory(),
			new nvidia::apex::SurfaceBufferParametersFactory(),
			new nvidia::apex::SubmeshParametersFactory(),
			new nvidia::apex::RenderMeshAssetParametersFactory(),
			new nvidia::apex::BufferU8x1Factory(),
			new nvidia::apex::BufferU8x2Factory(),
			new nvidia::apex::BufferU8x3Factory(),
			new nvidia::apex::BufferU8x4Factory(),
			new nvidia::apex::BufferU16x1Factory(),
			new nvidia::apex::BufferU16x2Factory(),
			new nvidia::apex::BufferU16x3Factory(),
			new nvidia::apex::BufferU16x4Factory(),
			new nvidia::apex::BufferU32x1Factory(),
			new nvidia::apex::BufferU32x2Factory(),
			new nvidia::apex::BufferU32x3Factory(),
			new nvidia::apex::BufferU32x4Factory(),
			new nvidia::apex::BufferF32x1Factory(),
			new nvidia::apex::BufferF32x2Factory(),
			new nvidia::apex::BufferF32x3Factory(),
			new nvidia::apex::BufferF32x4Factory(),

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
} //nvidia::apex

#endif
