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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#ifndef __dgArray__
#define __dgArray__

#include "dgTypes.h"

template<class T>
class dgArray
{
	public:
	dgArray (int32_t granulatitySize,int32_t aligmentInBytes = DG_MEMORY_GRANULARITY);
	~dgArray ();


	T& operator[] (int32_t i);
	const T& operator[] (int32_t i) const;
	void Resize (int32_t size) const;
	int32_t GetBytesCapacity () const;
	int32_t GetElementsCapacity () const; 

	bool ExpandCapacityIfNeessesary (int32_t index, int32_t stride) const;

	private:
	int32_t m_granulatity;
	int32_t m_aligmentInByte;
	mutable int32_t m_maxSize;
	mutable T *m_array;
};


template<class T>
dgArray<T>::dgArray (int32_t granulatitySize,int32_t aligmentInBytes)
 :m_granulatity(granulatitySize), m_aligmentInByte(aligmentInBytes), m_maxSize(0), m_array(NULL)
{
	if (m_aligmentInByte <= 0) {
		m_aligmentInByte = 8;
	}
	m_aligmentInByte = 1 << exp_2(m_aligmentInByte);
}

template<class T>
dgArray<T>::~dgArray ()
{
	if (m_array) {
		HACD_FREE(m_array);
	}
}


template<class T>
const T& dgArray<T>::operator[] (int32_t i) const
{ 
	HACD_ASSERT (i >= 0);
	while (i >= m_maxSize) {
		Resize (i);
	}
	return m_array[i];
}


template<class T>
T& dgArray<T>::operator[] (int32_t i)
{
	HACD_ASSERT (i >= 0);
	while (i >= m_maxSize) {
		Resize (i);
	}
	return m_array[i];
}

template<class T>
int32_t dgArray<T>::GetElementsCapacity () const
{
	return m_maxSize;
}


template<class T>
int32_t dgArray<T>::GetBytesCapacity () const
{
	return  m_maxSize * int32_t (sizeof (T));
}


template<class T>
void dgArray<T>::Resize (int32_t size) const
{
	if (size >= m_maxSize) {
		size = size + m_granulatity - (size + m_granulatity) % m_granulatity;
		T* const newArray = (T*) HACD_ALLOC_ALIGNED(size_t(sizeof (T) * size), m_aligmentInByte);
		if (m_array) {
			for (int32_t i = 0; i < m_maxSize; i ++) {
				newArray[i]	= m_array[i];
			}
			HACD_FREE(m_array);
		}
		m_array = newArray;
		m_maxSize = size;
	} else if (size < m_maxSize) {
		size = size + m_granulatity - (size + m_granulatity) % m_granulatity;
		T* const newArray = (T*) HACD_ALLOC_ALIGNED(size_t(sizeof (T) * size), m_aligmentInByte);
		if (m_array) {
			for (int32_t i = 0; i < size; i ++) {
				newArray[i]	= m_array[i];
			}
			HACD_FREE(m_array);
		}
		m_array = newArray;
		m_maxSize = size;
	}
}

template<class T>
bool dgArray<T>::ExpandCapacityIfNeessesary (int32_t index, int32_t stride) const
{
	bool ret = false;
	int32_t size = (index + 1) * stride;
	while (size >= m_maxSize) {
		ret = true;
		Resize (m_maxSize * 2);
	}
	return ret;
}

#endif




