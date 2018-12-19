/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef MODULE_MODULEDESTRUCTIBLELEGACYREGISTRATIONH_H
#define MODULE_MODULEDESTRUCTIBLELEGACYREGISTRATIONH_H

#include "PsAllocator.h"
#include "NvRegistrationsForTraitsBase.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PxAssert.h"
#include <stdint.h>

// INCLUDE GENERATED FACTORIES
#include "CachedOverlaps_0p0.h"
#include "DestructibleActorParam_0p0.h"
#include "DestructibleActorParam_0p1.h"
#include "DestructibleActorParam_0p10.h"
#include "DestructibleActorParam_0p11.h"
#include "DestructibleActorParam_0p12.h"
#include "DestructibleActorParam_0p13.h"
#include "DestructibleActorParam_0p14.h"
#include "DestructibleActorParam_0p15.h"
#include "DestructibleActorParam_0p16.h"
#include "DestructibleActorParam_0p17.h"
#include "DestructibleActorParam_0p18.h"
#include "DestructibleActorParam_0p19.h"
#include "DestructibleActorParam_0p2.h"
#include "DestructibleActorParam_0p20.h"
#include "DestructibleActorParam_0p21.h"
#include "DestructibleActorParam_0p22.h"
#include "DestructibleActorParam_0p23.h"
#include "DestructibleActorParam_0p24.h"
#include "DestructibleActorParam_0p25.h"
#include "DestructibleActorParam_0p26.h"
#include "DestructibleActorParam_0p27.h"
#include "DestructibleActorParam_0p28.h"
#include "DestructibleActorParam_0p29.h"
#include "DestructibleActorParam_0p3.h"
#include "DestructibleActorParam_0p30.h"
#include "DestructibleActorParam_0p31.h"
#include "DestructibleActorParam_0p4.h"
#include "DestructibleActorParam_0p5.h"
#include "DestructibleActorParam_0p6.h"
#include "DestructibleActorParam_0p7.h"
#include "DestructibleActorParam_0p8.h"
#include "DestructibleActorParam_0p9.h"
#include "DestructibleActorState_0p0.h"
#include "DestructibleActorState_0p1.h"
#include "DestructibleActorState_0p2.h"
#include "DestructibleActorState_0p3.h"
#include "DestructibleAssetParameters_0p0.h"
#include "DestructibleAssetParameters_0p1.h"
#include "DestructibleAssetParameters_0p10.h"
#include "DestructibleAssetParameters_0p11.h"
#include "DestructibleAssetParameters_0p12.h"
#include "DestructibleAssetParameters_0p13.h"
#include "DestructibleAssetParameters_0p14.h"
#include "DestructibleAssetParameters_0p15.h"
#include "DestructibleAssetParameters_0p16.h"
#include "DestructibleAssetParameters_0p17.h"
#include "DestructibleAssetParameters_0p18.h"
#include "DestructibleAssetParameters_0p19.h"
#include "DestructibleAssetParameters_0p2.h"
#include "DestructibleAssetParameters_0p20.h"
#include "DestructibleAssetParameters_0p21.h"
#include "DestructibleAssetParameters_0p22.h"
#include "DestructibleAssetParameters_0p23.h"
#include "DestructibleAssetParameters_0p24.h"
#include "DestructibleAssetParameters_0p25.h"
#include "DestructibleAssetParameters_0p3.h"
#include "DestructibleAssetParameters_0p4.h"
#include "DestructibleAssetParameters_0p5.h"
#include "DestructibleAssetParameters_0p6.h"
#include "DestructibleAssetParameters_0p7.h"
#include "DestructibleAssetParameters_0p8.h"
#include "DestructibleAssetParameters_0p9.h"
#include "DestructibleModuleParameters_0p0.h"
#include "DestructibleModuleParameters_0p1.h"
#include "DestructibleModuleParameters_0p2.h"
#include "SurfaceTraceSetParameters_0p0.h"
#include "SurfaceTraceSetParameters_0p1.h"
#include "DestructibleActorParam_0p32.h"
#include "DestructibleActorChunks_0p0.h"
#include "DestructibleActorState_0p4.h"
#include "SurfaceTraceParameters_0p0.h"
#include "SurfaceTraceSetParameters_0p2.h"
#include "CachedOverlaps_0p1.h"
#include "MeshCookedCollisionStream_0p0.h"
#include "MeshCookedCollisionStreamsAtScale_0p0.h"
#include "DestructibleAssetCollisionDataSet_0p0.h"
#include "DestructibleAssetParameters_0p26.h"
#include "DestructiblePreviewParam_0p0.h"
#include "DestructibleDebugRenderParams_0p0.h"
#include "DestructibleModuleParameters_0p3.h"


// INCLUDE GENERATED CONVERSION
#include "ConversionCachedOverlaps_0p0_0p1.h"
#include "ConversionDestructibleActorParam_0p0_0p1.h"
#include "ConversionDestructibleActorParam_0p10_0p11.h"
#include "ConversionDestructibleActorParam_0p11_0p12.h"
#include "ConversionDestructibleActorParam_0p12_0p13.h"
#include "ConversionDestructibleActorParam_0p13_0p14.h"
#include "ConversionDestructibleActorParam_0p14_0p15.h"
#include "ConversionDestructibleActorParam_0p15_0p16.h"
#include "ConversionDestructibleActorParam_0p16_0p17.h"
#include "ConversionDestructibleActorParam_0p17_0p18.h"
#include "ConversionDestructibleActorParam_0p18_0p19.h"
#include "ConversionDestructibleActorParam_0p19_0p20.h"
#include "ConversionDestructibleActorParam_0p1_0p2.h"
#include "ConversionDestructibleActorParam_0p20_0p21.h"
#include "ConversionDestructibleActorParam_0p21_0p22.h"
#include "ConversionDestructibleActorParam_0p22_0p23.h"
#include "ConversionDestructibleActorParam_0p23_0p24.h"
#include "ConversionDestructibleActorParam_0p24_0p25.h"
#include "ConversionDestructibleActorParam_0p25_0p26.h"
#include "ConversionDestructibleActorParam_0p26_0p27.h"
#include "ConversionDestructibleActorParam_0p27_0p28.h"
#include "ConversionDestructibleActorParam_0p28_0p29.h"
#include "ConversionDestructibleActorParam_0p29_0p30.h"
#include "ConversionDestructibleActorParam_0p2_0p3.h"
#include "ConversionDestructibleActorParam_0p30_0p31.h"
#include "ConversionDestructibleActorParam_0p31_0p32.h"
#include "ConversionDestructibleActorParam_0p3_0p4.h"
#include "ConversionDestructibleActorParam_0p4_0p5.h"
#include "ConversionDestructibleActorParam_0p5_0p6.h"
#include "ConversionDestructibleActorParam_0p6_0p7.h"
#include "ConversionDestructibleActorParam_0p7_0p8.h"
#include "ConversionDestructibleActorParam_0p8_0p9.h"
#include "ConversionDestructibleActorParam_0p9_0p10.h"
#include "ConversionDestructibleActorState_0p0_0p1.h"
#include "ConversionDestructibleActorState_0p1_0p2.h"
#include "ConversionDestructibleActorState_0p2_0p3.h"
#include "ConversionDestructibleActorState_0p3_0p4.h"
#include "ConversionDestructibleAssetParameters_0p0_0p1.h"
#include "ConversionDestructibleAssetParameters_0p10_0p11.h"
#include "ConversionDestructibleAssetParameters_0p11_0p12.h"
#include "ConversionDestructibleAssetParameters_0p12_0p13.h"
#include "ConversionDestructibleAssetParameters_0p13_0p14.h"
#include "ConversionDestructibleAssetParameters_0p14_0p15.h"
#include "ConversionDestructibleAssetParameters_0p15_0p16.h"
#include "ConversionDestructibleAssetParameters_0p16_0p17.h"
#include "ConversionDestructibleAssetParameters_0p17_0p18.h"
#include "ConversionDestructibleAssetParameters_0p18_0p19.h"
#include "ConversionDestructibleAssetParameters_0p19_0p20.h"
#include "ConversionDestructibleAssetParameters_0p1_0p2.h"
#include "ConversionDestructibleAssetParameters_0p20_0p21.h"
#include "ConversionDestructibleAssetParameters_0p21_0p22.h"
#include "ConversionDestructibleAssetParameters_0p22_0p23.h"
#include "ConversionDestructibleAssetParameters_0p23_0p24.h"
#include "ConversionDestructibleAssetParameters_0p24_0p25.h"
#include "ConversionDestructibleAssetParameters_0p25_0p26.h"
#include "ConversionDestructibleAssetParameters_0p2_0p3.h"
#include "ConversionDestructibleAssetParameters_0p3_0p4.h"
#include "ConversionDestructibleAssetParameters_0p4_0p5.h"
#include "ConversionDestructibleAssetParameters_0p5_0p6.h"
#include "ConversionDestructibleAssetParameters_0p6_0p7.h"
#include "ConversionDestructibleAssetParameters_0p7_0p8.h"
#include "ConversionDestructibleAssetParameters_0p8_0p9.h"
#include "ConversionDestructibleAssetParameters_0p9_0p10.h"
#include "ConversionDestructibleModuleParameters_0p0_0p1.h"
#include "ConversionDestructibleModuleParameters_0p1_0p2.h"
#include "ConversionDestructibleModuleParameters_0p2_0p3.h"
#include "ConversionSurfaceTraceSetParameters_0p0_0p1.h"
#include "ConversionSurfaceTraceSetParameters_0p1_0p2.h"


// global namespace

class ModuleDestructibleLegacyRegistration : public NvParameterized::RegistrationsForTraitsBase
{
public:
	static void invokeRegistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleDestructibleLegacyRegistration().registerAll(*parameterizedTraits);
		}
	}

	static void invokeUnregistration(NvParameterized::Traits* parameterizedTraits)
	{
		if (parameterizedTraits)
		{
			ModuleDestructibleLegacyRegistration().unregisterAll(*parameterizedTraits);
		}
	}

	void registerAvailableFactories(NvParameterized::Traits& parameterizedTraits)
	{
		::NvParameterized::Factory* factoriesToRegister[] = {
// REGISTER GENERATED FACTORIES
			new nvidia::parameterized::CachedOverlaps_0p0Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p0Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p1Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p10Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p11Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p12Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p13Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p14Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p15Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p16Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p17Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p18Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p19Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p2Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p20Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p21Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p22Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p23Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p24Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p25Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p26Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p27Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p28Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p29Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p3Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p30Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p31Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p4Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p5Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p6Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p7Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p8Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p9Factory(),
			new nvidia::parameterized::DestructibleActorState_0p0Factory(),
			new nvidia::parameterized::DestructibleActorState_0p1Factory(),
			new nvidia::parameterized::DestructibleActorState_0p2Factory(),
			new nvidia::parameterized::DestructibleActorState_0p3Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p0Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p1Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p10Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p11Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p12Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p13Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p14Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p15Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p16Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p17Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p18Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p19Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p2Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p20Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p21Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p22Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p23Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p24Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p25Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p3Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p4Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p5Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p6Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p7Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p8Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p9Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p0Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p1Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p2Factory(),
			new nvidia::parameterized::SurfaceTraceSetParameters_0p0Factory(),
			new nvidia::parameterized::SurfaceTraceSetParameters_0p1Factory(),

		};

		for (size_t i = 0; i < sizeof(factoriesToRegister)/sizeof(factoriesToRegister[0]); ++i)
		{
			parameterizedTraits.registerFactory(*factoriesToRegister[i]);
		}
	}

	virtual void registerAvailableConverters(NvParameterized::Traits& parameterizedTraits)
	{
// REGISTER GENERATED CONVERSION
			{
			typedef nvidia::apex::legacy::ConversionCachedOverlaps_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p10_0p11 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p11_0p12 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p12_0p13 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p13_0p14 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p14_0p15 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p15_0p16 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p16_0p17 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p17_0p18 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p18_0p19 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p19_0p20 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p1_0p2 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p20_0p21 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p21_0p22 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p22_0p23 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p23_0p24 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p24_0p25 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p25_0p26 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p26_0p27 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p27_0p28 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p28_0p29 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p29_0p30 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p2_0p3 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p30_0p31 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p31_0p32 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p3_0p4 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p4_0p5 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p5_0p6 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p6_0p7 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p7_0p8 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p8_0p9 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p9_0p10 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p1_0p2 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p2_0p3 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p3_0p4 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p10_0p11 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p11_0p12 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p12_0p13 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p13_0p14 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p14_0p15 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p15_0p16 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p16_0p17 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p17_0p18 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p18_0p19 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p19_0p20 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p1_0p2 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p20_0p21 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p21_0p22 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p22_0p23 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p23_0p24 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p24_0p25 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p25_0p26 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p2_0p3 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p3_0p4 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p4_0p5 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p5_0p6 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p6_0p7 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p7_0p8 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p8_0p9 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p9_0p10 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p1_0p2 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p2_0p3 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionSurfaceTraceSetParameters_0p0_0p1 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}
			{
			typedef nvidia::apex::legacy::ConversionSurfaceTraceSetParameters_0p1_0p2 ConverterToRegister;
			parameterizedTraits.registerConversion(ConverterToRegister::TOldClass::staticClassName(),
								ConverterToRegister::TOldClass::ClassVersion,
								ConverterToRegister::TNewClass::ClassVersion,
								*(ConverterToRegister::Create(&parameterizedTraits)));
			}

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
			new nvidia::parameterized::CachedOverlaps_0p0Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p0Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p1Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p10Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p11Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p12Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p13Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p14Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p15Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p16Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p17Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p18Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p19Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p2Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p20Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p21Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p22Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p23Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p24Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p25Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p26Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p27Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p28Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p29Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p3Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p30Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p31Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p4Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p5Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p6Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p7Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p8Factory(),
			new nvidia::parameterized::DestructibleActorParam_0p9Factory(),
			new nvidia::parameterized::DestructibleActorState_0p0Factory(),
			new nvidia::parameterized::DestructibleActorState_0p1Factory(),
			new nvidia::parameterized::DestructibleActorState_0p2Factory(),
			new nvidia::parameterized::DestructibleActorState_0p3Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p0Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p1Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p10Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p11Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p12Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p13Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p14Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p15Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p16Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p17Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p18Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p19Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p2Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p20Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p21Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p22Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p23Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p24Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p25Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p3Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p4Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p5Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p6Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p7Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p8Factory(),
			new nvidia::parameterized::DestructibleAssetParameters_0p9Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p0Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p1Factory(),
			new nvidia::parameterized::DestructibleModuleParameters_0p2Factory(),
			new nvidia::parameterized::SurfaceTraceSetParameters_0p0Factory(),
			new nvidia::parameterized::SurfaceTraceSetParameters_0p1Factory(),

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
			{
			typedef nvidia::apex::legacy::ConversionCachedOverlaps_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p10_0p11 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p11_0p12 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p12_0p13 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p13_0p14 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p14_0p15 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p15_0p16 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p16_0p17 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p17_0p18 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p18_0p19 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p19_0p20 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p1_0p2 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p20_0p21 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p21_0p22 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p22_0p23 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p23_0p24 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p24_0p25 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p25_0p26 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p26_0p27 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p27_0p28 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p28_0p29 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p29_0p30 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p2_0p3 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p30_0p31 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p31_0p32 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p3_0p4 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p4_0p5 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p5_0p6 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p6_0p7 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p7_0p8 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p8_0p9 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorParam_0p9_0p10 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p1_0p2 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p2_0p3 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleActorState_0p3_0p4 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p10_0p11 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p11_0p12 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p12_0p13 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p13_0p14 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p14_0p15 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p15_0p16 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p16_0p17 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p17_0p18 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p18_0p19 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p19_0p20 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p1_0p2 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p20_0p21 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p21_0p22 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p22_0p23 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p23_0p24 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p24_0p25 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p25_0p26 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p2_0p3 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p3_0p4 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p4_0p5 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p5_0p6 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p6_0p7 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p7_0p8 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p8_0p9 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleAssetParameters_0p9_0p10 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p1_0p2 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionDestructibleModuleParameters_0p2_0p3 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionSurfaceTraceSetParameters_0p0_0p1 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}
			{
			typedef nvidia::apex::legacy::ConversionSurfaceTraceSetParameters_0p1_0p2 ConverterToUnregister;
			::NvParameterized::Conversion* removedConv = parameterizedTraits.removeConversion(ConverterToUnregister::TOldClass::staticClassName(),
								ConverterToUnregister::TOldClass::ClassVersion,
								ConverterToUnregister::TNewClass::ClassVersion);
			if (removedConv) {
				removedConv->~Conversion(); parameterizedTraits.free(removedConv); // PLACEMENT DELETE 
			} else {
				// assert("Conversion was not found");
			}
			}

	}

};

// global namespace

#endif
