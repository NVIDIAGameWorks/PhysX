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

#ifndef SAMPLE_ASSET_H
#define SAMPLE_ASSET_H

#include "FrameworkFoundation.h"

namespace SampleFramework
{

	class SampleAsset
	{
		friend class SampleAssetManager;
	public:
		enum Type
		{
			ASSET_MATERIAL = 0,
			ASSET_TEXTURE,
			ASSET_INPUT,

			NUM_TYPES
		}_Type;

		virtual bool isOk(void) const = 0;

		Type        getType(void) const { return m_type; }
		const char *getPath(void) const { return m_path; }

	protected:
		SampleAsset(Type type, const char *path);
		virtual ~SampleAsset(void);

		virtual void release(void) { delete this; }

	private:
		SampleAsset &operator=(const SampleAsset&) { return *this; }

		const Type m_type;
		char      *m_path;
		PxU32      m_numUsers;
	};

} // namespace SampleFramework

#endif
