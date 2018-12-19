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

#ifndef __dgTree__
#define __dgTree__

#include "dgTypes.h"

// Note: this is a low level class for dgTree use only
// unpredictable result will happen if you attempt to manipulate
// any member of this class
class dgRedBackNode : public UANS::UserAllocated
{
	public:
	enum REDBLACK_COLOR
	{
		RED = true,
		BLACK = false
	};

	dgRedBackNode()
	{
	}

	virtual ~dgRedBackNode () 
	{
	}

	void RemoveAllLow ();
	void RotateLeft(dgRedBackNode** const head); 
	void RotateRight(dgRedBackNode** const head); 
	void RemoveFixup (dgRedBackNode* const node, dgRedBackNode* * const head); 

	dgRedBackNode* GetLeft() const;
	dgRedBackNode* GetRight() const;
	dgRedBackNode* GetParent() const;

	dgRedBackNode (dgRedBackNode* const parent);
	inline void Initdata (dgRedBackNode* const parent);
	inline void SetColor (REDBLACK_COLOR color);
	REDBLACK_COLOR GetColor () const;
	uint32_t IsInTree () const;
	inline void SetInTreeFlag (uint32_t flag);

	void RemoveAll ();
	dgRedBackNode* Prev() const;
	dgRedBackNode* Next() const;
	dgRedBackNode* Minimum() const;
	dgRedBackNode* Maximum() const;
	void Remove (dgRedBackNode** const head);
	void Unlink (dgRedBackNode** const head);
	void InsertFixup(dgRedBackNode** const head); 

	dgRedBackNode* m_left;
	dgRedBackNode* m_right;
	dgRedBackNode* m_parent;
	uint32_t  m_color	: 1;
	uint32_t  m_inTree	: 1;
//	uint32_t  m_pad[2];
};

template<class OBJECT, class KEY>
class dgTree 
{
	public:
	class dgTreeNode: public dgRedBackNode
	{
		dgTreeNode (
			const OBJECT &info, 
			const KEY &key, 
			dgTreeNode* parentNode)
			:dgRedBackNode(parentNode), m_info (info), m_key (key)
		{
//			HACD_ASSERT ((uint64_t (&m_info) & 0x0f) == 0);
		}

		~dgTreeNode () 
		{
		}

		dgTreeNode* GetLeft () const
		{
			return (dgTreeNode* )dgRedBackNode::m_left;
		}

		dgTreeNode* GetRight () const
		{
			return (dgTreeNode* )dgRedBackNode::m_right;
		}

		dgTreeNode* GetParent ()
		{
			return (dgTreeNode* )dgRedBackNode::m_parent;
		}

		void SetLeft (dgTreeNode* const node)
		{
			dgRedBackNode::m_left = node;
		}

		void SetRight (dgTreeNode* const node)
		{
			dgRedBackNode::m_right = node;
		}

		void SetParent (dgTreeNode* const node)
		{
			dgRedBackNode::m_parent = node;
		}

		public:
		const KEY& GetKey() const
		{
			return m_key;
		}

		OBJECT& GetInfo()
		{
			return m_info;
		}

		private:
		OBJECT m_info;
		KEY m_key; 
		friend class dgTree<OBJECT, KEY>;

	};

	class Iterator
	{

		public:
		Iterator(const dgTree<OBJECT,KEY> &me)
		{
			m_ptr = NULL;
			m_tree = &me;
		}

		~Iterator()
		{
		}

		void Begin() 
		{
			m_ptr = m_tree->Minimum();
		}

		void End()  
		{
			m_ptr = m_tree->Maximum();
		}

		void Set (dgTreeNode* const node)
		{
			m_ptr = node;
		}

		operator int32_t() const 
		{
			return m_ptr != NULL;
		}

		void operator++ ()
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator++ (int32_t)
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->Next();
		}

		void operator-- () 
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		void operator-- (int32_t) 
		{
			HACD_ASSERT (m_ptr);
			m_ptr = m_ptr->Prev();
		}

		OBJECT &operator* () const 
		{
			return ((dgTreeNode*)m_ptr)->GetInfo();
		}

		dgTreeNode* GetNode() const
		{
			return (dgTreeNode*)m_ptr;
		}

		KEY GetKey () const
		{
			dgTreeNode* const tmp = (dgTreeNode*)m_ptr;
			return tmp ? tmp->GetKey() : KEY(0);
		}

		private:
		dgRedBackNode* m_ptr;
		const dgTree* m_tree;

	};


	// ***********************************************************
	// member functions
	// ***********************************************************
	public:

//	dgTree ();
	dgTree (void);
	virtual ~dgTree (); 


	operator int32_t() const;
	int32_t GetCount() const;

	dgTreeNode* GetRoot () const;
	dgTreeNode* Minimum () const;
	dgTreeNode* Maximum () const;

	dgTreeNode* Find (KEY key) const;
	dgTreeNode* FindGreater (KEY key) const;
	dgTreeNode* FindGreaterEqual (KEY key) const;
	dgTreeNode* FindLessEqual (KEY key) const;

	dgTreeNode* GetNodeFromInfo (OBJECT &info) const;

	dgTreeNode* Insert (const OBJECT &element, KEY key, bool& elementWasInTree);
	dgTreeNode* Insert (const OBJECT &element, KEY key);
	dgTreeNode* Insert (dgTreeNode* const node, KEY key);

	dgTreeNode* Replace (OBJECT &element, KEY key);
	dgTreeNode* ReplaceKey (KEY oldKey, KEY newKey);
	dgTreeNode* ReplaceKey (dgTreeNode* const node, KEY key);

	void Remove (KEY key);
	void Remove (dgTreeNode* const node);
	void RemoveAll (); 

	void Unlink (dgTreeNode* const node);
	void SwapInfo (dgTree& tree);

	bool SanityCheck () const;

	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	int32_t m_count;
	dgTreeNode* m_head;

	int32_t CompareKeys (const KEY &key0, const KEY &key1) const;
	bool SanityCheck (dgTreeNode* const ptr, int32_t height) const;

	friend class dgTreeNode;
};


inline dgRedBackNode::dgRedBackNode (dgRedBackNode* const parent)
{
	Initdata (parent);
}

inline void dgRedBackNode::Initdata (dgRedBackNode* const parent)
{
	SetColor (RED);
	SetInTreeFlag (true);
	m_left = NULL;
	m_right = NULL;
	m_parent = parent;
}

inline void dgRedBackNode::SetColor (dgRedBackNode::REDBLACK_COLOR color)
{
	m_color = color;
}



inline dgRedBackNode::REDBLACK_COLOR  dgRedBackNode::GetColor () const
{
	return REDBLACK_COLOR (m_color);
}


inline void dgRedBackNode::SetInTreeFlag (uint32_t flag)
{
	m_inTree = flag;
}

inline uint32_t dgRedBackNode::IsInTree () const
{
	return m_inTree;
}

/*
template<class OBJECT, class KEY>
dgTree<OBJECT, KEY>::dgTree ()
{
	m_count	= 0;
	m_head = NULL;
}
*/

template<class OBJECT, class KEY>
dgTree<OBJECT, KEY>::dgTree (void)
{
	m_count	= 0;
	m_head = NULL;
}


template<class OBJECT, class KEY>
dgTree<OBJECT, KEY>::~dgTree () 
{
	RemoveAll();
}

template<class OBJECT, class KEY>
dgTree<OBJECT, KEY>::operator int32_t() const
{
	return m_head != NULL;
}

template<class OBJECT, class KEY>
int32_t dgTree<OBJECT, KEY>::GetCount() const
{
	return m_count;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Minimum () const
{
	return m_head ? (dgTreeNode* )m_head->Minimum() : NULL;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Maximum () const
{
	return m_head ? (dgTreeNode* )m_head->Maximum() : NULL;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::GetRoot () const
{
	return m_head;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Find (KEY key) const
{
	if (m_head == NULL) {
		return NULL;
	}

	dgTreeNode* ptr = m_head;
	while (ptr != NULL) {
		if (key < ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == -1) ;
			ptr = ptr->GetLeft();
		} else if (key > ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 1) ;
			ptr = ptr->GetRight();
		} else {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 0) ;
			break;
		}
	}
	return ptr;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::GetNodeFromInfo (OBJECT &info) const
{
	dgTreeNode* const node = (dgTreeNode* ) &info;
	int64_t offset = ((char*) &node->m_info) - ((char *) node);
	dgTreeNode* const retnode = (dgTreeNode* ) (((char *) node) - offset);

	HACD_ASSERT (retnode->IsInTree ());
	HACD_ASSERT (&retnode->GetInfo () == &info);
	return (retnode->IsInTree ()) ? retnode : NULL;

}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::FindGreater (KEY key) const
{
	if (m_head == NULL) {
		return NULL;
	}

	dgTreeNode* prev = NULL;
	dgTreeNode* ptr = m_head;

	while (ptr != NULL) {
		if (key < ptr->m_key) {
		prev = ptr;
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

#ifdef __ENABLE_SANITY_CHECK
	if (prev) {
		Iterator iter (*this);
		for (iter.Begin(); iter.GetNode() != prev; iter ++) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 <= key);
		}
		for (; iter.GetNode(); iter ++) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 > key);
		}
	}
#endif

	return (dgTreeNode* )prev; 
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::FindGreaterEqual (KEY key) const
{
	if (m_head == NULL) {
		return NULL;
	}

	dgTreeNode* prev = NULL;
	dgTreeNode* ptr = m_head;
	
	while (ptr != NULL) {
		if (key == ptr->m_key) {
			return ptr;
		}
		if (key < ptr->m_key) {
		prev = ptr;
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

#ifdef __ENABLE_SANITY_CHECK
	if (prev) {
		Iterator iter (*this);
		for (iter.Begin(); iter.GetNode() != prev; iter ++) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 <= key);
		}
		for (; iter.GetNode(); iter ++) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 >= key);
		}
	}
#endif

	return (dgTreeNode* )prev; 
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::FindLessEqual (KEY key) const
{
	if (m_head == NULL) {
		return NULL;
	}

	dgTreeNode* prev = NULL;
	dgTreeNode* ptr = m_head;

	while (ptr != NULL) {
		if (key == ptr->m_key) {
			return ptr;
		}

		if (key < ptr->m_key) {
			ptr = ptr->GetLeft();
		} else {
			prev = ptr;
			ptr = ptr->GetRight();
		}

	}

#ifdef __ENABLE_SANITY_CHECK
	if (prev) {
		Iterator iter (*this);
		for (iter.End(); iter.GetNode() != prev; iter --) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 >= key);
		}
		for (; iter.GetNode(); iter --) {
			KEY key1 = iter.GetKey(); 
			HACD_ASSERT (key1 < key);
		}
	}
#endif

	return (dgTreeNode* )prev; 
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Insert (const OBJECT &element, KEY key, bool& elementWasInTree)
{
	dgTreeNode* parent = NULL;
	dgTreeNode* ptr = m_head;
	int32_t val = 0;
	elementWasInTree = false;
	while (ptr != NULL) {
		parent = ptr;

		if (key < ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == -1) ;
			val = -1;
			ptr = ptr->GetLeft();
		} else if (key > ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 1) ;
			val = 1;
			ptr = ptr->GetRight();
		} else {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 0) ;
			elementWasInTree = true;
			return ptr;
		}
	}

	m_count	++;
	ptr = HACD_NEW(dgTreeNode)(element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}

	dgTreeNode** const headPtr = (dgTreeNode**) &m_head;
//	ptr->InsertFixup ((dgRedBackNode**)&m_head);
	ptr->InsertFixup ((dgRedBackNode**)headPtr);
	return ptr;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Insert (const OBJECT &element, KEY key)
{
	bool foundState;

	dgTreeNode* const node = Insert (element, key, foundState);
	if (foundState) {
		return NULL;
	}
	return node;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Insert (typename dgTree<OBJECT, KEY>::dgTreeNode* const node, KEY key)
{
	int32_t val = 0;
	dgTreeNode* ptr = m_head;
	dgTreeNode* parent = NULL;
	while (ptr != NULL) {
		parent = ptr;
//		val = CompareKeys (ptr->m_key, key);
//		if (val < 0) {
//			ptr = ptr->GetLeft();
//		} else if (val > 0) {
//			ptr = ptr->GetRight();
//		} else {
//			return NULL;
//		}

		if (key < ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == -1) ;
			val = -1;
			ptr = ptr->GetLeft();
		} else if (key > ptr->m_key) {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 1) ;
			val = 1;
			ptr = ptr->GetRight();
		} else {
			HACD_ASSERT (CompareKeys (ptr->m_key, key) == 0) ;
			return NULL;
		}
	}

	m_count	++;

	ptr = node;
	ptr->m_key = key;
	ptr->Initdata (parent);

	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}

	dgTreeNode** const headPtr = (dgTreeNode**) &m_head;
//	ptr->InsertFixup ((dgRedBackNode**)&m_head);
	ptr->InsertFixup ((dgRedBackNode**)headPtr);
	return ptr;
}


template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::Replace (OBJECT &element, KEY key)
{
	dgTreeNode* parent = NULL;
	dgTreeNode* ptr = m_head;
	int32_t val = 0;
	while (ptr != NULL) {
		parent = ptr;

		HACD_ASSERT (0);
		val = CompareKeys (ptr->m_key, key);
		if (val == 0) {
			ptr->m_info = element;
			return ptr;
		}
		if (val < 0) {
			ptr = ptr->GetLeft();
		} else {
			ptr = ptr->GetRight();
		}
	}

	ptr = HACD_NEW(dgTreeNode) (element, key, parent);
	if (!parent) {
		m_head = ptr;
	} else {
		if (val < 0) {
			parent->m_left = ptr; 
		} else {
			parent->m_right = ptr;
		}
	}

	dgTreeNode** const headPtr = (dgTreeNode**) &m_head;
//	ptr->InsertFixup ((dgRedBackNode**)&m_head);
	ptr->InsertFixup ((dgRedBackNode**)headPtr );
	return ptr;
}


template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::ReplaceKey (typename dgTree<OBJECT, KEY>::dgTreeNode* const node, KEY key)
{
	Unlink (node);
	dgTreeNode* const ptr = Insert (node, key);
	HACD_ASSERT (ptr);
	return ptr;
}

template<class OBJECT, class KEY>
typename dgTree<OBJECT, KEY>::dgTreeNode* dgTree<OBJECT, KEY>::ReplaceKey (KEY oldKey, KEY newKey)
{
	dgTreeNode* const node = Find (oldKey);
	return node ? ReplaceKey (node, newKey) : NULL;
}

template<class OBJECT, class KEY>
void dgTree<OBJECT, KEY>::Unlink (typename dgTree<OBJECT, KEY>::dgTreeNode* const node)
{
	m_count	--;

	dgTreeNode** const headPtr = (dgTreeNode**) &m_head;
	node->Unlink ((dgRedBackNode**)headPtr);
	HACD_ASSERT (!Find (node->GetKey()));
}


template<class OBJECT, class KEY>
void dgTree<OBJECT, KEY>::Remove (typename dgTree<OBJECT, KEY>::dgTreeNode* const node)
{
	m_count	--;
	dgTreeNode** const headPtr = (dgTreeNode**) &m_head;
	node->Remove ((dgRedBackNode**)headPtr);
}

template<class OBJECT, class KEY>
void dgTree<OBJECT, KEY>::Remove (KEY key) 
{
	dgTreeNode* node;

	// find node in tree 
	node = Find (key);
	if (node == NULL) {
		return;
	}
	Remove (node);
}

template<class OBJECT, class KEY>
void dgTree<OBJECT, KEY>::RemoveAll () 
{
	if (m_head) {
		m_count	 = 0;
		m_head->RemoveAll ();
		m_head = NULL;
	}
}

template<class OBJECT, class KEY>
bool dgTree<OBJECT, KEY>::SanityCheck () const
{
	return SanityCheck (m_head, 0);
}


template<class OBJECT, class KEY>
bool dgTree<OBJECT, KEY>::SanityCheck (typename dgTree<OBJECT, KEY>::dgTreeNode* const ptr, int32_t height) const
{
	if (!ptr) {
		return true;
	}

	if (!ptr->IsInTree()) {
		return false;
	}

	if (ptr->m_left) {
		if (CompareKeys (ptr->m_key, ptr->GetLeft()->m_key) > 0) {
			return false;
		}
	}

	if (ptr->m_right) {
		if (CompareKeys (ptr->m_key, ptr->GetRight()->m_key) < 0) {
			return false;
		}
	}

	if (ptr->GetColor() == dgTreeNode::BLACK) {
		height ++;
	} else if (!((!ptr->m_left  || (ptr->m_left->GetColor() == dgTreeNode::BLACK)) &&
			       (!ptr->m_right || (ptr->m_right->GetColor() == dgTreeNode::BLACK)))) {
	  	return false;
	}

	if (!ptr->m_left && !ptr->m_right) {
		int32_t bh = 0;
		for (dgTreeNode* x = ptr; x; x = x->GetParent()) {
	 		if (x->GetColor() == dgTreeNode::BLACK) {
				bh ++;
			}
		}
		if (bh != height) {
			return false;
		}
	}

	if (ptr->m_left && !SanityCheck (ptr->GetLeft(), height)) {
		return false;
	}

	if (ptr->m_right && !SanityCheck (ptr->GetRight(), height)) {
		return false;
	}
	return true;
}


template<class OBJECT, class KEY>
int32_t dgTree<OBJECT, KEY>::CompareKeys (const KEY &key0, const KEY &key1) const
{
	if (key1 < key0) {
		return - 1;
	}
	if (key1 > key0) {
		return 1;
	}
	return 0;
}

template<class OBJECT, class KEY>
void dgTree<OBJECT, KEY>::SwapInfo (dgTree<OBJECT, KEY>& tree)
{
	Swap (m_head, tree.m_head);
	Swap (m_count, tree.m_count);
}


#endif


