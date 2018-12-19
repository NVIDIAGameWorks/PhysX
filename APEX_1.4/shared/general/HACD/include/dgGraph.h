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
#ifndef __dgGraph__
#define __dgGraph__

#include "dgTypes.h"
#include "dgList.h"

template<class dgNodeData, class dgEdgeData> class dgGraphEdge;
template<class dgNodeData, class dgEdgeData> class dgGraphNode;


template<class dgNodeData, class dgEdgeData>
class dgGraph: public dgList<dgGraphNode<dgNodeData, dgEdgeData> >
{
	public:
	dgGraph (void);
	~dgGraph ();


	typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* AddNode ();
	void DeleteNode (typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node);

	void Trace () const;
};

template<class dgNodeData, class dgEdgeData> 
class dgGraphNode: public dgList<dgGraphEdge<dgNodeData, dgEdgeData> >
{
	public:
	dgGraphNode ();
	~dgGraphNode ();

	typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* AddEdge(typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node);
	void DeleteHalfEdge(typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* const edge);
	void DeleteEdge(typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* const edge);

	void Trace () const;

	dgNodeData m_nodeData;
};

template<class dgNodeData, class dgEdgeData> 
class dgGraphEdge
{
	public:
	dgGraphEdge();
	~dgGraphEdge();

	typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* m_node;
	dgEdgeData m_edgeData;
};



template<class dgNodeData, class dgEdgeData>
dgGraph<dgNodeData, dgEdgeData>::dgGraph () 
	:dgList<dgGraphNode<dgNodeData, dgEdgeData> >()
{
}


template<class dgNodeData, class dgEdgeData>
dgGraph<dgNodeData, dgEdgeData>::~dgGraph () 
{
}

template<class dgNodeData, class dgEdgeData>
typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* dgGraph<dgNodeData, dgEdgeData>::AddNode ()
{
	typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node = dgGraph<dgNodeData, dgEdgeData>::Append();
	return node;
}

template<class dgNodeData, class dgEdgeData>
void dgGraph<dgNodeData, dgEdgeData>::DeleteNode (typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node)
{
	for (typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* link = node->GetInfo().GetFirst(); link; link = link->GetNext()) {	
		typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const twinNode = link->GetInfo().m_node;
		for (typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* link1 = twinNode->GetInfo().GetFirst(); link1; link1 = link1->GetNext()) {	
			if (link1->GetInfo().m_node == node) {
				twinNode->GetInfo().Remove (link1);
				break;
			}
		}
	}
	dgList<dgGraphNode<dgNodeData, dgEdgeData> >::Remove (node);
}

template<class dgNodeData, class dgEdgeData>
void dgGraph<dgNodeData, dgEdgeData>::Trace () const
{
	for (typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* link = dgGraphNode<dgNodeData, dgEdgeData>::GetFirst(); link; link = link->GetNext()) {	
		link->GetInfo().Trace ();
//		dgTrace (("%d: ", link->GetInfo().m_index));
//		for (dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* edge = link->GetInfo().GetFirst(); edge; edge = edge->GetNext()) {	
//			dgListNode* node;
//			node = edge->GetInfo().m_node;
//			dgTrace (("%d ", node->GetInfo().m_index));
//		}
//		dgTrace (("\n"));
	}
//	dgTrace (("\n"));
}


template<class dgNodeData, class dgEdgeData> 
dgGraphNode<dgNodeData, dgEdgeData>::dgGraphNode() 
{

}


template<class dgNodeData, class dgEdgeData> 
dgGraphNode<dgNodeData, dgEdgeData>::~dgGraphNode() 
{

}

template<class dgNodeData, class dgEdgeData> 
typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* dgGraphNode<dgNodeData, dgEdgeData>::AddEdge (typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node)
{
	typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* edge = dgGraphNode<dgNodeData, dgEdgeData>::Append();

	edge->GetInfo().m_node = node;
	return edge;
}

template<class dgNodeData, class dgEdgeData> 
void dgGraphNode<dgNodeData, dgEdgeData>::DeleteHalfEdge(typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* const edge)
{
	dgList<dgGraphEdge<dgNodeData, dgEdgeData> >::Remove (edge);
}

template<class dgNodeData, class dgEdgeData> 
void dgGraphNode<dgNodeData, dgEdgeData>::DeleteEdge(typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* const edge)
{
	typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node = edge->GetInfo().m_node;
	
	for (typename dgGraphNode<dgNodeData, dgEdgeData>::dgListNode* twinEdge = node->GetInfo().GetFirst(); twinEdge; twinEdge = twinEdge->GetNext()) {	
		if (&twinEdge->GetInfo().m_node->GetInfo() == this) {
			node->GetInfo().DeleteHalfEdge(twinEdge);
			break;
		}
	}

	DeleteHalfEdge(edge);
}	

template<class dgNodeData, class dgEdgeData> 
void dgGraphNode<dgNodeData, dgEdgeData>::Trace () const
{
//	dgTrace (("%d: ", m_index));
//	for (typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* edge = typename dgGraph<dgNodeData, dgEdgeData>::GetFirst(); edge; edge = edge->GetNext()) {	
//		typename dgGraph<dgNodeData, dgEdgeData>::dgListNode* const node;
//		node = edge->GetInfo().m_node;
//		dgTrace (("%d ", node->GetInfo().m_index));
//	}
//	dgTrace (("\n"));
}


template<class dgNodeData, class dgEdgeData> 
dgGraphEdge<dgNodeData, dgEdgeData>::dgGraphEdge() 
{

}

template<class dgNodeData, class dgEdgeData> 
dgGraphEdge<dgNodeData, dgEdgeData>::~dgGraphEdge() 
{

}



#endif

