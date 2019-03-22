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

#ifndef SAMPLE_TREE_H
#define	SAMPLE_TREE_H

#include "foundation/Px.h"

namespace SampleFramework
{

class Tree
{
public:
	class Node
	{
	public:
		Node() : mParent(NULL), mHead(NULL), mTail(NULL), mPrev(NULL), mNext(NULL)
		{
		}

	private:
		Node(const Node& node);
		Node& operator=(const Node& node);

	public:
		PX_FORCE_INLINE	bool	isRoot()			const	{ return NULL == mParent; }
		PX_FORCE_INLINE	bool	isLeaf()			const	{ return NULL == mHead; }

		PX_FORCE_INLINE	Node*	getParent()			const	{ return mParent; }
		PX_FORCE_INLINE	Node*	getFirstChild()		const	{ return mHead; }
		PX_FORCE_INLINE	Node*	getLastChild()		const	{ return mTail; }
		PX_FORCE_INLINE	Node*	getPrevSibling()	const	{ return mPrev; }
		PX_FORCE_INLINE	Node*	getNextSibling()	const	{ return mNext; }

		PX_FORCE_INLINE	Node*	getFirstLeaf()		const	{ return NULL == mHead ? const_cast<Node*>(this) : mHead->getFirstLeaf(); }
		PX_FORCE_INLINE	Node*	getLastLeaf()		const	{ return NULL == mTail ? const_cast<Node*>(this) : mTail->getLastLeaf(); }

		bool	isOffspringOf(const Node& node)		const
		{
			return (this == &node) || (NULL != mParent && mParent->isOffspringOf(node));
		}

	public:
		bool appendChild(Node& node);
		bool removeChild(Node& node);

	private:
		Node*	mParent;
		Node*	mHead;
		Node*	mTail;
		Node*	mPrev;
		Node*	mNext;
	};
};

} // namespace SampleFramework

#endif
