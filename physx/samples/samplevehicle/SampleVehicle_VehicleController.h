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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef SAMPLE_VEHICLE_VEHICLE_CONTROLLER_H
#define SAMPLE_VEHICLE_VEHICLE_CONTROLLER_H

#include "common/PxPhysXCommonConfig.h"
#include "foundation/PxVec3.h"
#include "vehicle/PxVehicleSDK.h"
#include "vehicle/PxVehicleUpdate.h"
#include "vehicle/PxVehicleUtilControl.h"

using namespace physx;

class SampleVehicle_VehicleController
{
public:

	SampleVehicle_VehicleController();
	~SampleVehicle_VehicleController();

	void setCarKeyboardInputs
		(const bool accel, const bool brake, const bool handbrake, 
		 const bool steerleft, const bool steerright, 
		 const bool gearup, const bool geardown)
	{
		mKeyPressedAccel=accel;
		mKeyPressedBrake=brake;
		mKeyPressedHandbrake=handbrake;
		mKeyPressedSteerLeft=steerleft;
		mKeyPressedSteerRight=steerright;
		mKeyPressedGearUp=gearup;
		mKeyPressedGearDown=geardown;
	}

	void setCarGamepadInputs
		(const PxF32 accel, const PxF32 brake, 
		 const PxF32 steer, 
		 const bool gearup, const bool geardown, 
		 const bool handbrake)
	{
		mGamepadAccel=accel;
		mGamepadCarBrake=brake;
		mGamepadCarSteer=steer;
		mGamepadGearup=gearup;
		mGamepadGeardown=geardown;
		mGamepadCarHandbrake=handbrake;
	}

	void setTankKeyboardInputs
		(const bool accel, const bool thrustLeft, const bool thrustRight, const bool brakeLeft, const bool brakeRight, const bool gearUp, const bool gearDown)
	{
		mKeyPressedAccel=accel;
		mKeyPressedThrustLeft=thrustLeft;
		mKeyPressedThrustRight=thrustRight;
		mKeyPressedBrakeLeft=brakeLeft;
		mKeyPressedBrakeRight=brakeRight;
		mKeyPressedGearUp=gearUp;
		mKeyPressedGearDown=gearDown;
	}

	void setTankGamepadInputs
		(const PxF32 accel, const PxF32 thrustLeft, const PxF32 thrustRight, const PxF32 brakeLeft, const PxF32 brakeRight, const bool gearUp, const bool gearDown)
	{
		mGamepadAccel=accel;
		mTankThrustLeft=thrustLeft;
		mTankThrustRight=thrustRight;
		mTankBrakeLeft=brakeLeft;
		mTankBrakeRight=brakeRight;
		mGamepadGearup=gearUp;
		mGamepadGeardown=gearDown;
	}

	void toggleAutoGearFlag() 
	{
		mToggleAutoGears = true;
	}

	void update(const PxF32 dtime, const PxVehicleWheelQueryResult& vehicleWheelQueryResults, PxVehicleWheels& focusVehicle);

	void clear();

private:

	//Raw driving inputs - keys (car + tank)
	bool			mKeyPressedAccel;
	bool			mKeyPressedGearUp;
	bool			mKeyPressedGearDown;

	//Raw driving inputs - keys (car only)
	bool			mKeyPressedBrake;
	bool			mKeyPressedHandbrake;
	bool			mKeyPressedSteerLeft;
	bool			mKeyPressedSteerRight;

	//Raw driving inputs - keys (tank only)
	bool			mKeyPressedThrustLeft;
	bool			mKeyPressedThrustRight;
	bool			mKeyPressedBrakeLeft;
	bool			mKeyPressedBrakeRight;

	//Raw driving inputs - gamepad (car + tank)
	PxF32			mGamepadAccel;
	bool			mGamepadGearup;
	bool			mGamepadGeardown;

	//Raw driving inputs - gamepad (car only)
	PxF32			mGamepadCarBrake;
	PxF32			mGamepadCarSteer;
	bool			mGamepadCarHandbrake;

	//Raw driving inputs - (tank only)
	PxF32			mTankThrustLeft;
	PxF32			mTankThrustRight;
	PxF32			mTankBrakeLeft;
	PxF32			mTankBrakeRight;

	//Record and replay using raw driving inputs.
	bool			mRecord;
	bool			mReplay;
	enum
	{
		MAX_NUM_RECORD_REPLAY_SAMPLES=8192
	};
	// Keyboard
	bool			mKeyboardAccelValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardBrakeValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardHandbrakeValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardSteerLeftValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardSteerRightValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardGearupValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mKeyboardGeardownValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	// Gamepad - (tank + car)
	PxF32			mGamepadAccelValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mGamepadGearupValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mGamepadGeardownValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	// Gamepad - car only
	PxF32			mGamepadCarBrakeValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	PxF32			mGamepadCarSteerValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	bool			mGamepadCarHandbrakeValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	//Gamepad - tank only.
	PxF32			mGamepadTankThrustLeftValues[MAX_NUM_RECORD_REPLAY_SAMPLES];
	PxF32			mGamepadTankThrustRightValues[MAX_NUM_RECORD_REPLAY_SAMPLES];

	PxU32			mNumSamples;
	PxU32			mNumRecordedSamples;

	// Raw data taken from the correct stream (live input stream or replay stream)
	bool			mUseKeyInputs;

	// Toggle autogears flag on focus vehicle
	bool			mToggleAutoGears;

	//Auto-reverse mode.
	bool			mIsMovingForwardSlowly;
	bool			mInReverseMode;

	//Update 
	void processRawInputs(const PxF32 timestep, const bool useAutoGears, PxVehicleDrive4WRawInputData& rawInputData);
	void processRawInputs(const PxF32 timestep, const bool useAutoGears, PxVehicleDriveTankRawInputData& rawInputData);
	void processAutoReverse(
		const PxVehicleWheels& focusVehicle, const PxVehicleDriveDynData& driveDynData, const PxVehicleWheelQueryResult& vehicleWheelQueryResults,
		const PxVehicleDrive4WRawInputData& rawInputData, 
		bool& toggleAutoReverse, bool& newIsMovingForwardSlowly) const;
	void processAutoReverse(
		const PxVehicleWheels& focusVehicle, const PxVehicleDriveDynData& driveDynData, const PxVehicleWheelQueryResult& vehicleWheelQueryResults,
		const PxVehicleDriveTankRawInputData& rawInputData, 
		bool& toggleAutoReverse, bool& newIsMovingForwardSlowly) const;

	////////////////////////////////
	//Record and replay deprecated at the moment.
	//Setting functions as private to avoid them being used.
	///////////////////////////////
	bool getIsInRecordReplayMode() const {return (mRecord || mReplay);}
	bool getIsRecording() const {return mRecord;}
	bool getIsReplaying() const {return mReplay;}

	void enableRecordReplayMode()
	{
		PX_ASSERT(!getIsInRecordReplayMode());
		mRecord=true;
		mReplay=false;
		mNumRecordedSamples=0;
	}

	void disableRecordReplayMode()
	{
		PX_ASSERT(getIsInRecordReplayMode());
		mRecord=false;
		mReplay=false;
		mNumRecordedSamples=0;
	}

	void restart();
	////////////////////////////

};

#endif //SAMPLE_VEHICLE_VEHICLE_CONTROLLER_H