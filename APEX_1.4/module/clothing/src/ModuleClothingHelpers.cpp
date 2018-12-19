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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#include "ApexDefs.h"

#include "ModuleClothingHelpers.h"
#include "AbstractMeshDescription.h"

namespace nvidia
{
namespace apex
{

void AbstractMeshDescription::UpdateDerivedInformation(RenderDebugInterface* renderDebug)
{
	if (numIndices > 0)
	{
		pMin = pPosition[pIndices[0]];
		pMax = pMin;
	}
	avgEdgeLength = 0;
	avgTriangleArea = 0;

	uint32_t triCount(numIndices / 3);
	uint32_t edgeCount(numIndices);
	for (uint32_t j = 0; j < numIndices; j += 3)
	{
		uint32_t i0 = pIndices[j + 0];
		uint32_t i1 = pIndices[j + 1];
		uint32_t i2 = pIndices[j + 2];

		const PxVec3& v0 = pPosition[i0];
		const PxVec3& v1 = pPosition[i1];
		const PxVec3& v2 = pPosition[i2];

		pMin.minimum(v0);
		pMin.minimum(v1);
		pMin.minimum(v2);

		pMax.maximum(v0);
		pMax.maximum(v1);
		pMax.maximum(v2);

		PxVec3 e0 = v1 - v0;
		PxVec3 e1 = v2 - v1;
		PxVec3 e2 = v0 - v2;

		avgEdgeLength += e0.magnitude();
		avgEdgeLength += e1.magnitude();
		avgEdgeLength += e2.magnitude();


		if (renderDebug)
		{
			using RENDER_DEBUG::DebugColors;
			RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkBlue));
			RENDER_DEBUG_IFACE(renderDebug)->debugLine(v0, v1);
			RENDER_DEBUG_IFACE(renderDebug)->debugLine(v1, v2);
			RENDER_DEBUG_IFACE(renderDebug)->debugLine(v2, v0);
			RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::Green));
			RENDER_DEBUG_IFACE(renderDebug)->debugPoint(v0, 0.1f);
			RENDER_DEBUG_IFACE(renderDebug)->debugPoint(v1, 0.1f);
			RENDER_DEBUG_IFACE(renderDebug)->debugPoint(v2, 0.1f);
		}

		float triangleArea = e0.cross(e2).magnitude() * 0.5f;
		avgTriangleArea += triangleArea;
		triCount++;
	}

	avgEdgeLength /= edgeCount;
	avgTriangleArea /= triCount;
	centroid = 0.5f * (pMin + pMax);
	radius = 0.5f * (pMax - pMin).magnitude();

	//printf("Min = <%f, %f, %f>; Max = <%f, %f, %f>; centroid = <%f, %f, %f>; radius = %f; avgEdgeLength = %f; avgTriangleArea = %f;\n",
	//	pMin.x, pMin.y, pMin.z, pMax.x, pMax.y, pMax.z, centroid.x, centroid.y, centroid.z, radius, avgEdgeLength, avgTriangleArea);
}

}
} // namespace nvidia

