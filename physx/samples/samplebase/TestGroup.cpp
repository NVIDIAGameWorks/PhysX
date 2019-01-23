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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.

#include "TestGroup.h"

#include <stdio.h>
#include <string.h>

Test::TestGroup::TestGroup(const char* name, size_t count)
	: mTest(NULL)
	, mParent(NULL)
{
	if (MAX_COUNT == count)
		count = strlen(name);
	mName = new char[count + 1];
	strncpy(mName, name, count);
	mName[count] = '\0';
}

Test::TestGroup::TestGroup(TestGroup* group)
	: mTest(group->mTest)
	, mParent(NULL)
{
	mName = new char[strlen(group->getName()) + 1];
	strcpy(mName, group->getName());
}

Test::TestGroup::TestGroup(SampleCreator test, const char* name, size_t count)
	: mTest(test)
	, mParent(NULL)
{
	PX_ASSERT(test);

	if (MAX_COUNT == count)
		count = strlen(name);
	mName = new char[count + 1];
	strncpy(mName, name, count);
	mName[count] = '\0';
}

Test::TestGroup::~TestGroup()
{
	if (!isTest())
	{
		for (PxU32 i = 0; i < mChildren.size(); i++)
			delete mChildren[i];

		delete[] mName;
	}
}

void Test::TestGroup::getPath(SampleArray<TestGroup*>& path)
{
	if (getParent())
		getParent()->getPath(path);

	path.pushBack(this);
}

void Test::TestGroup::getPathName(char* strBuffer, unsigned strBufferMaxLength, bool omitRoot)
{
	SampleArray<TestGroup*> path;
	getPath(path);

	unsigned charCount = 1; //\0
	for (unsigned i = omitRoot ? 1 : 0; i < path.size(); ++i)
	{
		const TestGroup& node = *path[i];

		unsigned nodeNameLength = unsigned(strlen(node.getName()));
		if (node.getFirstChild())
			nodeNameLength += 1;

		if (charCount + nodeNameLength < strBufferMaxLength)
		{
			sprintf(strBuffer + (charCount - 1), "%s%s", node.getName(), node.getFirstChild() ? "/" : "");
			charCount += nodeNameLength;
		}
	}
}

Test::TestGroup* Test::TestGroup::getGroupFromPathName(const char* pathName, bool omitRoot)
{
	if (!omitRoot || getParent())
	{
		if (strstr(pathName, getName()) != pathName)
			return NULL;

		pathName += strlen(getName());

		if (getFirstChild())
		{
			if (strstr(pathName, "/") != pathName)
				return NULL;

			pathName += strlen("/");
		}
		else
		{
			if (pathName[0] == '\0')
				return this;
			else
				return NULL;
		}
	}

	for (unsigned i = 0; i < mChildren.size(); ++i)
	{
		TestGroup* group = mChildren[i]->getGroupFromPathName(pathName, omitRoot);
		if (group)
			return group;
	}
	return NULL;
}

void Test::TestGroup::addTest(SampleCreator test, const char* name, size_t count)
{
	PX_ASSERT(!isTest() && test);
	TestGroup* testGroup = new TestGroup(test, name, count);
	addGroup(testGroup);
}

void Test::TestGroup::addGroup(TestGroup* group)
{
	PX_ASSERT(group && !group->getParent());
	mChildren.pushBack(group);
	group->mParent = this;
}

Test::TestGroup* Test::TestGroup::deepCopy()
{
	TestGroup* groupCopy = new TestGroup(this);

	for (unsigned i = 0; i < mChildren.size(); i++)
	{
		TestGroup* childCopy = mChildren[i]->deepCopy();
		groupCopy->addGroup(childCopy);
	}

	return groupCopy;
}

Test::TestGroup* Test::TestGroup::addPath(SampleArray<TestGroup*>& path)
{
	if (path.size() == 0)
		return NULL;

	TestGroup* current = this;
	for (unsigned i = 0; i < path.size(); i++)
	{
		TestGroup* pathGroup = path[i];
		TestGroup* child = current->getChildByName(pathGroup->getName());
		if (!child)
		{
			child = new TestGroup(pathGroup);
			current->addGroup(child);
		}
		current = child;
	}
	
	return current;
}

Test::TestGroup* Test::TestGroup::getNextChild(TestGroup& current)
{
	int nextIndex = getChildIndex(current) + 1;
	if (nextIndex >= int(mChildren.size()))
		return NULL;

	return mChildren[nextIndex];
}

Test::TestGroup* Test::TestGroup::getPreviousChild(TestGroup& current)
{
	int prevIndex = getChildIndex(current) - 1;
	if (prevIndex < 0)
		return NULL;

	return mChildren[prevIndex];
}

Test::TestGroup* Test::TestGroup::getChildByName(const char* name, size_t count)
{
	if (MAX_COUNT == count)
		count = strlen(name) + 1;
	for (unsigned i = 0; i < mChildren.size(); i++)
	{
		if (strncmp(mChildren[i]->getName(), name, count) == 0)
			return mChildren[i];
	}
	return NULL;
}

Test::TestGroup* Test::TestGroup::getFirstTest()
{
	TestGroup* current = getFirstLeaf();
	if (!current || current->isTest())
		return current;

	return getNextTest(current);
}

Test::TestGroup* Test::TestGroup::getLastTest()
{
	TestGroup* current = getLastLeaf();
	if (!current || current->isTest())
		return current;

	return getPreviousTest(current);
}

Test::TestGroup* Test::TestGroup::getNextTest(TestGroup* current)
{
	current = getNextLeaf(current);
	while (current && !current->isTest())
		current = getNextLeaf(current);

	return current;
}

Test::TestGroup* Test::TestGroup::getPreviousTest(TestGroup* current)
{
	current = getPreviousLeaf(current);
	while (current && !current->isTest())
		current = getPreviousLeaf(current);

	return current;
}

unsigned Test::TestGroup::getChildIndex(TestGroup& child)
{
	PX_ASSERT(!isTest());
	TestGroup** p = mChildren.find(&child);
	PX_ASSERT(p != mChildren.end());
	return unsigned(p - mChildren.begin());
}

Test::TestGroup* Test::TestGroup::getFirstLeaf()
{
	TestGroup* firstChild = getFirstChild();
	if (!firstChild)
		return this;

	return firstChild->getFirstLeaf();
}

Test::TestGroup* Test::TestGroup::getLastLeaf()
{
	TestGroup* lastChild = getLastChild();
	if (!lastChild)
		return this;

	return lastChild->getLastLeaf();
}

Test::TestGroup* Test::TestGroup::getNextLeaf(TestGroup* current)
{
	PX_ASSERT(current);

	if (current == this)
		return NULL;

	TestGroup* parent = current->getParent();
	if (!parent)
		return NULL;

	TestGroup* nextSibling = parent->getNextChild(*current);
	if (nextSibling)
		return nextSibling->getFirstLeaf();
	else
		return getNextLeaf(parent);
}

Test::TestGroup* Test::TestGroup::getPreviousLeaf(TestGroup* current)
{
	PX_ASSERT(current);

	if (current == this)
		return NULL;

	TestGroup* parent = current->getParent();
	if (!parent)
		return NULL;

	TestGroup* prevSibling = parent->getPreviousChild(*current); 
	if (prevSibling)
		return prevSibling->getLastLeaf();
	else
		return getPreviousLeaf(parent);
}

