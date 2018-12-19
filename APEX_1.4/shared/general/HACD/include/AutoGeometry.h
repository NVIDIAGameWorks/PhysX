
#ifndef AUTO_GEOMETRY_H

#define AUTO_GEOMETRY_H

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
	class SimpleHull
	{
	public:
		SimpleHull(void)
		{
			mBoneIndex = 0;
			mVertexCount = 0;
			mVertices  = 0;
			mTriCount = 0;
			mIndices = 0;
			mMeshVolume = 0;
			mParentIndex = -1;
			mBoneName = 0;
		}
		int32_t           mBoneIndex;
		int32_t           mParentIndex;
		const char   *mBoneName;
		float         mTransform[16];
		uint32_t  mVertexCount;
		float        *mVertices;
		uint32_t  mTriCount;
		uint32_t *mIndices;
		float         mMeshVolume;
	};

	enum BoneOption
	{
		BO_INCLUDE,
		BO_EXCLUDE,
		BO_COLLAPSE
	};

	class SimpleBone
	{
	public:
		SimpleBone(void)
		{
			mOption = BO_INCLUDE;
			mParentIndex = -1;
			mBoneName = 0;
			mBoneMinimalWeight = 0.4f;
		}
		BoneOption   mOption;
		const char  *mBoneName;
		int32_t          mParentIndex;
		float        mBoneMinimalWeight;
		float        mTransform[16];
		float        mInverseTransform[16];
	};

	class SimpleSkinnedVertex
	{
	public:
		float          mPos[3];
		unsigned short mBone[4];
		float          mWeight[4];
	};

	class SimpleSkinnedMesh
	{
	public:
		uint32_t         mVertexCount;
		SimpleSkinnedVertex *mVertices;

	};

	class AutoGeometry
	{
	public:

		virtual SimpleHull ** createCollisionVolumes(float collapse_percentage,          // percentage volume to collapse a child into a parent
			uint32_t bone_count,
			const SimpleBone *bones,
			const SimpleSkinnedMesh *mesh,
			uint32_t &geom_count) = 0;


		virtual SimpleHull ** createCollisionVolumes(float collapse_percentage,uint32_t &geom_count) = 0;

		virtual void addSimpleSkinnedTriangle(const SimpleSkinnedVertex &v1,
			const SimpleSkinnedVertex &v2,
			const SimpleSkinnedVertex &v3) = 0;

		virtual void addSimpleBone(const SimpleBone &b) = 0;

		virtual const char * stristr(const char *str,const char *match) = 0; // case insensitive ststr

	};

	AutoGeometry * createAutoGeometry();
	void           releaseAutoGeometry(AutoGeometry *g);

}; // end of namespace


#endif
