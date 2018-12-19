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

#include "dgTypes.h"
#include "dgTree.h"


dgRedBackNode *dgRedBackNode::Minimum () const
{
	dgRedBackNode* ptr = (dgRedBackNode *)this;
	for (; ptr->m_left; ptr = ptr->m_left){}
	return ptr;
}

dgRedBackNode *dgRedBackNode::Maximum () const
{
	dgRedBackNode* ptr = (dgRedBackNode *)this;
	for (; ptr->m_right; ptr = ptr->m_right){}
	return ptr;
}


dgRedBackNode *dgRedBackNode::Prev () const
{
	if (m_left) {
		return m_left->Maximum ();
	}

	dgRedBackNode* node = (dgRedBackNode *)this;
	dgRedBackNode* ptr = m_parent;
	for (; ptr && node == ptr->m_left; ptr = ptr->m_parent) {
		node = ptr;
	}
	return ptr;

}

dgRedBackNode *dgRedBackNode::Next () const
{

	if (m_right) {
		return m_right->Minimum ();
	}

	dgRedBackNode* node = (dgRedBackNode *)this;
	dgRedBackNode* ptr = m_parent;
	for (; ptr && node == ptr->m_right; ptr = ptr->m_parent) {
		node = ptr;
	}
	return ptr;
}

// rotate node me to left 
void dgRedBackNode::RotateLeft(dgRedBackNode** const head) 
{
	dgRedBackNode* const me = this;
	dgRedBackNode* const child = me->m_right;
	
	//establish me->m_right link 
	me->m_right = child->m_left;
	if (child->m_left != NULL) {
		child->m_left->m_parent = me;
	}
	
	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_left) {
			me->m_parent->m_left = child;
		} else {
			me->m_parent->m_right = child;
		}
	} else {
		*head = child;
	}
	
	// link child and me 
	child->m_left = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// rotate node me to right  *
void dgRedBackNode::RotateRight(dgRedBackNode ** const head) 
{
	dgRedBackNode* const me = this;
	dgRedBackNode* const child = me->m_left;

	// establish me->m_left link 
	me->m_left = child->m_right;
	if (child->m_right != NULL) {
		child->m_right->m_parent = me;
	}

	// establish child->m_parent link 
	if (child != NULL) {
		child->m_parent = me->m_parent;
	}
	if (me->m_parent) {
		if (me == me->m_parent->m_right) {
			me->m_parent->m_right = child;
		} else {
			me->m_parent->m_left = child;
		}
	} else {
		*head = child;
	}

	// link me and child 
	child->m_right = me;
	if (me != NULL) {
		me->m_parent = child;
	}
}


// maintain Red-Black tree balance after inserting node ptr   
void dgRedBackNode::InsertFixup(dgRedBackNode ** const head) 
{
	dgRedBackNode* ptr = this;
	// check Red-Black properties 
	while ((ptr != *head) && (ptr->m_parent->GetColor() == RED)) {
		// we have a violation 
		if (ptr->m_parent == ptr->m_parent->m_parent->m_left) {
			dgRedBackNode* const tmp = ptr->m_parent->m_parent->m_right;
			if (tmp && (tmp->GetColor() == RED)) {
				// uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_right) {
					// make ptr a left child 
					ptr = ptr->m_parent;
					ptr->RotateLeft(head);
				}

				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent) {
					ptr->m_parent->m_parent->SetColor(RED);
					ptr->m_parent->m_parent->RotateRight(head);
				}
			}
		} else {
			HACD_ASSERT (ptr->m_parent == ptr->m_parent->m_parent->m_right);
			// mirror image of above code 
			dgRedBackNode* const tmp = ptr->m_parent->m_parent->m_left;
			if (tmp && (tmp->GetColor() == RED)) {
				//uncle is RED 
				ptr->m_parent->SetColor(BLACK);
				tmp->SetColor(BLACK) ;
				ptr->m_parent->m_parent->SetColor(RED) ;
				ptr = ptr->m_parent->m_parent;
			} else {
				// uncle is BLACK 
				if (ptr == ptr->m_parent->m_left) {
					ptr = ptr->m_parent;
					ptr->RotateRight(head);
				}
				ptr->m_parent->SetColor(BLACK);
				if (ptr->m_parent->m_parent->GetColor() == BLACK) {
					ptr->m_parent->m_parent->SetColor(RED) ;
				   ptr->m_parent->m_parent->RotateLeft (head); 
				}
			}
		}
	}
	(*head)->SetColor(BLACK);
}


//maintain Red-Black tree balance after deleting node x 
void dgRedBackNode::RemoveFixup (dgRedBackNode* const thisNode, dgRedBackNode ** const head) 
{
	dgRedBackNode* ptr = this;
	dgRedBackNode* node = thisNode;
	while ((node != *head) && (!node || node->GetColor() == BLACK)) {
		if (node == ptr->m_left) {
			if (!ptr) {
				return;
			}
			dgRedBackNode* tmp = ptr->m_right;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateLeft (head);
				tmp = ptr->m_right;
				if (!ptr || !tmp) {
					return;
				}
			}
			if ((!tmp->m_left  || (tmp->m_left->GetColor() == BLACK)) && 
				 (!tmp->m_right || (tmp->m_right->GetColor() == BLACK))) {
				tmp->SetColor(RED);
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) {
				tmp->m_left->SetColor(BLACK);
				tmp->SetColor(RED);
				tmp->RotateRight (head);
				tmp = ptr->m_right;
				if (!ptr || !tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_right) {
				tmp->m_right->SetColor(BLACK) ;
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateLeft (head);
			}
			node = *head;

		} else {
		  	if (!ptr) {
				return;
		  	}
			dgRedBackNode* tmp = ptr->m_left;
			if (!tmp) {
				return;
			}
			if (tmp->GetColor() == RED) {
				tmp->SetColor(BLACK) ;
				ptr->SetColor(RED) ;
				ptr->RotateRight (head);
				tmp = ptr->m_left;
				if (!ptr || !tmp) {
					return;
				}
			}

			if ((!tmp->m_right || (tmp->m_right->GetColor() == BLACK)) && 
				 (!tmp->m_left  || (tmp->m_left->GetColor() == BLACK))) {
				tmp->SetColor(RED) ;
				node = ptr;
				ptr = ptr->m_parent;
				continue;
			} else if (!tmp->m_left || (tmp->m_left->GetColor() == BLACK)) {
				tmp->m_right->SetColor(BLACK) ;
				tmp->SetColor(RED) ;
				tmp->RotateLeft (head);
				tmp = ptr->m_left;
				if (!ptr || !tmp) {
					return;
				}
			}
			tmp->SetColor (ptr->GetColor());
			if (tmp->m_left) {
				tmp->m_left->SetColor(BLACK);
			}
			if (ptr) {
				ptr->SetColor(BLACK) ;
				ptr->RotateRight (head);
			}
			node = *head;
		}
	}
	if (node) {
		node->SetColor(BLACK);
	}
}

void dgRedBackNode::Unlink (dgRedBackNode ** const head) 
{
//	dgRedBackNode *child;
//	dgRedBackNode *endNode;
//	dgRedBackNode *endNodeParent;
//	dgRedBackNode::REDBLACK_COLOR oldColor;

	dgRedBackNode* const node = this;
//	node->Kill();
	node->SetInTreeFlag(false);

	if (!node->m_left || !node->m_right) {
		// y has a NULL node as a child 
		dgRedBackNode* const endNode = node;

		// x is y's only child 
		dgRedBackNode* child = endNode->m_right;
		if (endNode->m_left) {
			child = endNode->m_left;
		}

		// remove y from the parent chain 
		if (child) {
			child->m_parent = endNode->m_parent;
		}

		if (endNode->m_parent)	{
			if (endNode == endNode->m_parent->m_left) {
				endNode->m_parent->m_left = child;
			} else {
				endNode->m_parent->m_right = child;
			}
		} else {
			*head = child;
		}

		if (endNode->GetColor() == BLACK) {
			endNode->m_parent->RemoveFixup (child, head);
		}
//		endNode->Release();
//		delete endNode;

	} else {

		// find tree successor with a NULL node as a child 
		dgRedBackNode* endNode = node->m_right;
		while (endNode->m_left != NULL) {
			endNode = endNode->m_left;
		}

		HACD_ASSERT (endNode);
		HACD_ASSERT (endNode->m_parent);
		HACD_ASSERT (!endNode->m_left);

		// x is y's only child 
		dgRedBackNode* const child = endNode->m_right;

		HACD_ASSERT ((endNode != node->m_right) || !child || (child->m_parent == endNode));

		endNode->m_left = node->m_left;
		node->m_left->m_parent = endNode;

		dgRedBackNode* endNodeParent = endNode;
		if (endNode != node->m_right) {
			if (child) {
				child->m_parent = endNode->m_parent;
			}
			endNode->m_parent->m_left = child;
			endNode->m_right = node->m_right;
			node->m_right->m_parent = endNode;
			endNodeParent = endNode->m_parent;
		}


		if (node == *head) {
			*head = endNode;
		} else if (node == node->m_parent->m_left) {
			node->m_parent->m_left = endNode;
		} else {
			node->m_parent->m_right = endNode;
		}
		endNode->m_parent = node->m_parent;

		dgRedBackNode::REDBLACK_COLOR oldColor = endNode->GetColor();
		endNode->SetColor (node->GetColor());
		node->SetColor	(oldColor);

		if (oldColor == BLACK) {
			endNodeParent->RemoveFixup (child, head);
		}
//		node->Release();
//		delete node;
	}
}

void dgRedBackNode::Remove (dgRedBackNode ** const head) 
{
	Unlink (head);
	delete this;	
}

void dgRedBackNode::RemoveAllLow ()
{
	if (m_left) {
		m_left->RemoveAllLow();
	}
	if (m_right) {
		m_right->RemoveAllLow ();
	}
	SetInTreeFlag(false);
//	Kill();
//	Release();
	delete this;
}

void dgRedBackNode::RemoveAll ()
{
	dgRedBackNode* root = this;
	for (; root->m_parent; root = root->m_parent);
	root->RemoveAllLow();
}


