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


#ifndef LINK_H
#define LINK_H

#include "PxSimpleTypes.h"
#include "PxAssert.h"

namespace nvidia
{
namespace apex
{

class Link
{
public:
	Link()
	{
		adj[1] = adj[0] = this;
	}

	virtual	~Link()
	{
		remove();
	}

	/*
		which = 0:	(-A-...-B-link-)  +  (-this-X-...-Y-)  =  (-A-...-B-link-this-X-...-Y-)
		which = 1:	(-X-...-Y-this-)  +  (-link-A-...-B-)  =  (-X-...-Y-this-link-A-...-B-)
	 */
	void	setAdj(uint32_t which, Link* link)
	{
		uint32_t other = (which &= 1) ^ 1;
		Link* linkAdjOther = link->adj[other];
		adj[which]->adj[other] = linkAdjOther;
		linkAdjOther->adj[which] = adj[which];
		adj[which] = link;
		link->adj[other] = this;
	}

	Link*	getAdj(uint32_t which) const
	{
		return adj[which & 1];
	}

	void	remove()
	{
		adj[1]->adj[0] = adj[0];
		adj[0]->adj[1] = adj[1];
		adj[1] = adj[0] = this;
	}

	bool	isSolitary() const
	{
		PX_ASSERT((adj[0] == this) == (adj[1] == this));
		return adj[0] == this;
	}

protected:
	Link*	adj[2];
};

}
} // end namespace nvidia::apex


#endif // #ifndef LINK_H
