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



#ifndef DIR_ENTRY_H
#define DIR_ENTRY_H

#include "foundation/PxPreprocessor.h"

#if defined PX_LINUX || defined PX_ANDROID
#	include <dirent.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <string>
#	include <sstream>
#else
#	error Unsupported platform
#endif

namespace physx
{
	class DirEntry
	{
	public:

		DirEntry()
		{
			mDir = NULL;
			mEntry = NULL;
			mIdx = 0;
			mCount = 0;
		}

		~DirEntry()
		{
			if (!isDone())
			{
				while (next());
			}

			// The Find.cpp loop behaves badly and doesn't cleanup the DIR pointer
			if (mDir)
			{
				closedir(mDir);
				mDir = NULL;
			}
		}

		// Get successive element of directory.
		// Returns true on success, error otherwise.
		bool next()
		{
			if (mIdx < mCount)
			{
				mEntry = readdir(mDir);
				++mIdx;
			}
			else
			{
				bool ret = (0 == closedir(mDir));
				mDir = NULL;
				mEntry = NULL;
				return ret;
			}
			return true;
		}

		// No more entries in directory?
		bool isDone() const
		{
			return mIdx >= mCount;

		}

		// Is this entry a directory?
		bool isDirectory() const
		{
			if (mEntry)
			{
				if(DT_UNKNOWN == mEntry->d_type)
				{
					// on some fs d_type is DT_UNKNOWN, so we need to use stat instead
					std::ostringstream path;
					path << mDirPath << "/" << mEntry->d_name;
					struct stat s;
					if(stat(path.str().c_str(), &s) == 0)
					{
						return S_ISDIR(s.st_mode);
					}
					else 
					{
						return false;
					}
				}
				else 
				{
					return DT_DIR == mEntry->d_type;
				}
			}
			else
			{
				return false;
			}
		}

		// Get name of this entry.
		const char* getName() const
		{
			if (mEntry)
			{
				return mEntry->d_name;
			}
			else
			{
				return NULL;
			}
		}

		// Get first entry in directory.
		static bool GetFirstEntry(const char* path, DirEntry& dentry)
		{
			dentry.mDir = opendir(path);
			dentry.mDirPath.assign(path, strlen(path));
			if (!dentry.mDir)
			{
				return false;
			}

			dentry.mIdx = 0;

			// count files
			dentry.mCount = 0;
			if (dentry.mDir != NULL)
			{
				while (readdir(dentry.mDir))
				{
					dentry.mCount++;
				}
			}
			closedir(dentry.mDir);
			dentry.mDir = opendir(path);
			dentry.mEntry = readdir(dentry.mDir);
			return true;
		}

	private:

		DIR* mDir;
		std::string mDirPath;
		struct dirent* mEntry;
		long mIdx, mCount;
	};
}

#endif
