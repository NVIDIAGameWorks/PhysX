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

#ifndef QUAT_H
#define QUAT_H

//	Singe / VecReal Precision Vec 3
//  Matthias Mueller
//  derived from Quat.h

#include "Vec3.h"

namespace M
{

class Quat
{
public:
	Quat()	{	}

	VecReal x,y,z,w;

	Quat(VecReal nx, VecReal ny, VecReal nz, VecReal nw) : x(nx),y(ny),z(nz),w(nw) {}

	Quat(VecReal angleRadians, const Vec3& unitAxis)
	{
		const VecReal a = angleRadians * 0.5f;
		const VecReal s = vecSin(a);
		w = cos(a);
		x = unitAxis.x * s;
		y = unitAxis.y * s;
		z = unitAxis.z * s;
	}

	Quat(const Quat& v): x(v.x), y(v.y), z(v.z), w(v.w) {}

	void toRadiansAndUnitAxis(VecReal& angle, Vec3& axis) const
	{
		const VecReal quatEpsilon = VecReal(1.0e-8f);
		const VecReal s2 = x*x+y*y+z*z;
		if(s2<quatEpsilon*quatEpsilon)  // can't extract a sensible axis
		{
			angle = 0;
			axis = Vec3(1,0,0);
		}
		else
		{
			const VecReal s = 1.0f / vecSqrt(s2);
			axis = Vec3(x,y,z) * s; 
			angle = vecAbs(w)<quatEpsilon ? VEC_PI : vecAtan2(s2*s, w) * 2;
		}

	}

	/**
	\brief Gets the angle between this quat and the identity quaternion.

	<b>Unit:</b> Radians
	*/
	VecReal getAngle() const
	{
		return vecACos(w) * VecReal(2);
	}


	/**
	\brief Gets the angle between this quat and the argument

	<b>Unit:</b> Radians
	*/
	VecReal getAngle(const Quat& q) const
	{
		return vecACos(dot(q)) * VecReal(2);
	}


	/**
	\brief This is the squared 4D vector length, should be 1 for unit quaternions.
	*/
	VecReal magnitudeSquared() const
	{
		return x*x + y*y + z*z + w*w;
	}

	/**
	\brief returns the scalar product of this and other.
	*/
	VecReal dot(const Quat& v) const
	{
		return x * v.x + y * v.y + z * v.z  + w * v.w;
	}

	Quat getNormalized() const
	{
		const VecReal s = (VecReal)1.0/magnitude();
		return Quat(x*s, y*s, z*s, w*s);
	}


	VecReal magnitude() const
	{
		return vecSqrt(magnitudeSquared());
	}

	//modifiers:
	/**
	\brief maps to the closest unit quaternion.
	*/
	VecReal normalize()											// convert this Quat to a unit quaternion
	{
		const VecReal mag = magnitude();
		if (mag)
		{
			const VecReal imag = VecReal(1) / mag;

			x *= imag;
			y *= imag;
			z *= imag;
			w *= imag;
		}
		return mag;
	}

	/*
	\brief returns the conjugate.

	\note for unit quaternions, this is the inverse.
	*/
	Quat getConjugate() const
	{
		return Quat(-x,-y,-z,w);
	}

	/*
	\brief returns imaginary part.
	*/
	Vec3 getImaginaryPart() const
	{
		return Vec3(x,y,z);
	}

	/** brief computes rotation of x-axis */
	Vec3 getBasisVector0()	const
	{	
		//		return rotate(Vec3(1,0,0));
		const VecReal x2 = x*(VecReal)2.0;
		const VecReal w2 = w*(VecReal)2.0;
		return Vec3(	(w * w2) - 1.0f + x*x2,
			(z * w2)        + y*x2,
			(-y * w2)       + z*x2);
	}

	/** brief computes rotation of y-axis */
	Vec3 getBasisVector1()	const 
	{	
		//		return rotate(Vec3(0,1,0));
		const VecReal y2 = y*(VecReal)2.0;
		const VecReal w2 = w*(VecReal)2.0;
		return Vec3(	(-z * w2)       + x*y2,
			(w * w2) - 1.0f + y*y2,
			(x * w2)        + z*y2);
	}


	/** brief computes rotation of z-axis */
	Vec3 getBasisVector2() const	
	{	
		//		return rotate(Vec3(0,0,1));
		const VecReal z2 = z*(VecReal)2.0;
		const VecReal w2 = w*(VecReal)2.0;
		return Vec3(	(y * w2)        + x*z2,
			(-x * w2)       + y*z2,
			(w * w2) - 1.0f + z*z2);
	}

	/**
	rotates passed vec by this (assumed unitary)
	*/
	const Vec3 rotate(const Vec3& v) const
		//	  const Vec3 rotate(const Vec3& v) const
	{
		const VecReal vx = (VecReal)2.0*v.x;
		const VecReal vy = (VecReal)2.0*v.y;
		const VecReal vz = (VecReal)2.0*v.z;
		const VecReal w2 = w*w-(VecReal)0.5;
		const VecReal dot2 = (x*vx + y*vy +z*vz);
		return Vec3
			(
			(vx*w2 + (y * vz - z * vy)*w + x*dot2), 
			(vy*w2 + (z * vx - x * vz)*w + y*dot2), 
			(vz*w2 + (x * vy - y * vx)*w + z*dot2)
			);
		/*
		const Vec3 qv(x,y,z);
		return (v*(w*w-0.5f) + (qv.cross(v))*w + qv*(qv.dot(v)))*2;
		*/
	}

	/**
	inverse rotates passed vec by this (assumed unitary)
	*/
	const Vec3 rotateInv(const Vec3& v) const
		//	  const Vec3 rotateInv(const Vec3& v) const
	{
		const VecReal vx = (VecReal)2.0*v.x;
		const VecReal vy = (VecReal)2.0*v.y;
		const VecReal vz = (VecReal)2.0*v.z;
		const VecReal w2 = w*w-(VecReal)0.5;
		const VecReal dot2 = (x*vx + y*vy +z*vz);
		return Vec3
			(
			(vx*w2 - (y * vz - z * vy)*w + x*dot2), 
			(vy*w2 - (z * vx - x * vz)*w + y*dot2), 
			(vz*w2 - (x * vy - y * vx)*w + z*dot2)
			);
		//		const Vec3 qv(x,y,z);
		//		return (v*(w*w-0.5f) - (qv.cross(v))*w + qv*(qv.dot(v)))*2;
	}

	/**
	\brief Assignment operator
	*/
	Quat&	operator=(const Quat& p)			{ x = p.x; y = p.y; z = p.z; w = p.w;	return *this;		}

	Quat& operator*= (const Quat& q)
	{
		const VecReal tx = w*q.x + q.w*x + y*q.z - q.y*z;
		const VecReal ty = w*q.y + q.w*y + z*q.x - q.z*x;
		const VecReal tz = w*q.z + q.w*z + x*q.y - q.x*y;

		w = w*q.w - q.x*x - y*q.y - q.z*z;
		x = tx;
		y = ty;
		z = tz;

		return *this;
	}

	Quat& operator+= (const Quat& q)
	{
		x+=q.x;
		y+=q.y;
		z+=q.z;
		w+=q.w;
		return *this;
	}

	Quat& operator-= (const Quat& q)
	{
		x-=q.x;
		y-=q.y;
		z-=q.z;
		w-=q.w;
		return *this;
	}

	Quat& operator*= (const VecReal s)
	{
		x*=s;
		y*=s;
		z*=s;
		w*=s;
		return *this;
	}

	/** quaternion multiplication */
	Quat operator*(const Quat& q) const
	{
		return Quat(w*q.x + q.w*x + y*q.z - q.y*z,
			w*q.y + q.w*y + z*q.x - q.z*x,
			w*q.z + q.w*z + x*q.y - q.x*y,
			w*q.w - x*q.x - y*q.y - z*q.z);
	}

	/** quaternion addition */
	Quat operator+(const Quat& q) const
	{
		return Quat(x+q.x,y+q.y,z+q.z,w+q.w);
	}

	/** quaternion subtraction */
	Quat operator-() const
	{
		return Quat(-x,-y,-z,-w);
	}


	Quat operator-(const Quat& q) const
	{
		return Quat(x-q.x,y-q.y,z-q.z,w-q.w);
	}


	Quat operator*(VecReal r) const
	{
		return Quat(x*r,y*r,z*r,w*r);
	}

	static   Quat createIdentity() { return Quat(0,0,0,1); }
};

}

#endif 
