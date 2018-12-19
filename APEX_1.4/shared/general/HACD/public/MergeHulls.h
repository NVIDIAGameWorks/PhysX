

#ifndef MERGE_HULLS_H

#define MERGE_HULLS_H

#include "PlatformConfigHACD.h"

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

class MergeHull
{
public:
	uint32_t			mTriangleCount;
	uint32_t			mVertexCount;
	const float	*mVertices;
	const uint32_t	*mIndices;
};

typedef hacd::vector< MergeHull > MergeHullVector;

class MergeHullsInterface
{
public:
	// Merge these input hulls.
	virtual uint32_t mergeHulls(const MergeHullVector &inputHulls,
									MergeHullVector &outputHulls,
									uint32_t	mergeHullCount,
									float smallClusterThreshold,
									uint32_t maxHullVertices,
									hacd::ICallback *callback) = 0;


	virtual void release(void) = 0;

protected:
	virtual ~MergeHullsInterface(void)
	{

	}

};

MergeHullsInterface * createMergeHullsInterface(void);

}; // end of HACD namespace

#endif
