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


#include <PsString.h>
#include "PxAssert.h"

#include <DirEntry.h>

#include "Find.h"

namespace nvidia {
namespace apex {

void Find(const char* root, FileHandler& f, const char** ignoredFiles)
{
	if (!root)
		return;

	// Fix slashes
	char goodRoot[128];
	strcpy(goodRoot, root);
#if PX_WINDOWS_FAMILY
	for (char* p = goodRoot; *p; ++p)
	{
		if ('/' == *p)
			*p = '\\';
	}
#endif

	physx::DirEntry dentry;
	if (!physx::DirEntry::GetFirstEntry(goodRoot, dentry))
	{
		PX_ALWAYS_ASSERT();
		return;
	}

	for (; !dentry.isDone(); dentry.next())
	{
		const char* filename = dentry.getName();

		if (!filename || 0 == strcmp(".", filename) || 0 == strcmp("..", filename))
			continue;

		bool doSkip = false;
		for (size_t i = 0; ignoredFiles && ignoredFiles[i]; ++i)
		{
			if (0 == strcmp(filename, ignoredFiles[i]))
			{
				doSkip = true;
				break;
			}
		}

		if (doSkip)
			continue;

		char tmp[128];
		physx::shdfnd::snprintf(tmp, sizeof(tmp), "%s/%s", goodRoot, filename);

#if PX_WINDOWS_FAMILY
	for (char* p = tmp; *p; ++p)
	{
		if ('/' == *p)
			*p = '\\';
	}
#endif

		if (dentry.isDirectory())
		{
			Find(tmp, f, ignoredFiles);
			continue;
		}

		f.handle(tmp);
	}
}

}}
