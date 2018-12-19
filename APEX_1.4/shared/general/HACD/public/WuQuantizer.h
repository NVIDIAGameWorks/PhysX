

#ifndef WU_QUANTIZER_H
#define WU_QUANTIZER_H

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

#include "PlatformConfigHACD.h"

namespace HACD
{

class WuQuantizer
{
public:

	// use the Wu quantizer with 10 bits of resolution on each axis.  Precision down to 0.0009765625
	// All input data is normalized to a unit cube.
	virtual const float * wuQuantize3D(uint32_t vcount,
											const float *vertices,
											bool denormalizeResults,
											uint32_t maxVertices,
											uint32_t &outputCount) = 0;

	// Use the kemans quantizer, similar results, but much slower.
	virtual const float * kmeansQuantize3D(uint32_t vcount,
												const float *vertices,
												bool denormalizeResults,
												uint32_t maxVertices,
												uint32_t &outputCount) = 0;

	virtual const float * getDenormalizeScale(void) const = 0;

	virtual const float * getDenormalizeCenter(void) const = 0;

	virtual void release(void) = 0;


protected:
	virtual ~WuQuantizer(void)
	{

	}
};

WuQuantizer * createWuQuantizer(void);

};

#endif
