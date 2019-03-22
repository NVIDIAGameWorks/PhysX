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


#ifndef SUBMARINE_CAMERA_CONTROLLER_H
#define SUBMARINE_CAMERA_CONTROLLER_H

#include "SampleCameraController.h"

class SubmarineCameraController: public DefaultCameraController
{
public:
	SubmarineCameraController(): mFollowingMode(true) {}
	~SubmarineCameraController(){}

	virtual void update(Camera& camera, PxReal dtime)
	{
		if(!mFollowingMode)
			DefaultCameraController::update(camera, dtime);
	}

	void updateFollowingMode(Camera& camera, PxReal dtime, const PxVec3& targetPos)
	{
		PX_ASSERT(mFollowingMode);
		// default update (orientation)
		DefaultCameraController::update(camera, dtime);

		// put camera behind submarine/crab
		camera.setPos(targetPos - camera.getViewDir()*15.0f);
	}

	void setFollowingMode(bool follow) { mFollowingMode = follow;}

private:
	bool	mFollowingMode;
};

#endif
