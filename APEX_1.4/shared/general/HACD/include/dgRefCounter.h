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

#ifndef __DGREF_COUNTER__
#define __DGREF_COUNTER__

class dgRefCounter
{
	public:
	dgRefCounter(void);
	int GetRef() const;
	int Release();
	void AddRef();

	protected:
	virtual ~dgRefCounter(void);

	private:
	int m_refCount;
};


inline dgRefCounter::dgRefCounter(void)
{
	m_refCount = 1;
}

inline dgRefCounter::~dgRefCounter(void)
{
	HACD_ASSERT (m_refCount <= 1);
}


inline int dgRefCounter::GetRef() const
{
	return m_refCount;
}

inline int dgRefCounter::Release()
{
	m_refCount --;
	if (!m_refCount) {
		delete this;
		return 0;
	}
	return m_refCount;
}

inline void dgRefCounter::AddRef()
{
	m_refCount ++;
}

#endif
