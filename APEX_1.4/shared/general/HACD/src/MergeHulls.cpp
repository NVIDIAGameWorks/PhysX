#include "MergeHulls.h"
#include "ConvexHull.h"
#include "SparseArray.h"
#include <string.h>
#include <math.h>

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

using namespace hacd;

namespace HACD
{

typedef SparseArray< float > TestedMap;

static int gCombineCount=0;

static float fm_computeBestFitAABB(uint32_t vcount,const float *points,uint32_t pstride,float *bmin,float *bmax) // returns the diagonal distance
{

	const uint8_t *source = (const uint8_t *) points;

	bmin[0] = points[0];
	bmin[1] = points[1];
	bmin[2] = points[2];

	bmax[0] = points[0];
	bmax[1] = points[1];
	bmax[2] = points[2];


	for (uint32_t i=1; i<vcount; i++)
	{
		source+=pstride;
		const float *p = (const float *) source;

		if ( p[0] < bmin[0] ) bmin[0] = p[0];
		if ( p[1] < bmin[1] ) bmin[1] = p[1];
		if ( p[2] < bmin[2] ) bmin[2] = p[2];

		if ( p[0] > bmax[0] ) bmax[0] = p[0];
		if ( p[1] > bmax[1] ) bmax[1] = p[1];
		if ( p[2] > bmax[2] ) bmax[2] = p[2];

	}

	float dx = bmax[0] - bmin[0];
	float dy = bmax[1] - bmin[1];
	float dz = bmax[2] - bmin[2];

	return (float) ::sqrtf( dx*dx + dy*dy + dz*dz );

}




static bool fm_intersectAABB(const float *bmin1,const float *bmax1,const float *bmin2,const float *bmax2)
{
	if ((bmin1[0] > bmax2[0]) || (bmin2[0] > bmax1[0])) return false;
	if ((bmin1[1] > bmax2[1]) || (bmin2[1] > bmax1[1])) return false;
	if ((bmin1[2] > bmax2[2]) || (bmin2[2] > bmax1[2])) return false;
	return true;
}


static HACD_INLINE float det(const float *p1,const float *p2,const float *p3)
{
	return  p1[0]*p2[1]*p3[2] + p2[0]*p3[1]*p1[2] + p3[0]*p1[1]*p2[2] -p1[0]*p3[1]*p2[2] - p2[0]*p1[1]*p3[2] - p3[0]*p2[1]*p1[2];
}


static float  fm_computeMeshVolume(const float *vertices,uint32_t tcount,const uint32_t *indices)
{
	float volume = 0;
	for (uint32_t i=0; i<tcount; i++,indices+=3)
	{
		const float *p1 = &vertices[ indices[0]*3 ];
		const float *p2 = &vertices[ indices[1]*3 ];
		const float *p3 = &vertices[ indices[2]*3 ];
		volume+=det(p1,p2,p3); // compute the volume of the tetrahedran relative to the origin.
	}

	volume*=(1.0f/6.0f);
	if ( volume < 0 )
		volume*=-1;
	return volume;
}



class CHull : public UANS::UserAllocated
	{
	public:
		CHull(uint32_t vcount,const float *vertices,uint32_t tcount,const uint32_t *indices,uint32_t guid)
		{
			mGuid = guid;
			mVertexCount = vcount;
			mTriangleCount = tcount;
			mVertices = (float *)HACD_ALLOC(sizeof(float)*3*vcount);
			memcpy(mVertices,vertices,sizeof(float)*3*vcount);
			mIndices = (uint32_t *)HACD_ALLOC(sizeof(uint32_t)*3*tcount);
			memcpy(mIndices,indices,sizeof(uint32_t)*3*tcount);
			mVolume = fm_computeMeshVolume( mVertices, mTriangleCount, mIndices);
			mDiagonal = fm_computeBestFitAABB( mVertexCount, mVertices, sizeof(float)*3, mMin, mMax );
			float dx = mMax[0] - mMin[0];
			float dy = mMax[1] - mMin[1];
			float dz = mMax[2] - mMin[2];
			dx*=0.1f; // inflate 1/10th on each edge
			dy*=0.1f; // inflate 1/10th on each edge
			dz*=0.1f; // inflate 1/10th on each edge
			mMin[0]-=dx;
			mMin[1]-=dy;
			mMin[2]-=dz;
			mMax[0]+=dx;
			mMax[1]+=dy;
			mMax[2]+=dz;
		}

		~CHull(void)
		{
			HACD_FREE(mVertices);
			HACD_FREE(mIndices);
		}

		bool overlap(const CHull &h) const
		{
			return fm_intersectAABB(mMin,mMax, h.mMin, h.mMax );
		}

		uint32_t			mGuid;
		float		mMin[3];
		float		mMax[3];
		float		mVolume;
		float		mDiagonal; // long edge..
		uint32_t			mVertexCount;
		uint32_t			mTriangleCount;
		float			*mVertices;
		uint32_t			*mIndices;
	};

	// Usage: std::sort( list.begin(), list.end(), StringSortRef() );
	class CHullSort
	{
	public:

		bool operator()(const CHull *a,const CHull *b) const
		{
			return a->mVolume < b->mVolume;
		}
	};



typedef hacd::vector< CHull * > CHullVector;

class MyMergeHullsInterface : public MergeHullsInterface, public UANS::UserAllocated
{
public:
	MyMergeHullsInterface(void)
	{
		mHasBeenTested = NULL;
	}

	virtual ~MyMergeHullsInterface(void)
	{

	}

	// Merge these input hulls.
	virtual uint32_t mergeHulls(const MergeHullVector &inputHulls,
		MergeHullVector &outputHulls,
		uint32_t mergeHullCount,
		float smallClusterThreshold,
		uint32_t maxHullVertices,
		hacd::ICallback *callback)
	{
		mGuid = 0;

		uint32_t count = (uint32_t)inputHulls.size();
		mHasBeenTested = HACD_NEW(TestedMap)(count*count);
		mSmallClusterThreshold = smallClusterThreshold;
		mMaxHullVertices = maxHullVertices;
		mMergeNumHulls = mergeHullCount;

		mTotalVolume = 0;
		for (uint32_t i=0; i<inputHulls.size(); i++)
		{
			const MergeHull &h = inputHulls[i];
			CHull *ch = HACD_NEW(CHull)(h.mVertexCount,h.mVertices,h.mTriangleCount,h.mIndices,mGuid++);
			mChulls.push_back(ch);
			mTotalVolume+=ch->mVolume;
			if ( callback )
			{
				float fraction = (float)i / (float)inputHulls.size();
				callback->ReportProgress("Gathering Hulls To Merge", fraction );
			}
		}

		//
		uint32_t mergeCount = count - mergeHullCount;
		uint32_t mergeIndex = 0;

		for(;;)
		{
			if ( callback )
			{
				float fraction = (float)mergeIndex / (float)mergeCount;
				callback->ReportProgress("Merging", fraction );
			}
			bool combined = combineHulls(); // mege smallest hulls first, up to the max merge count.
			if ( !combined ) break;
			mergeIndex++;
		} 

		// return results..
		for (uint32_t i=0; i<mChulls.size(); i++)
		{
			CHull *ch = mChulls[i];
			MergeHull mh;
			mh.mVertexCount = ch->mVertexCount;
			mh.mTriangleCount = ch->mTriangleCount;
			mh.mIndices = ch->mIndices;
			mh.mVertices = ch->mVertices;
			outputHulls.push_back(mh);
			if ( callback )
			{
				float fraction = (float)i / (float)mChulls.size();
				callback->ReportProgress("Gathering Merged Hulls Output", fraction );
			}

		}
		delete mHasBeenTested;
		return (uint32_t)outputHulls.size();
	}

	virtual void ConvexDecompResult(uint32_t hvcount,const float *hvertices,uint32_t htcount,const uint32_t *hindices)
	{
		CHull *ch = HACD_NEW(CHull)(hvcount,hvertices,htcount,hindices,mGuid++);
		if ( ch->mVolume > 0.00001f )
		{
			mChulls.push_back(ch);
		}
		else
		{
			delete ch;
		}
	}


	virtual void release(void) 
	{
		delete this;
	}

	static float canMerge(CHull *a,CHull *b)
	{
		if ( !a->overlap(*b) ) return 0; // if their AABB's (with a little slop) don't overlap, then return.

		// ok..we are going to combine both meshes into a single mesh
		// and then we are going to compute the concavity...
		float ret = 0;

		uint32_t combinedVertexCount = a->mVertexCount + b->mVertexCount;
		float *combinedVertices = (float *)HACD_ALLOC(combinedVertexCount*sizeof(float)*3);
		float *dest = combinedVertices;
		memcpy(dest,a->mVertices, sizeof(float)*3*a->mVertexCount);
		dest+=a->mVertexCount*3;
		memcpy(dest,b->mVertices,sizeof(float)*3*b->mVertexCount);

		HullResult hresult;
		HullLibrary hl;
		HullDesc   desc;
		desc.mVcount       = combinedVertexCount;
		desc.mVertices     = combinedVertices;
		desc.mVertexStride = sizeof(float)*3;
		desc.mUseWuQuantizer = true;
		HullError hret = hl.CreateConvexHull(desc,hresult);
		HACD_ASSERT( hret == QE_OK );
		if ( hret == QE_OK )
		{
			ret  = fm_computeMeshVolume( hresult.mOutputVertices, hresult.mNumTriangles, hresult.mIndices );
		}
		HACD_FREE(combinedVertices);
		hl.ReleaseResult(hresult);
		return ret;
	}


	CHull * doMerge(CHull *a,CHull *b)
	{
		CHull *ret = 0;
		uint32_t combinedVertexCount = a->mVertexCount + b->mVertexCount;
		float *combinedVertices = (float *)HACD_ALLOC(combinedVertexCount*sizeof(float)*3);
		float *dest = combinedVertices;
		memcpy(dest,a->mVertices, sizeof(float)*3*a->mVertexCount);
		dest+=a->mVertexCount*3;
		memcpy(dest,b->mVertices,sizeof(float)*3*b->mVertexCount);
		HullResult hresult;
		HullLibrary hl;
		HullDesc   desc;
		desc.mVcount       = combinedVertexCount;
		desc.mVertices     = combinedVertices;
		desc.mVertexStride = sizeof(float)*3;
		desc.mMaxVertices = mMaxHullVertices;
		desc.mUseWuQuantizer = true;
		HullError hret = hl.CreateConvexHull(desc,hresult);
		HACD_ASSERT( hret == QE_OK );
		if ( hret == QE_OK )
		{
			ret = HACD_NEW(CHull)(hresult.mNumOutputVertices, hresult.mOutputVertices, hresult.mNumTriangles, hresult.mIndices,mGuid++);
		}
		HACD_FREE(combinedVertices);
		hl.ReleaseResult(hresult);
		return ret;
	}

	class CombineVolumeJob 
	{
	public:
		CombineVolumeJob(CHull *hullA,CHull *hullB,uint32_t hashIndex)
		{
			mHullA		= hullA;
			mHullB		= hullB;
			mHashIndex	= hashIndex;
            mCombinedVolume = 0;
		}

		virtual ~CombineVolumeJob() {}

		void startJob(void)
		{
			job_process(NULL,0);
		}

		virtual void job_process(void * /*userData*/,int32_t /*userId*/)   // RUNS IN ANOTHER THREAD!! MUST BE THREAD SAFE!
		{
			mCombinedVolume = canMerge(mHullA,mHullB);
		}

		virtual void job_onFinish(void * /*userData*/,int32_t /*userId*/)  // runs in primary thread of the context
		{
			gCombineCount--;
		}

		virtual void job_onCancel(void * /*userData*/,int32_t /*userId*/)  // runs in primary thread of the context
		{

		}

	//private:
		uint32_t	mHashIndex;
		CHull	*mHullA;
		CHull	*mHullB;
		float	mCombinedVolume;
	};

	bool combineHulls(void)
	{
		bool combine = false;
		// each new convex hull is given a unique guid.
		// A hash map is used to make sure that no hulls are tested twice.
		CHullVector output;
		uint32_t count = (uint32_t)mChulls.size();

		// Early out to save walking all the hulls. Hulls are combined based on 
		// a target number or on a number of generated hulls.
		bool mergeTargetMet = (uint32_t)mChulls.size() <= mMergeNumHulls;
		if (mergeTargetMet && (mSmallClusterThreshold == 0.0f))
			return false;		

		hacd::vector< CombineVolumeJob > jobs;

		// First, see if there are any pairs of hulls who's combined volume we have not yet calculated.
		// If there are, then we add them to the jobs list
		{
			for (uint32_t i=0; i<count; i++)
			{
				CHull *cr = mChulls[i];
				for (uint32_t j=i+1; j<count; j++)
				{
					CHull *match = mChulls[j];
					uint32_t hashIndex;
					if ( match->mGuid < cr->mGuid )
					{
						hashIndex = (match->mGuid << 16) | cr->mGuid;
					}
					else
					{
						hashIndex = (cr->mGuid << 16 ) | match->mGuid;
					}

					float *v = mHasBeenTested->find(hashIndex);

					if ( v == NULL )
					{
						CombineVolumeJob job(cr,match,hashIndex);
						jobs.push_back(job);
						(*mHasBeenTested)[hashIndex] = 0.0f;  // assign it to some value so we don't try to create more than one job for it.
					}
				}
			}
		}

		// ok..we have posted all of the jobs, let's let's solve them in parallel
		for (uint32_t i=0; i<jobs.size(); i++)
		{
			jobs[i].startJob();
		}


		// once we have the answers, now put the results into the hash table.
		for (uint32_t i=0; i<jobs.size(); i++)
		{
			CombineVolumeJob &job = jobs[i];
			(*mHasBeenTested)[job.mHashIndex] = job.mCombinedVolume;
		}

		float bestVolume = 1e9;
		CHull *mergeA = NULL;
		CHull *mergeB = NULL;
		// now find the two hulls which merged produce the smallest combined volume.
		{
			for (uint32_t i=0; i<count; i++)
			{
				CHull *cr = mChulls[i];
				for (uint32_t j=i+1; j<count; j++)
				{
					CHull *match = mChulls[j];
					uint32_t hashIndex;
					if ( match->mGuid < cr->mGuid )
					{
						hashIndex = (match->mGuid << 16) | cr->mGuid;
					}
					else
					{
						hashIndex = (cr->mGuid << 16 ) | match->mGuid;
					}
					float *v = mHasBeenTested->find(hashIndex);
					HACD_ASSERT(v);
					if ( v && *v != 0 && *v < bestVolume )
					{
						bestVolume = *v;
						mergeA = cr;
						mergeB = match;
					}
				}
			}
		}

		// If we found a merge pair, and we are below the merge threshold or we haven't reduced to the target
		// do the merge.
		bool thresholdBelow = ((bestVolume / mTotalVolume) * 100.0f) < mSmallClusterThreshold;
		if ( mergeA && (thresholdBelow || !mergeTargetMet))
		{
			CHull *merge = doMerge(mergeA,mergeB);

			float volumeA = mergeA->mVolume;
			float volumeB = mergeB->mVolume;

			if ( merge )
			{
				combine = true;
				output.push_back(merge);
				for (CHullVector::iterator j=mChulls.begin(); j!=mChulls.end(); ++j)
				{
					CHull *h = (*j);
					if ( h !=mergeA && h != mergeB )
					{
						output.push_back(h);
					}
				}
				delete mergeA;
				delete mergeB;
				// Remove the old volumes and add the new one.
				mTotalVolume -= (volumeA + volumeB);
				mTotalVolume += merge->mVolume;
			}
			mChulls = output;
		}

		return combine;
	}

private:
	TestedMap			*mHasBeenTested;
	uint32_t				mGuid;
	float				mTotalVolume;
	float				mSmallClusterThreshold;
	uint32_t				mMergeNumHulls;
	uint32_t				mMaxHullVertices;
	CHullVector			mChulls;
};

MergeHullsInterface * createMergeHullsInterface(void)
{
	MyMergeHullsInterface *m = HACD_NEW(MyMergeHullsInterface);
	return static_cast< MergeHullsInterface *>(m);
}


};
