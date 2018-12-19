// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef D3D11_RENDERER_MEMORY_MACROS_H
#define D3D11_RENDERER_MEMORY_MACROS_H

namespace SampleRenderer
{

template<class T> 
PX_INLINE void dxSafeRelease( T*& t ) { if(t) { t->Release(); t = NULL; } }

template<class T>
PX_INLINE void deleteSafe( T*& t ) { if(t) { delete t; t = NULL; } }

template<class T>
PX_INLINE bool deleteAndReturnTrue( T& t ) { deleteSafe(t); return true; }

template<class T>
PX_INLINE bool dxReleaseAndReturnTrue( T& t ) { dxSafeRelease(t); return true; }

template<class T>
PX_INLINE void deleteAll( T& t ) { std::remove_if(t.begin(), t.end(), deleteAndReturnTrue<typename T::value_type>); };

template<class T>
PX_INLINE void dxSafeReleaseAll( T& t ) { std::remove_if(t.begin(), t.end(), dxReleaseAndReturnTrue<typename T::value_type>); };

template<class T>
PX_INLINE void resizeIfSmallerThan( T& t, typename T::size_type s, typename T::value_type v = typename T::value_type() ) 
{
	if (s > t.size())
		t.resize(s, v);
}

}

#endif
