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



#ifndef CUSTOM_BUFFER_ITERATOR_H
#define CUSTOM_BUFFER_ITERATOR_H

/*!
\file
\brief class CustomBufferIterator
*/

#include "RenderMesh.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief This class is used to access specific elements in an untyped chunk of memory
*/
class CustomBufferIterator
{
public:
	/**
	\brief Returns the memory start of a specific vertex.

	All custom buffers are stored interleaved, so this is also the memory start of the first attribute of this vertex.
	*/
	virtual void*		getVertex(uint32_t triangleIndex, uint32_t vertexIndex) const = 0;

	/**
	\brief Returns the index of a certain custom buffer.

	\note This is constant throughout the existence of this class.
	*/
	virtual int32_t		getAttributeIndex(const char* attributeName) const = 0;

	/**
	\brief Returns a pointer to a certain attribute of the specified vertex/triangle.

	\param [in] triangleIndex Which triangle
	\param [in] vertexIndex Which of the vertices of this triangle (must be either 0, 1 or 2)
	\param [in] attributeName The name of the attribute you wish the data for
	\param [out] outFormat The format of the attribute, reinterpret_cast the void pointer accordingly.
	*/
	virtual void*		getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, const char* attributeName, RenderDataFormat::Enum& outFormat) const = 0;

	/**
	\brief Returns a pointer to a certain attribute of the specified vertex/triangle.

	\note This is the faster method than the one above since it won't do any string comparisons

	\param [in] triangleIndex Which triangle
	\param [in] vertexIndex Which of the vertices of this triangle (must be either 0, 1 or 2)
	\param [in] attributeIndex The indexof the attribute you wish the data for (use CustomBufferIterator::getAttributeIndex to find the index to a certain attribute name
	\param [out] outFormat The format of the attribute, reinterpret_cast the void pointer accordingly.
	\param [out] outName The name associated with the attribute
	*/
	virtual void*		getVertexAttribute(uint32_t triangleIndex, uint32_t vertexIndex, uint32_t attributeIndex, RenderDataFormat::Enum& outFormat, const char*& outName) const = 0;

protected:
	CustomBufferIterator() {}
	virtual				~CustomBufferIterator() {}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // CUSTOM_BUFFER_ITERATOR_H
