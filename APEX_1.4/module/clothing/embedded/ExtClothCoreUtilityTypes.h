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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef EXT_CLOTH_CORE_UTILTY_TYPES_H
#define EXT_CLOTH_CORE_UTILTY_TYPES_H
/** \addtogroup common
@{
*/

#include "PxAssert.h"
#include "PxFlags.h"

#if !PX_DOXYGEN
namespace nvidia
{
#endif


struct PxStridedData
{
	/**
	\brief The offset in bytes between consecutive samples in the data.

	<b>Default:</b> 0
	*/
	uint32_t stride;
	const void* data;

	PxStridedData() : stride( 0 ), data( NULL ) {}

	template<typename TDataType>
	PX_INLINE const TDataType& at( uint32_t idx ) const
	{
		uint32_t theStride( stride );
		if ( theStride == 0 )
			theStride = sizeof( TDataType );
		uint32_t offset( theStride * idx );
		return *(reinterpret_cast<const TDataType*>( reinterpret_cast< const uint8_t* >( data ) + offset ));
	}
};

template<typename TDataType>
struct PxTypedStridedData
{
	uint32_t stride;
	const TDataType* data;

	PxTypedStridedData()
		: stride( 0 )
		, data( NULL )
	{
	}

};

struct PxBoundedData : public PxStridedData
{
	uint32_t count;
	PxBoundedData() : count( 0 ) {}
};

template<uint8_t TNumBytes>
struct PxPadding
{
	uint8_t mPadding[TNumBytes];
	PxPadding()
	{
		for ( uint8_t idx =0; idx < TNumBytes; ++idx )
			mPadding[idx] = 0;
	}
};

template <uint32_t NB_ELEMENTS> class PxFixedSizeLookupTable
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================
public:
	
	PxFixedSizeLookupTable() 
		: mNbDataPairs(0)
	{
	}

	PxFixedSizeLookupTable(const physx::PxEMPTY) {}

	PxFixedSizeLookupTable(const float* dataPairs, const uint32_t numDataPairs)
	{
		memcpy(mDataPairs,dataPairs,sizeof(float)*2*numDataPairs);
		mNbDataPairs=numDataPairs;
	}

	PxFixedSizeLookupTable(const PxFixedSizeLookupTable& src)
	{
		memcpy(mDataPairs,src.mDataPairs,sizeof(float)*2*src.mNbDataPairs);
		mNbDataPairs=src.mNbDataPairs;
	}

	~PxFixedSizeLookupTable()
	{
	}

	PxFixedSizeLookupTable& operator=(const PxFixedSizeLookupTable& src)
	{
		memcpy(mDataPairs,src.mDataPairs,sizeof(float)*2*src.mNbDataPairs);
		mNbDataPairs=src.mNbDataPairs;
		return *this;
	}

	PX_FORCE_INLINE void addPair(const float x, const float y)
	{
		PX_ASSERT(mNbDataPairs<NB_ELEMENTS);
		mDataPairs[2*mNbDataPairs+0]=x;
		mDataPairs[2*mNbDataPairs+1]=y;
		mNbDataPairs++;
	}

	PX_FORCE_INLINE float getYVal(const float x) const
	{
		if(0==mNbDataPairs)
		{
			PX_ASSERT(false);
			return 0;
		}

		if(1==mNbDataPairs || x<getX(0))
		{
			return getY(0);
		}

		float x0=getX(0);
		float y0=getY(0);

		for (uint32_t i = 1; i<mNbDataPairs; i++)
		{
			const float x1=getX(i);
			const float y1=getY(i);

			if((x>=x0)&&(x<x1))
			{
				return (y0+(y1-y0)*(x-x0)/(x1-x0));
			}

			x0=x1;
			y0=y1;
		}

		PX_ASSERT(x>=getX(mNbDataPairs-1));
		return getY(mNbDataPairs-1);
	}

	uint32_t getNbDataPairs() const {return mNbDataPairs;}
	
	void clear()
	{
		memset(mDataPairs, 0, NB_ELEMENTS*2*sizeof(float));
		mNbDataPairs = 0;
	}

	PX_FORCE_INLINE float getX(const uint32_t i) const
	{
		return mDataPairs[2*i];
	}
	PX_FORCE_INLINE float getY(const uint32_t i) const
	{
		return mDataPairs[2*i+1];
	}

	float mDataPairs[2*NB_ELEMENTS];
	uint32_t mNbDataPairs;
	uint32_t mPad[3];

	
};

struct PxMeshFlag
{
	enum Enum
	{
		/**
		\brief Specifies if the SDK should flip normals.

		The PhysX libraries assume that the face normal of a triangle with vertices [a,b,c] can be computed as:
		edge1 = b-a
		edge2 = c-a
		face_normal = edge1 x edge2.

		Note: This is the same as a counterclockwise winding in a right handed coordinate system or
		alternatively a clockwise winding order in a left handed coordinate system.

		If this does not match the winding order for your triangles, raise the below flag.
		*/
		eFLIPNORMALS		=	(1<<0),
		e16_BIT_INDICES		=	(1<<1)	//!< Denotes the use of 16-bit vertex indices
	};
};

/**
\brief collection of set bits defined in PxMeshFlag.

@see PxMeshFlag
*/
typedef physx::PxFlags<PxMeshFlag::Enum, uint16_t> PxMeshFlags;
using physx::PxFlags;
PX_FLAGS_OPERATORS(PxMeshFlag::Enum, uint16_t)

#if !PX_DOXYGEN
} // namespace nvidia
#endif


/** @} */
#endif
