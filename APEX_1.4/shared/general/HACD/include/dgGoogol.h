/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgGoogol__
#define __dgGoogol__


#include "dgArray.h"
#include "dgVector.h"



//#define DG_GOOGOL_SIZE	16
#define DG_GOOGOL_SIZE		4

class dgGoogol
{
	public:
	dgGoogol(void);
	dgGoogol(double value);
	~dgGoogol(void);

	double GetAproximateValue() const;
	void InitFloatFloat (double value);

	dgGoogol operator+ (const dgGoogol &A) const; 
	dgGoogol operator- (const dgGoogol &A) const; 
	dgGoogol operator* (const dgGoogol &A) const; 
	dgGoogol operator/ (const dgGoogol &A) const; 

	dgGoogol operator+= (const dgGoogol &A); 
	dgGoogol operator-= (const dgGoogol &A); 

	dgGoogol Floor () const;

#ifdef _DEBUG
	void ToString (char* const string) const;
#endif

	private:
	void NegateMantissa (uint64_t* const mantissa) const;
	void CopySignedMantissa (uint64_t* const mantissa) const;
	int32_t NormalizeMantissa (uint64_t* const mantissa) const;
	uint64_t CheckCarrier (uint64_t a, uint64_t b) const;
	void ShiftRightMantissa (uint64_t* const mantissa, int32_t bits) const;

	int32_t LeadinZeros (uint64_t a) const;
	void ExtendeMultiply (uint64_t a, uint64_t b, uint64_t& high, uint64_t& low) const;
	void ScaleMantissa (uint64_t* const out, uint64_t scale) const;

	int8_t m_sign;
	int16_t m_exponent;
	uint64_t m_mantissa[DG_GOOGOL_SIZE];
};


class dgHugeVector: public dgTemplateVector<dgGoogol>
{
	public:
	dgHugeVector ()
		:dgTemplateVector<dgGoogol>()
	{
	}

	dgHugeVector (const dgBigVector& a)
		:dgTemplateVector<dgGoogol>(dgGoogol (a.m_x), dgGoogol (a.m_y), dgGoogol (a.m_z), dgGoogol (a.m_w))
	{
	}

	dgHugeVector (const dgTemplateVector<dgGoogol>& a)
		:dgTemplateVector<dgGoogol>(a)
	{
	}

	dgHugeVector (double x, double y, double z, double w)
		:dgTemplateVector<dgGoogol>(x, y, z, w)
	{
	}

	dgGoogol EvaluePlane (const dgHugeVector& point) const 
	{
		return (point % (*this)) + m_w;
	}




};


#endif
