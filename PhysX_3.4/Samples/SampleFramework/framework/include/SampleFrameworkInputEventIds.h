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
#ifndef SAMPLE_FRAMEWORK_INPUT_EVENT_IDS_H
#define SAMPLE_FRAMEWORK_INPUT_EVENT_IDS_H

static const float GAMEPAD_ROTATE_SENSITIVITY = 0.7f;
static const float GAMEPAD_DEFAULT_SENSITIVITY = 1.0f;

// InputEvents used by SampleApplication
enum SampleFrameworkInputEventIds
{
	CAMERA_SHIFT_SPEED = 0,
	CAMERA_MOVE_LEFT,
	CAMERA_MOVE_RIGHT,
	CAMERA_MOVE_UP,
	CAMERA_MOVE_DOWN,
	CAMERA_MOVE_FORWARD,
	CAMERA_MOVE_BACKWARD,
	CAMERA_SPEED_INCREASE,
	CAMERA_SPEED_DECREASE,

	CAMERA_MOUSE_LOOK,
	CAMERA_MOVE_BUTTON,

	CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT,
	CAMERA_GAMEPAD_ROTATE_UP_DOWN,
	CAMERA_GAMEPAD_MOVE_LEFT_RIGHT,
	CAMERA_GAMEPAD_MOVE_FORWARD_BACK,

	CAMERA_JUMP,
	CAMERA_CROUCH,
	CAMERA_CONTROLLER_INCREASE,
	CAMERA_CONTROLLER_DECREASE,

	NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS, 
};

// InputEvent descriptions used by SampleApplication
const char* const SampleFrameworkInputEventDescriptions[] =
{
	"change the camera speed",
	"move the camera left", 
	"move the camera right",
	"move the camera up",
	"move the camera down",
	"move the camera forward",
	"move the camera backward",
	"increase the camera move speed",
	"decrease the camera move speed",

	"look around with the camera",
	"enable looking around with the camera",

	"look left and right with the camera",
	"look up and down look with the camera",
	"move the camera left and right",
	"move the camera forward and backward",

	"jump",
	"crouch",
	"next controller",
	"previous controller",
};

PX_COMPILE_TIME_ASSERT(PX_ARRAY_SIZE(SampleFrameworkInputEventDescriptions) == NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS);

#endif

