// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef RENDERER_INTEROPABLE_BUFFER_H
#define RENDERER_INTEROPABLE_BUFFER_H

#include <RendererConfig.h>
#include "cudamanager/PxCudaContextManager.h"

typedef struct CUgraphicsResource_st *CUgraphicsResource;

namespace SampleRenderer
{

	class RendererInteropableBuffer
	{
	public:
		RendererInteropableBuffer(bool mustBeRegistered, physx::PxCudaContextManager *interopContext)
			: m_mustBeRegisteredInCUDA(mustBeRegistered)
			, m_registeredInCUDA(false)
			, m_interopContext(interopContext)			
			, m_InteropHandle(NULL)
			
		{
		}

		bool getInteropResourceHandle(CUgraphicsResource &handle)
		{
			if(m_registeredInCUDA && m_InteropHandle)
			{
				handle = m_InteropHandle;

				return true;
			}

			return false;
		}

	protected:

		bool					m_mustBeRegisteredInCUDA;
		bool					m_registeredInCUDA;
		physx::PxCudaContextManager* m_interopContext;
		CUgraphicsResource		m_InteropHandle;
	};

} // namespace SampleRenderer

#endif
