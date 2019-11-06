##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##  * Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
##  * Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##  * Neither the name of NVIDIA CORPORATION nor the names of its
##    contributors may be used to endorse or promote products derived
##    from this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
## EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
## PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Copyright (c) 2018-2019 NVIDIA Corporation. All rights reserved.

#
# Build PhysXVehicle common
#

SET(PHYSX_SOURCE_DIR ${PHYSX_ROOT_DIR}/source)
SET(LL_SOURCE_DIR ${PHYSX_SOURCE_DIR}/physxvehicle/src)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PHYSX_ROOT_DIR}/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/PhysXVehicle.cmake)



SET(PHYSX_VEHICLE_HEADERS
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleComponents.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleDrive.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleDrive4W.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleDriveNW.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleDriveTank.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleNoDrive.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleSDK.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleShaders.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleTireFriction.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleUpdate.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleUtil.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleUtilControl.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleUtilSetup.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleUtilTelemetry.h
	${PHYSX_ROOT_DIR}/include/vehicle/PxVehicleWheels.h
)
SOURCE_GROUP(include FILES ${PHYSX_VEHICLE_HEADERS})

SET(PHYSX_VEHICLE_SOURCE
	${LL_SOURCE_DIR}/PxVehicleComponents.cpp
	${LL_SOURCE_DIR}/PxVehicleDrive.cpp
	${LL_SOURCE_DIR}/PxVehicleDrive4W.cpp
	${LL_SOURCE_DIR}/PxVehicleDriveNW.cpp
	${LL_SOURCE_DIR}/PxVehicleDriveTank.cpp
	${LL_SOURCE_DIR}/PxVehicleMetaData.cpp
	${LL_SOURCE_DIR}/PxVehicleNoDrive.cpp
	${LL_SOURCE_DIR}/PxVehicleSDK.cpp
	${LL_SOURCE_DIR}/PxVehicleSerialization.cpp
	${LL_SOURCE_DIR}/PxVehicleSuspWheelTire4.cpp
	${LL_SOURCE_DIR}/PxVehicleTireFriction.cpp
	${LL_SOURCE_DIR}/PxVehicleUpdate.cpp
	${LL_SOURCE_DIR}/PxVehicleWheels.cpp
	${LL_SOURCE_DIR}/VehicleUtilControl.cpp
	${LL_SOURCE_DIR}/VehicleUtilSetup.cpp
	${LL_SOURCE_DIR}/VehicleUtilTelemetry.cpp
	${LL_SOURCE_DIR}/PxVehicleDefaults.h
	${LL_SOURCE_DIR}/PxVehicleLinearMath.h
	${LL_SOURCE_DIR}/PxVehicleSerialization.h
	${LL_SOURCE_DIR}/PxVehicleSuspLimitConstraintShader.h
	${LL_SOURCE_DIR}/PxVehicleSuspWheelTire4.h
)
SOURCE_GROUP(src FILES ${PHYSX_VEHICLE_SOURCE})

SET(PHYSX_VEHICLE_METADATA_HEADERS
	${LL_SOURCE_DIR}/physxmetadata/include/PxVehicleAutoGeneratedMetaDataObjectNames.h
	${LL_SOURCE_DIR}/physxmetadata/include/PxVehicleAutoGeneratedMetaDataObjects.h
	${LL_SOURCE_DIR}/physxmetadata/include/PxVehicleMetaDataObjects.h
)
SOURCE_GROUP(metadata\\include FILES ${PHYSX_VEHICLE_METADATA_HEADERS})

SET(PHYSX_VEHICLE_METADATA_SOURCE
	${LL_SOURCE_DIR}/physxmetadata/src/PxVehicleAutoGeneratedMetaDataObjects.cpp
	${LL_SOURCE_DIR}/physxmetadata/src/PxVehicleMetaDataObjects.cpp
)
SOURCE_GROUP(metadata\\src FILES ${PHYSX_VEHICLE_METADATA_SOURCE})

ADD_LIBRARY(PhysXVehicle ${PHYSXVEHICLE_LIBTYPE}
	${PHYSX_VEHICLE_SOURCE}
	${PHYSX_VEHICLE_HEADERS}
	${PHYSX_VEHICLE_METADATA_HEADERS}
	${PHYSX_VEHICLE_METADATA_SOURCE}
)

INSTALL(FILES ${PHYSX_VEHICLE_HEADERS} DESTINATION ${PHYSX_INSTALL_PREFIX}/include/vehicle)

TARGET_INCLUDE_DIRECTORIES(PhysXVehicle 
	PRIVATE ${PHYSXVEHICLE_PLATFORM_INCLUDES}

	PRIVATE ${PHYSX_ROOT_DIR}/include

	PRIVATE ${PHYSX_SOURCE_DIR}/common/include
	PRIVATE ${PHYSX_SOURCE_DIR}/common/src

	PRIVATE ${PHYSX_SOURCE_DIR}/physxvehicle/src
	PRIVATE ${PHYSX_SOURCE_DIR}/physxvehicle/src/physxmetadata/include

	PRIVATE ${PHYSX_SOURCE_DIR}/physxmetadata/extensions/include
	PRIVATE ${PHYSX_SOURCE_DIR}/physxmetadata/core/include

	PRIVATE ${PHYSX_SOURCE_DIR}/physxextensions/src/serialization/Xml

	PRIVATE ${PHYSX_SOURCE_DIR}/pvdsdk/src
	
	PRIVATE ${PHYSX_SOURCE_DIR}/pvd/include	
)

# No linked libraries

# Use generator expressions to set config specific preprocessor definitions
TARGET_COMPILE_DEFINITIONS(PhysXVehicle 
	PRIVATE ${PHYSXVEHICLE_COMPILE_DEFS}
)

SET_TARGET_PROPERTIES(PhysXVehicle PROPERTIES
	OUTPUT_NAME PhysXVehicle
)
	

IF(NV_USE_GAMEWORKS_OUTPUT_DIRS AND PHYSXVEHICLE_LIBTYPE STREQUAL "STATIC")	
	SET_TARGET_PROPERTIES(PhysXVehicle PROPERTIES 
		ARCHIVE_OUTPUT_NAME_DEBUG "PhysXVehicle_static"
		ARCHIVE_OUTPUT_NAME_CHECKED "PhysXVehicle_static"
		ARCHIVE_OUTPUT_NAME_PROFILE "PhysXVehicle_static"
		ARCHIVE_OUTPUT_NAME_RELEASE "PhysXVehicle_static"
	)
ENDIF()

IF(PHYSXVEHICLE_COMPILE_PDB_NAME_DEBUG)
	SET_TARGET_PROPERTIES(PhysXVehicle PROPERTIES 
		COMPILE_PDB_NAME_DEBUG ${PHYSXVEHICLE_COMPILE_PDB_NAME_DEBUG}
		COMPILE_PDB_NAME_CHECKED ${PHYSXVEHICLE_COMPILE_PDB_NAME_CHECKED}
		COMPILE_PDB_NAME_PROFILE ${PHYSXVEHICLE_COMPILE_PDB_NAME_PROFILE}
		COMPILE_PDB_NAME_RELEASE ${PHYSXVEHICLE_COMPILE_PDB_NAME_RELEASE}
	)
ENDIF()

TARGET_LINK_LIBRARIES(PhysXVehicle
	PUBLIC ${PHYSXVEHICLE_PLATFORM_LINKED_LIBS} PhysXFoundation PhysXPvdSDK
)

IF(PX_GENERATE_SOURCE_DISTRO)			
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${PHYSX_VEHICLE_SOURCE})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${PHYSX_VEHICLE_HEADERS})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${PHYSX_VEHICLE_METADATA_HEADERS})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${PHYSX_VEHICLE_METADATA_SOURCE})
ENDIF()

# enable -fPIC so we can link static libs with the editor
SET_TARGET_PROPERTIES(PhysXVehicle PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
