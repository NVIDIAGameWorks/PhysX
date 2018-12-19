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



#ifndef MI_FAST_XML_H
#define MI_FAST_XML_H

#include "MiPlatformConfig.h"
#include "MiFileBuf.h"		// defines the basic file stream interface.

namespace mimp
{

class FastXml
{
public:
	/***
	* Callbacks to the user with the contents of the XML file properly digested.
	*/
	class Callback
	{
	public:

		virtual bool processComment(const char *comment) = 0; // encountered a comment in the XML

		// 'element' is the name of the element that is being closed.
		// depth is the recursion depth of this element.
		// Return true to continue processing the XML file.
		// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
		// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
		virtual bool processClose(const char *element,MiU32 depth,bool &isError) = 0;	  // process the 'close' indicator for a previously encountered element

		// return true to continue processing the XML document, false to skip.
		virtual bool processElement(
			const char *elementName,   // name of the element
			MiI32 argc,         // number of attributes pairs
			const char **argv,         // list of attributes.
			const char  *elementData,  // element data, null if none
			MiI32 lineno) = 0;  // line number in the source XML file

		// process the XML declaration header
		virtual bool processXmlDeclaration(
			MiI32 /*argc*/,
			const char ** /*argv*/,
			const char  * /*elementData*/,
			MiI32 /*lineno*/)
		{
			return true;
		}

		virtual bool processDoctype(
			const char * /*rootElement*/, //Root element tag
			const char * /*type*/,        //SYSTEM or PUBLIC
			const char * /*fpi*/,         //Formal Public Identifier
			const char * /*uri*/)         //Path to schema file
		{
			return true;
		}

		virtual void *  fastxml_malloc(MiU32 size) = 0;
		virtual void	fastxml_free(void *mem) = 0;

	};

	virtual bool processXml(MiFileBuf &buff,bool streamFromMemory=false) = 0;

	virtual const char *getError(MiI32 &lineno) = 0; // report the reason for a parsing error, and the line number where it occurred.

	virtual void release(void) = 0;
};

const char *getAttribute(const char *attr, MiI32 argc, const char **argv);

FastXml * createFastXml(FastXml::Callback *iface);

}; // end of namespace mimp

#endif // FAST_XML_H
