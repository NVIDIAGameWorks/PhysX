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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
#ifndef _SAMPLE_VEHICLE_INPUT_EVENT_IDS_H
#define _SAMPLE_VEHICLE_INPUT_EVENT_IDS_H

#include <SampleBaseInputEventIds.h>

// InputEvents used by SampleVehicle
enum SampleVehicleInputEventIds
{
	SAMPLE_VEHICLE_FIRST_ID = NUM_SAMPLE_BASE_INPUT_EVENT_IDS,

	CAR_ACCELERATE_BRAKE,

	//KEYBOARD (car+tank)
	VEH_ACCELERATE_KBD,
	VEH_GEAR_UP_KBD,
	VEH_GEAR_DOWN_KBD,

	VEH_SAVE_KBD,

	//KEYBOARD (car)
	CAR_BRAKE_KBD,
	CAR_HANDBRAKE_KBD,
	CAR_STEER_LEFT_KBD,
	CAR_STEER_RIGHT_KBD,

	//KEYBOARD (tank)
	TANK_THRUST_LEFT_KBD,
	TANK_THRUST_RIGHT_KBD,
	TANK_BRAKE_LEFT_KBD,
	TANK_BRAKE_RIGHT_KBD,

	//KEYBOARD (camera)
	CAMERA_ROTATE_LEFT_KBD,
	CAMERA_ROTATE_RIGHT_KBD,
	CAMERA_ROTATE_UP_KBD,
	CAMERA_ROTATE_DOWN_KBD,

	//GAMEPAD (car+tank)
	VEH_ACCELERATE_PAD,
	VEH_GEAR_UP_PAD,
	VEH_GEAR_DOWN_PAD,

	//GAMEPAD (car)
	CAR_BRAKE_PAD,
	CAR_HANDBRAKE_PAD,
	CAR_STEER_PAD,

	//GAMEPAD (tank)
	TANK_THRUST_LEFT_PAD,
	TANK_THRUST_RIGHT_PAD,
	TANK_BRAKE_LEFT_PAD,
	TANK_BRAKE_RIGHT_PAD,

	//GAMEPAD (camera)
	CAMERA_ROTATE_LEFT_RIGHT_PAD,
	CAMERA_ROTATE_UP_DOWN_PAD,

	//
	AUTOMATIC_GEAR,
	DEBUG_RENDER_FLAG,
	DEBUG_RENDER_WHEEL ,
	DEBUG_RENDER_ENGINE,
	RETRY,
	FIX_CAR,
	CAMERA_LOCK,
	W3MODE,


	NUM_SAMPLE_VEHICLE_INPUT_EVENT_IDS,
};

#endif
