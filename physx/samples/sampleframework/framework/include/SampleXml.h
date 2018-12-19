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

#ifndef SAMPLE_XML_H
#define	SAMPLE_XML_H

#include "PsFastXml.h"
#include "PsString.h"
#include "SampleTree.h"

namespace FAST_XML
{

class xml_node : private SampleFramework::Tree::Node
{
public:
	xml_node(const char* name, const char* data, const physx::shdfnd::FastXml::AttributePairs& attr)
		: mName(name), mData(data)
	{
		PX_ASSERT( (attr.getNbAttr()*2+1) < MAX_ARGS);
		for(physx::PxI32 i=0; i<attr.getNbAttr(); i++)
		{
			mArgv[i*2] = attr.getKey(i);
			mArgv[i*2+1] = attr.getValue(i);
		}
		physx::shdfnd::FastXml::AttributePairs tmp(attr.getNbAttr()*2, mArgv);
		mAttr = tmp;
	}

	~xml_node()
	{
		xml_node* node = first_node();
		while (node != NULL)
		{
			xml_node* child = node;
			node = node->next_sibling();
			delete child;
		}
	}

public:
	xml_node* parent() const { return static_cast<xml_node*>(getParent()); }
	xml_node* first_node() const { return static_cast<xml_node*>(getFirstChild()); }
	xml_node* next_sibling() const { return static_cast<xml_node*>(getNextSibling()); }
	bool append_node(xml_node& node) { return appendChild(node); }

	xml_node* first_node(const char* name) const
	{
		for (xml_node* node = first_node(); node != NULL; node = node->next_sibling())
		{
			if (0 == strcmp(node->name(), name))
			{
				return node;
			}
		}

		return NULL;
	}

public:
	const char* name() const { return mName; }
	const char* value() const { return mData; }
	const char *getXMLAttribute(const char *attr) const
	{
		return mAttr.get(attr);
	}

private:
	static const physx::PxI32 MAX_ARGS = 50;

	const char*		mName;
	const char*		mData;	
	const char*		mArgv[MAX_ARGS];
	physx::shdfnd::FastXml::AttributePairs mAttr;
};

class XmlBuilder : public physx::shdfnd::FastXml::Callback
{
public:
	XmlBuilder() : mRoot(NULL), mThis(NULL) {}
	virtual ~XmlBuilder()
	{
		delete mRoot;
	}

public:
	xml_node* rootnode() const { return mRoot; }

	virtual bool processComment(const char *comment)
	{
		return true;
	}

	virtual bool processClose(const char *element, physx::PxU32 depth, bool &isError)
	{
		mThis = mThis->parent();
		return true;
	}

	virtual bool processElement(
			const char *elementName,   // name of the element
			const char  *elementData,  // element data, null if none
			const physx::shdfnd::FastXml::AttributePairs& attr,      // attributes
			physx::PxI32 lineno)
	{
		xml_node* node = new xml_node(elementName, elementData, attr);

		if (NULL != mThis)
			mThis->append_node(*node);
		else if (NULL == mRoot)
			mRoot = node;
		else
		{
			physx::shdfnd::printFormatted("error: more than 1 root node in xml file\n");
			return false;
		}

		mThis = node;
		return true;
	}

	virtual void*	allocate(physx::PxU32 size)
	{
		return malloc(size);
	}

	virtual void	deallocate(void *mem)
	{
		free(mem);
	}

private:
	xml_node*	mRoot;
	xml_node*	mThis;
};

}

#endif
