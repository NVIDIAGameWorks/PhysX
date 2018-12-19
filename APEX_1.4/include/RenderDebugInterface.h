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



#ifndef RENDER_DEBUG_INTERFACE_H
#define RENDER_DEBUG_INTERFACE_H

/*!
\file
\brief debug rendering classes and structures
*/

#include "ApexDefs.h"
#include "RenderDebugTyped.h" // include the header file containing the base class
#include "ApexInterface.h"
#include "Renderable.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "common/PxRenderBuffer.h"
#endif

/// Macros for getting render debug interface for argument
#define RENDER_DEBUG_IFACE(ptr) ((ptr)->getRenderDebugInterface())

namespace nvidia
{

namespace apex
{
class UserRenderer;
class UserRenderResourceManager;

PX_PUSH_PACK_DEFAULT

#if PX_PHYSICS_VERSION_MAJOR == 3

/**
\brief This is a helper class implementation of PxRenderBuffer that holds the counts
and pointers for renderable data.  Does not own the memory, simply used to transfer
the state.  The append method is not supported.
*/
class PhysXRenderBuffer : public PxRenderBuffer
{
public:
	PhysXRenderBuffer()
	{
		clear();
	}

	/**
	\brief Get number of points in the render buffer
	*/
	virtual uint32_t getNbPoints() const 
	{
		return mNbPoints;
	}

	/**
	\brief Get points data
	*/
	virtual const PxDebugPoint* getPoints() const
	{
		return mPoints;
	}

	/**
	\brief Get number of lines in the render buffer
	*/
	virtual uint32_t getNbLines() const
	{
		return mNbLines;
	}

	/**
	\brief Get lines data
	*/
	virtual const PxDebugLine* getLines() const 
	{
		return mLines;
	}

	/**
	\brief Get number of triangles in the render buffer
	*/
	virtual uint32_t getNbTriangles() const 
	{
		return mNbTriangles;
	}

	/**
	\brief Get triangles data
	*/
	virtual const PxDebugTriangle* getTriangles() const 
	{
		return mTriangles;
	}

	/**
	\brief Get number of texts in the render buffer
	*/
	virtual uint32_t getNbTexts() const 
	{
		return mNbTexts;
	}

	/**
	\brief Get texts data
	*/
	virtual const PxDebugText* getTexts() const 
	{
		return mTexts;
	}

	/**
	\brief Append PhysX render buffer
	*/
	virtual void append(const PxRenderBuffer& other)
	{
		PX_UNUSED(other);
		PX_ALWAYS_ASSERT(); // this method not implemented!
	}

	/**
	\brief Clear this buffer
	*/
	virtual void clear() 
	{
		mNbPoints = 0;
		mPoints = NULL;
		mNbLines = 0;
		mLines = NULL;
		mNbTriangles = 0;
		mTriangles = NULL;
		mNbTexts = 0;
		mTexts = NULL;
	}

	/**
	\brief Number of points
	*/
	uint32_t		mNbPoints;
	/**
	\brief Points data
	*/
	PxDebugPoint	*mPoints;
	/**
	\brief Number of lines
	*/
	uint32_t		mNbLines;
	/**
	\brief Lines data
	*/
	PxDebugLine		*mLines;
	/**
	\brief Number of triangles
	*/
	uint32_t		mNbTriangles;
	/**
	\brief Triangles data
	*/
	PxDebugTriangle	*mTriangles;
	/**
	\brief Number of texts
	*/
	uint32_t		mNbTexts;
	/**
	\brief Text data
	*/
	PxDebugText		*mTexts;
};
#endif

/**
\brief wrapper for DebugRenderable
 */
class RenderDebugInterface : public ApexInterface, public Renderable
{
public:
	/**
	\brief Method to support rendering to a legacy PhysX SDK DebugRenderable object instead
	of to the APEX Render Resources API (i.e.: Renderable).

	This method is used to enable or disable the use of a legacy DebugRenderable.  When enabled,
	use the getDebugRenderable() method to get a legacy DebugRenerable object that will contain
	all the debug output.
	*/
	virtual void	setUseDebugRenderable(bool state) = 0;


#if PX_PHYSICS_VERSION_MAJOR == 3

	/**
	\brief Method to support rendering to a legacy PhysX SDK PxRenderBuffer object instead
	of to the APEX Render Resources API (i.e.: Renderable).

	When enabled with a call to setUseDebugRenderable(true), this method will return a legacy
	PxRenderBuffer object that contains all of the output of the RenderDebug class.
	*/
	virtual void	getRenderBuffer(PhysXRenderBuffer& renderable) = 0;

	/**
	\brief Method to support rendering to a legacy PhysX SDK PxRenderBuffer object instead
	of to the APEX Render Resources API (i.e.: Renderable). Lines and triangle in
	screen space

	When enabled with a call to setUseDebugRenderable(true), this method will return a legacy
	PxRenderBuffer object that contains all of the output of the RenderDebug class.
	*/
	virtual void	getRenderBufferScreenSpace(PhysXRenderBuffer& renderable) = 0;

	/**
	\brief Method to support rendering from an existing PhysX SDK PxRenderBuffer object.

	The contents of the PxRenderBuffer is added to the current contents of the
	RenderDebug object, and is output through the APEX Render Resources API.
	*/
	virtual void	addDebugRenderable(const physx::PxRenderBuffer& renderBuffer) = 0;

#endif //PX_PHYSICS_VERSION_MAJOR == 3

	virtual void release() = 0;

	/**
	\brief Returns render debug interface RENDER_DEBUG::RenderDebugTyped
	*/
	virtual RENDER_DEBUG::RenderDebugTyped* getRenderDebugInterface() = 0;
	
protected:

	virtual ~RenderDebugInterface(void) { };

};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RENDER_DEBUG_INTERFACE_H
