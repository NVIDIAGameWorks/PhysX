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


#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

namespace SampleRenderer
{

class safe_bool_base 
{
protected:
	typedef void (safe_bool_base::*bool_type)() const;
	void this_type_does_not_support_comparisons() const {}

	safe_bool_base() {}
	safe_bool_base(const safe_bool_base&) {}
	safe_bool_base& operator=(const safe_bool_base&) {return *this;}
	~safe_bool_base() {}
};

template <typename T=void> class safe_bool : public safe_bool_base
{
public:
	operator bool_type() const
	{
		return (static_cast<const T*>(this))->boolean_test() ?
			//&safe_bool_base::this_type_does_not_support_comparisons : 0;
			&safe_bool<T>::this_type_does_not_support_comparisons : 0;
	}
protected:
	~safe_bool() {}
};

template<> class safe_bool<void> : public safe_bool_base 
{
public:
	operator bool_type() const 
	{
		return boolean_test()==true ? 
			//&safe_bool_base::this_type_does_not_support_comparisons : 0;
			&safe_bool<void>::this_type_does_not_support_comparisons : 0;
	}
protected:
	virtual bool boolean_test() const=0;
	virtual ~safe_bool() {}
};

template <typename T, typename U> 
PX_INLINE bool operator==(const safe_bool<T>& lhs,const safe_bool<U>& rhs) 
{
	lhs.this_type_does_not_support_comparisons();	
	return false;
}

template <typename T,typename U> 
PX_INLINE bool operator!=(const safe_bool<T>& lhs,const safe_bool<U>& rhs) 
{
	lhs.this_type_does_not_support_comparisons();
	return false;	
}

}

#endif
