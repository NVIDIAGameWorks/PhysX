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

#ifndef __dgList__
#define __dgList__

#include "dgTypes.h"

template<class T>
class dgList 
{
	public:
		class dgListNode : public UANS::UserAllocated
	{
		dgListNode (dgListNode* const prev, dgListNode* const next) 
			:m_info () 
		{
//			HACD_ASSERT ((uint64_t (&m_info) & 0x0f) == 0);
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		dgListNode (const T &info, dgListNode* const prev, dgListNode* const next) 
			:m_info (info) 
		{
//			HACD_ASSERT ((uint64_t (&m_info) & 0x0f) == 0);
			m_prev = prev;
			m_next = next;
			if (m_prev) {
				m_prev->m_next = this;
			}
			if (m_next) {
				m_next->m_prev = this;
			}
		}

		virtual ~dgListNode()
		{
//			Unlink ();
		}

		void Unlink ()
		{
			if (m_prev) {
				m_prev->m_next = m_next;
			}

			if (m_next) {
				m_next->m_prev = m_prev;
			}
			m_prev = NULL;
			m_next = NULL;
		}

//		void Remove()
//		{
//			HACD_ASSERT (0);
//			Kill();
//			Unlink();
//			Release();
//		}

		void AddLast(dgListNode* const node) 
		{
			m_next = node;
			node->m_prev = this;
		}

		void AddFirst(dgListNode* const node) 
		{
			m_prev = node;
			node->m_next = this;
		}

		

		public:
		T& GetInfo()
		{
			return m_info;
		}

		dgListNode *GetNext() const
		{
			return m_next;
		}

		dgListNode *GetPrev() const
		{
			return m_prev;
		}

		private:
		T m_info;
		dgListNode *m_next;
		dgListNode *m_prev;
		friend class dgList<T>;

	};

	class Iterator
	{
		public:
		Iterator (const dgList<T> &me)
		{
			m_ptr = NULL;
			m_list = (dgList *)&me;
		}

		~Iterator ()
		{
		}

		operator int32_t() const
		{
			return m_ptr != NULL;
		}

		bool operator== (const Iterator &target) const
		{
			return (m_ptr == target.m_ptr) && (m_list == target.m_list);
		}

		void Begin()
		{
			m_ptr = m_list->GetFirst();
		}

		void End()
		{
			m_ptr = m_list->GetLast();
		}

		void Set (dgListNode* const node)
		{
			m_ptr = node;
		}

		void operator++ ()
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->m_next();
		}

		void operator++ (int32_t)
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->GetNext();
		}

		void operator-- () 
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		void operator-- (int32_t) 
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		T &operator* () const
		{
			return m_ptr->GetInfo();
		}

		dgListNode *GetNode() const
		{
			return m_ptr;
		}

		private:
		dgList *m_list;
		dgListNode *m_ptr;
	};

	// ***********************************************************
	// member functions
	// ***********************************************************
	public:

//	dgList ();
	dgList (void);
	virtual ~dgList ();

	operator int32_t() const;
	int32_t GetCount() const;
	dgListNode *GetLast() const;
	dgListNode *GetFirst() const;
	dgListNode *Append ();
	dgListNode *Append (dgListNode* const node);
	dgListNode *Append (const T &element);
	dgListNode *Addtop ();
	dgListNode *Addtop (dgListNode* const node);
	dgListNode *Addtop (const T &element);
	
	void RotateToEnd (dgListNode* const node);
	void RotateToBegin (dgListNode* const node);
	void InsertAfter (dgListNode* const root, dgListNode* const node);
	void InsertBefore (dgListNode* const root, dgListNode* const node);


	dgListNode *Find (const T &element) const;
	dgListNode *GetNodeFromInfo (T &m_info) const;
	void Remove (dgListNode* const node);
	void Remove (const T &element);
	void RemoveAll ();

	void Merge (dgList<T>& list);
	void Unlink (dgListNode* const node);
	bool SanityCheck () const;



	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	int32_t m_count;
	dgListNode *m_last;
	dgListNode *m_first;

	friend class dgListNode;
};

template<class T>
dgList<T>::dgList (void)
{
	m_count = 0;
	m_first = NULL;
	m_last = NULL;
}


template<class T>
dgList<T>::~dgList () 
{
	RemoveAll ();
}


template<class T>
int32_t dgList<T>::GetCount() const
{
	return m_count;
}

template<class T>
dgList<T>::operator int32_t() const
{
	return m_first != NULL;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetFirst() const
{
	return m_first;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetLast() const
{
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append (dgListNode* const node)
{
	HACD_ASSERT (node->m_next == NULL);
	HACD_ASSERT (node->m_prev == NULL);
	m_count	++;
	if (m_first == NULL) {
		m_last = node;
		m_first = node;
	} else {
		m_last->AddLast (node);
		m_last = node;
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append ()
{
	m_count	++;
	if (m_first == NULL) {
		m_first = HACD_NEW(dgListNode)(NULL, NULL);
		m_last = m_first;
	} else {
		m_last = HACD_NEW(dgListNode)(m_last, NULL);
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Append (const T &element)
{
	m_count	++;
	if (m_first == NULL) {
		m_first = HACD_NEW(dgListNode)(element, NULL, NULL);
		m_last = m_first;
	} else {
		m_last = HACD_NEW(dgListNode)(element, m_last, NULL);
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif

	return m_last;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop (dgListNode* const node)
{
	HACD_ASSERT (node->m_next == NULL);
	HACD_ASSERT (node->m_prev == NULL);
	m_count	++;
	if (m_last == NULL) {
		m_last = node;
		m_first = node;
	} else {
		m_first->AddFirst(node);
		m_first = node;
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	return m_first;
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop ()
{
	m_count	++;
	if (m_last == NULL) {
		m_last = HACD_NEW(dgListNode)(NULL, NULL);
		m_first = m_last;
	} else {
		m_first = HACD_NEW(dgListNode)(NULL, m_first);
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	return m_first;
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Addtop (const T &element)
{
	m_count	++;
	if (m_last == NULL) {
		m_last = HACD_NEW(dgListNode)(element, NULL, NULL);
		m_first = m_last;
	} else {
		m_first = HACD_NEW(dgListNode)(element, NULL, m_first);
	}
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
	return m_first;
}

template<class T>
void dgList<T>::InsertAfter (dgListNode* const root, dgListNode* const node)
{
	HACD_ASSERT (root);
	HACD_ASSERT (node != root);
	
	if (root->m_next != node) {
		if (node == m_first) {
			m_first = node->m_next;
		}
		if (node == m_last) {
			m_last = node->m_prev;
		}
		node->Unlink ();
		
		node->m_prev = root;
		node->m_next = root->m_next;
		if (root->m_next) {
			root->m_next->m_prev = node;
		} 
		root->m_next = node;

		if (node->m_next == NULL) {
			m_last = node;
		}

		HACD_ASSERT (m_last);
		HACD_ASSERT (!m_last->m_next);
		HACD_ASSERT (m_first);
		HACD_ASSERT (!m_first->m_prev);
		HACD_ASSERT (SanityCheck ());
	}
}


template<class T>
void dgList<T>::InsertBefore (dgListNode* const root, dgListNode* const node)
{
	HACD_ASSERT (root);
	HACD_ASSERT (node != root);
	
	if (root->m_prev != node) {
		if (node == m_last) {
			m_last = node->m_prev;
		}
		if (node == m_first) {
			m_first = node->m_next;
		}
		node->Unlink ();
		
		node->m_next = root;
		node->m_prev = root->m_prev;
		if (root->m_prev) {
			root->m_prev->m_next = node;
		} 
		root->m_prev = node;

		if (node->m_prev == NULL) {
			m_first = node;
		}

		HACD_ASSERT (m_first);
		HACD_ASSERT (!m_first->m_prev);
		HACD_ASSERT (m_last);
		HACD_ASSERT (!m_last->m_next);
		HACD_ASSERT (SanityCheck ());
	}
}


template<class T>
void dgList<T>::RotateToEnd (dgListNode* const node)
{
	if (node != m_last) {
		if (m_last != m_first) {
			if (node == m_first) {
				m_first = m_first->GetNext();
			}
			node->Unlink();
			m_last->AddLast(node);
			m_last = node; 
		}
	}

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
}

template<class T>
void dgList<T>::RotateToBegin (dgListNode* const node)
{
	if (node != m_first) {
		if (m_last != m_first) {
			if (node == m_last) {
				m_last = m_last->GetPrev();
			}
			node->Unlink();
			m_first->AddFirst(node);
			m_first = node; 
		}
	}

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
}


template<class T>
typename dgList<T>::dgListNode *dgList<T>::Find (const T &element) const
{
	dgListNode *node;
	for (node = m_first; node; node = node->GetNext()) {
		if (element	== node->m_info) {
			break;
		}
	}
	return node;
}

template<class T>
typename dgList<T>::dgListNode *dgList<T>::GetNodeFromInfo (T &info) const
{
//	int64_t offset;
//	dgListNode *node;

	dgListNode* const node = (dgListNode *) &info;
	int64_t offset = ((char*) &node->m_info) - ((char *) node);
	dgListNode* const retnode = (dgListNode *) (((char *) node) - offset);

	HACD_ASSERT (&retnode->GetInfo () == &info);
	return retnode;
}


template<class T> 
void dgList<T>::Remove (const T &element)
{
	dgListNode *const node = Find (element);
	if (node) {
		Remove (node);
	}
}

template<class T> 
void dgList<T>::Unlink (dgListNode* const node)
{
	HACD_ASSERT (node);

	m_count --;
	HACD_ASSERT (m_count >= 0);

	if (node == m_first) {
		m_first = m_first->GetNext();
	}
	if (node == m_last) {
		m_last = m_last->GetPrev();
	}
//	node->Remove();
	node->Unlink();

#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
}

template<class T> 
void dgList<T>::Merge (dgList<T>& list)
{
	m_count += list.m_count;
	if (list.m_first) {
		list.m_first->m_prev = m_last; 
	}
	if (m_last) {
		m_last->m_next = list.m_first;
	}
	m_last = list.m_last;
	if (!m_first) {
		m_first = list.m_first;
	}

	list.m_count = 0;
	list.m_last = NULL;
	list.m_first = NULL;
#ifdef __ENABLE_SANITY_CHECK 
	HACD_ASSERT (SanityCheck ());
#endif
}


template<class T>
void dgList<T>::Remove (dgListNode* const node)
{
	Unlink (node);
	delete node;
}


template<class T>
void dgList<T>::RemoveAll ()
{
	for (dgListNode *node = m_first; node; node = m_first) {
		m_count --;
		m_first = node->GetNext();
//		node->Remove();
		node->Unlink();
		delete node;
	}
	HACD_ASSERT (m_count == 0);
	m_last = NULL;
	m_first = NULL;
}

template<class T>
bool dgList<T>::SanityCheck () const
{
	#ifdef _DEBUG
	int32_t tCount = 0;
	for (dgListNode * node = m_first; node; node = node->GetNext()) {
		tCount ++;
		if (node->GetPrev()) {
			HACD_ASSERT (node->GetPrev() != node->GetNext());
			if (node->GetPrev()->GetNext() != node) {
				HACD_ASSERT (0);
				return false; 
			}
		}
		if (node->GetNext()) {
			HACD_ASSERT (node->GetPrev() != node->GetNext());
			if (node->GetNext()->GetPrev() != node)	{
				HACD_ASSERT (0);
				return false;
			}
		}
	}
	if (tCount != m_count) {
		HACD_ASSERT (0);
		return false;
	}
	#endif
	return true;
}


#endif


