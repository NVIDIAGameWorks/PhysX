#ifndef HACD_H

#define HACD_H

#include "PlatformConfigHACD.h"
#include <stdlib.h>

/*!
**
** Copyright (c) 2015 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**

*/

namespace HACD
{


class HACD_API
{
public:
	
	class Desc
	{
	public:
		Desc(void)
		{
			init();
		}

		bool				mRemoveDuplicateVertices;
		bool				mNormalizeInputMesh;
		bool				mUseFastVersion;
		uint32_t			mTriangleCount;
		uint32_t			mVertexCount;
		const float	*mVertices;
		const uint32_t	*mIndices;
		uint32_t			mMaxHullCount;
		uint32_t			mMaxMergeHullCount;
		uint32_t			mMaxHullVertices;
		float				mConcavity;
		float				mSmallClusterThreshold;
		float				mBackFaceDistanceFactor;
		uint32_t			mDecompositionDepth; // if using legacy ACD algorithm.
		hacd::ICallback*	mCallback;
		void init(void)
		{
			mRemoveDuplicateVertices = true;
			mDecompositionDepth = 0;
			mNormalizeInputMesh = false;
			mTriangleCount = 0;
			mVertexCount = 0;
			mVertices = NULL;
			mIndices = NULL;
			mMaxHullCount = 256;
			mMaxMergeHullCount = 256;
			mMaxHullVertices = 64;
			mConcavity = 0.2f;
			mSmallClusterThreshold = 0.0f;
			mBackFaceDistanceFactor = 0.2f;
			mUseFastVersion = false;
			mCallback = NULL;
		}
	};

	class Hull	
	{
	public:
		uint32_t			mTriangleCount;
		uint32_t			mVertexCount;
		const float			*mVertices;
		const uint32_t		*mIndices;
		float				mVolume;
	};

	virtual uint32_t		performHACD(const Desc &desc) = 0;
	virtual uint32_t		getHullCount(void) = 0;
	virtual const Hull		*getHull(uint32_t index) const = 0;
	virtual void			releaseHACD(void) = 0; // release memory associated with the last HACD request
	

	virtual void			release(void) = 0; // release the HACD_API interface
protected:
	virtual ~HACD_API(void)
	{

	}
};

HACD_API * createHACD_API(void);

};

#endif
