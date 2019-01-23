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

#include "PxTkMatrixUtils.h"

using namespace PxToolkit;

void PxToolkit::setRotX(PxMat33& m, PxReal angle)
{
	m = PxMat33(PxIdentity);

	const PxReal cos = cosf(angle);
	const PxReal sin = sinf(angle);

	m[1][1] = m[2][2] = cos;
	m[1][2] = sin;
	m[2][1] = -sin;
}

void PxToolkit::setRotY(PxMat33& m, PxReal angle)
{
	m = PxMat33(PxIdentity);

	const PxReal cos = cosf(angle);
	const PxReal sin = sinf(angle);

	m[0][0] = m[2][2] = cos;
	m[0][2] = -sin;
	m[2][0] = sin;
}

void PxToolkit::setRotZ(PxMat33& m, PxReal angle)
{
	m = PxMat33(PxIdentity);

	const PxReal cos = cosf(angle);
	const PxReal sin = sinf(angle);

	m[0][0] = m[1][1] = cos;
	m[0][1] = sin;
	m[1][0] = -sin;
}

PxQuat PxToolkit::getRotXQuat(float angle)
{
	PxMat33 m;
	setRotX(m, angle);
	return PxQuat(m);
}

PxQuat PxToolkit::getRotYQuat(float angle)
{
	PxMat33 m;
	setRotY(m, angle);
	return PxQuat(m);
}

PxQuat PxToolkit::getRotZQuat(float angle)
{
	PxMat33 m;
	setRotZ(m, angle);
	return PxQuat(m);
}
