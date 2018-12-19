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


#include "dgGoogol.h"
#include <string.h>


dgGoogol::dgGoogol(void)
	:m_sign(0)
	,m_exponent(0)
{
	memset (m_mantissa, 0, sizeof (m_mantissa));
}

dgGoogol::dgGoogol(double value)
	:m_sign(0)
	,m_exponent(0)
{
	int32_t exp;
	double mantissa = fabs (frexp(value, &exp));

	m_exponent = int16_t (exp);
	m_sign = (value >= 0) ? 0 : 1;

	memset (m_mantissa, 0, sizeof (m_mantissa));
	m_mantissa[0] = (uint64_t (double (uint64_t(1)<<62) * mantissa));

	// it looks like GCC have problems with this
//	HACD_ASSERT (m_mantissa[0] >= 0);
}


dgGoogol::~dgGoogol(void)
{
}

double dgGoogol::GetAproximateValue() const
{
	double mantissa = (double(1.0f) / double (uint64_t(1)<<62)) * double (m_mantissa[0]);
	mantissa = ldexp(mantissa, m_exponent) * (m_sign ?  double (-1.0f) : double (1.0f));
	return mantissa;
}

void dgGoogol::NegateMantissa (uint64_t* const mantissa) const
{
	uint64_t carrier = 1;
	for (int32_t i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
		uint64_t a = ~mantissa[i] + carrier;
		if (a) {
			carrier = 0;
		}
		mantissa[i] = a;
	}
}

void dgGoogol::CopySignedMantissa (uint64_t* const mantissa) const
{
	memcpy (mantissa, m_mantissa, sizeof (m_mantissa));
	if (m_sign) {
		NegateMantissa (mantissa);
	}
}

void dgGoogol::ShiftRightMantissa (uint64_t* const mantissa, int32_t bits) const
{
	uint64_t carrier = 0;
	if (int64_t (mantissa[0]) < int64_t (0)) {
		carrier = uint64_t (-1);
	}
	
	while (bits >= 64) {
		for (int32_t i = DG_GOOGOL_SIZE - 2; i >= 0; i --) {
			mantissa[i + 1] = mantissa[i];
		}
		mantissa[0] = carrier;
		bits -= 64;
	}

	if (bits > 0) {
		carrier <<= (64 - bits);
		for (int32_t i = 0; i < DG_GOOGOL_SIZE; i ++) {
			uint64_t a = mantissa[i];
			mantissa[i] = (a >> bits) | carrier;
			carrier = a << (64 - bits);
		}
	}
}

int32_t dgGoogol::LeadinZeros (uint64_t a) const
{
	#define dgCOUNTBIT(mask,add)		\
	{									\
		uint64_t test = a & mask;	\
		n += test ? 0 : add;			\
		a = test ? test : (a & ~mask);	\
	}

	int32_t n = 0;
	dgCOUNTBIT (0xffffffff00000000LL, 32);
	dgCOUNTBIT (0xffff0000ffff0000LL, 16);
	dgCOUNTBIT (0xff00ff00ff00ff00LL,  8);
	dgCOUNTBIT (0xf0f0f0f0f0f0f0f0LL,  4);
	dgCOUNTBIT (0xccccccccccccccccLL,  2);
	dgCOUNTBIT (0xaaaaaaaaaaaaaaaaLL,  1);

	return n;
}

int32_t dgGoogol::NormalizeMantissa (uint64_t* const mantissa) const
{
//	HACD_ASSERT((mantissa[0] & 0x8000000000000000ULL) == 0);

	int32_t bits = 0;
	if(int64_t (mantissa[0] * 2) < 0) {
		bits = 1;
		ShiftRightMantissa (mantissa, 1);
	} else {
		while (!mantissa[0] && bits > (-64 * DG_GOOGOL_SIZE)) {
			bits -= 64;
			for (int32_t i = 1; i < DG_GOOGOL_SIZE; i ++) {
				mantissa[i - 1] = mantissa[i];
			}
			mantissa[DG_GOOGOL_SIZE - 1] = 0;
		}

		if (bits > (-64 * DG_GOOGOL_SIZE)) {
			int32_t n = LeadinZeros (mantissa[0]) - 2;
			uint64_t carrier = 0;
			for (int32_t i = DG_GOOGOL_SIZE-1; i >= 0; i --) {
				uint64_t a = mantissa[i];
				mantissa[i] = (a << n) | carrier;
				carrier = a >> (64 - n);
			}
			bits -= n;
		}
	}
	return bits;
}

uint64_t dgGoogol::CheckCarrier (uint64_t a, uint64_t b) const
{
	return (uint64_t)(((uint64_t (-1) - b) < a) ? 1 : 0);
}

dgGoogol dgGoogol::operator+ (const dgGoogol &A) const
{
	dgGoogol tmp;
	if (m_mantissa[0] && A.m_mantissa[0]) {
		uint64_t mantissa0[DG_GOOGOL_SIZE];
		uint64_t mantissa1[DG_GOOGOL_SIZE];
		uint64_t mantissa[DG_GOOGOL_SIZE];

		CopySignedMantissa (mantissa0);
		A.CopySignedMantissa (mantissa1);

		int32_t exponetDiff = m_exponent - A.m_exponent;
		int32_t exponent = m_exponent;
		if (exponetDiff > 0) {
			ShiftRightMantissa (mantissa1, exponetDiff);
		} else if (exponetDiff < 0) {
			exponent = A.m_exponent;
			ShiftRightMantissa (mantissa0, -exponetDiff);
		} 

		uint64_t carrier = 0;
		for (int32_t i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
			uint64_t m0 = mantissa0[i];
			uint64_t m1 = mantissa1[i];
			mantissa[i] = m0 + m1 + carrier;
			carrier = CheckCarrier (m0, m1) | CheckCarrier (m0 + m1, carrier);
		}

		int8_t sign = 0;
		if (int64_t (mantissa[0]) < 0) {
			sign = 1;
			NegateMantissa (mantissa);
		}

		int32_t bits = NormalizeMantissa (mantissa);
		if (bits <= (-64 * DG_GOOGOL_SIZE)) {
			tmp.m_sign = 0;
			tmp.m_exponent = 0;
		} else {
			tmp.m_sign = sign;
			tmp.m_exponent =  int16_t (exponent + bits);
		}

		memcpy (tmp.m_mantissa, mantissa, sizeof (m_mantissa));
		

	} else if (A.m_mantissa[0]) {
		tmp = A;
	} else {
		tmp = *this;
	}

	return tmp;
}


dgGoogol dgGoogol::operator- (const dgGoogol &A) const
{
	dgGoogol tmp (A);
	tmp.m_sign = !tmp.m_sign;
	return *this + tmp;
}


void dgGoogol::ExtendeMultiply (uint64_t a, uint64_t b, uint64_t& high, uint64_t& low) const
{
	uint64_t bLow = b & 0xffffffff; 
	uint64_t bHigh = b >> 32; 
	uint64_t aLow = a & 0xffffffff; 
	uint64_t aHigh = a >> 32; 

	uint64_t l = bLow * aLow;

	uint64_t c1 = bHigh * aLow;
	uint64_t c2 = bLow * aHigh;
	uint64_t m = c1 + c2;
	uint64_t carrier = CheckCarrier (c1, c2) << 32;

	uint64_t h = bHigh * aHigh + carrier;

	uint64_t ml = m << 32;	
	uint64_t ll = l + ml;
	uint64_t mh = (m >> 32) + CheckCarrier (l, ml);	
	HACD_ASSERT ((mh & ~0xffffffff) == 0);

	uint64_t hh = h + mh;

	low = ll;
	high = hh;
}

void dgGoogol::ScaleMantissa (uint64_t* const dst, uint64_t scale) const
{
	uint64_t carrier = 0;
	for (int32_t i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
		if (m_mantissa[i]) {
			uint64_t low;
			uint64_t high;
			ExtendeMultiply (scale, m_mantissa[i], high, low);
			uint64_t acc = low + carrier;
			carrier = CheckCarrier (low, carrier);	
			HACD_ASSERT (CheckCarrier (carrier, high) == 0);
			carrier += high;
			dst[i + 1] = acc;
		} else {
			dst[i + 1] = carrier;
			carrier = 0;
		}

	}
	dst[0] = carrier;
}

dgGoogol dgGoogol::operator* (const dgGoogol &A) const
{
	HACD_ASSERT((m_mantissa[0] & 0x8000000000000000ULL) == 0);
	HACD_ASSERT((A.m_mantissa[0] & 0x8000000000000000ULL) == 0);

	if (m_mantissa[0] && A.m_mantissa[0]) {
		uint64_t mantissaAcc[DG_GOOGOL_SIZE * 2];
		memset (mantissaAcc, 0, sizeof (mantissaAcc));
		for (int32_t i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
			uint64_t a = m_mantissa[i];
			if (a) {
				uint64_t mantissaScale[2 * DG_GOOGOL_SIZE];
				memset (mantissaScale, 0, sizeof (mantissaScale));
				A.ScaleMantissa (&mantissaScale[i], a);

				uint64_t carrier = 0;
				for (int32_t j = 2 * DG_GOOGOL_SIZE - 1; j >= 0; j --) {
					uint64_t m0 = mantissaAcc[j];
					uint64_t m1 = mantissaScale[j];
					mantissaAcc[j] = m0 + m1 + carrier;
					carrier = CheckCarrier (m0, m1) | CheckCarrier (m0 + m1, carrier);
				}
			}
		}
		
		uint64_t carrier = 0;
		int32_t bits = LeadinZeros (mantissaAcc[0]) - 2;
		for (int32_t i = 2 * DG_GOOGOL_SIZE - 1; i >= 0; i --) {
			uint64_t a = mantissaAcc[i];
			mantissaAcc[i] = (a << bits) | carrier;
			carrier = a >> (64 - bits);
		}

		int32_t exp = m_exponent + A.m_exponent - (bits - 2);

		dgGoogol tmp;
		tmp.m_sign = m_sign ^ A.m_sign;
		tmp.m_exponent = int16_t (exp);
		memcpy (tmp.m_mantissa, mantissaAcc, sizeof (m_mantissa));

		return tmp;
	} 
	return dgGoogol(0.0);
}

dgGoogol dgGoogol::operator/ (const dgGoogol &A) const
{
	dgGoogol tmp (1.0 / A.GetAproximateValue());
	dgGoogol two (2.0);
	
	tmp = tmp * (two - A * tmp);
	tmp = tmp * (two - A * tmp);
	
	uint64_t copy[DG_GOOGOL_SIZE];

	bool test = true;
	int passes = 0;
	do {
		passes ++;
		memcpy (copy, tmp.m_mantissa, sizeof (tmp.m_mantissa));
		tmp = tmp * (two - A * tmp);
		test = true;
		for (int32_t i = 0; test && (i < DG_GOOGOL_SIZE); i ++) {
			test = (copy[i] == tmp.m_mantissa[i]);
		}
	} while (!test || (passes > (2 * DG_GOOGOL_SIZE)));	
	HACD_ASSERT (passes <= (2 * DG_GOOGOL_SIZE));
	return (*this) * tmp;
}


dgGoogol dgGoogol::operator+= (const dgGoogol &A)
{
	*this = *this + A;
	return *this;
}

dgGoogol dgGoogol::operator-= (const dgGoogol &A)
{
	*this = *this - A;
	return *this;
}

dgGoogol dgGoogol::Floor () const
{
	if (m_exponent < 1) {
		return dgGoogol (0.0);
	} 
	int32_t bits = m_exponent + 2;
	int32_t start = 0;
	while (bits >= 64) {
		bits -= 64;
		start ++;
	}
	
	dgGoogol tmp (*this);
	for (int32_t i = DG_GOOGOL_SIZE - 1; i > start; i --) {
		tmp.m_mantissa[i] = 0;
	}
	uint64_t mask = uint64_t(-1) << (64 - bits);
	tmp.m_mantissa[start] &= mask;
	if (m_sign) {
		HACD_ASSERT (0);
	}

	return tmp;
}

#ifdef _DEBUG
void dgGoogol::ToString (char* const string) const
{
	dgGoogol tmp (*this);
	dgGoogol base (10.0);

//char aaaa[256];
double a = tmp.GetAproximateValue(); 

	while (tmp.GetAproximateValue() > 1.0) {
		tmp = tmp/base;
	}
a = tmp.GetAproximateValue(); 

	int32_t index = 0;
//	tmp.m_exponent = 1;
	while (tmp.m_mantissa[0]) {
		
//double xxx = tmp.GetAproximateValue();
//double xxx1 = digit.GetAproximateValue();
//double m = floor (a);
//a = a - m;
//a = a * 10;
//aaaa[index] = char (m) + '0';

		tmp = tmp * base;
		dgGoogol digit (tmp.Floor());
		tmp -= digit;
		double val = digit.GetAproximateValue();
		string[index] = char (val) + '0';
		index ++;
	}
	string[index] = 0;
}


#endif