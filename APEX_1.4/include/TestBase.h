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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#ifndef TEST_BASE_H
#define TEST_BASE_H

#include "ApexUsingNamespace.h"

/*!
\file
\brief An TestBase is an interface for managing unit tests
*/

namespace nvidia
{

namespace apex
{

class Actor;

/**
	Interface for user's unit tests
*/
class TestFunctionInterface
{
public:

	/**
		Constructor for TestFunctionInterface
	*/
	TestFunctionInterface(const char* name) : mName(name) {}
	
	/**
		Run CPU tests, returns true if all tests passed
	*/
	virtual bool run(void*, void*) = 0;
	
	/**
		Check state, true if user's test object in correct state
	*/
	virtual bool check(void*, void*) = 0;

	/**
		Run GPU tests, returns true if all tests passed
	*/
	virtual bool runGpu(void*, void*, void*) = 0;

	/**
		User's test name
	*/
	const char* mName;
};

/**
	Defines the UnitTestsActor API which is instantiated from an UnitTestsAsset
*/
class TestBase
{
public:

	/**
	\brief Returns the name of the test at this index.

	\param [in] unitTestsIndex : The test number to refer to; must be less than the result of getUnitTestCount
	*/
	virtual const char* getUnitTestsName(uint32_t unitTestsIndex) const = 0;

	/**
	\brief Returns the index of the test for this name.

	\param [in] unitTestsName : The test name to refer to; if there is no such name method returns -1
	*/
	virtual uint32_t getUnitTestsIndex(const char* unitTestsName) const = 0;

	/**
	\brief run unit test

	\param [in] unitTestsID : The unit test number to refer to; must be less than the result of getUnitTestsCount
	\param [in] dataPtr : The pointer to data which is needed for unit test.
	*/
	virtual bool runUnitTests(uint32_t unitTestsID, void* dataPtr) const = 0;

	/**
	\brief check unit test
	*/
	virtual bool checkUnitTests(uint32_t unitTestsID, void* dataPtr) const = 0;

	/**
	\brief returns the number of unit tests
	*/
	virtual uint32_t getUnitTestsCount() const = 0;

	/**
	\brief set level of runtime check
	\param [in] level - level of runtime check

	 level = 0 - no runtime checks
	 level = 1 - easy-weight checks. Perfomance should not drop more than 10-20%
	 level = 2 - medium-weight checks. Perfomance drop less than 300-500%
	 level = 3 - hard-weight checks. Perfomance drop isn't limited
	*/
	virtual void setRuntimeCheckLevel(uint32_t level) = 0;
};


}; // end of apex namespace
}; // end of nvidia namespace

#endif
