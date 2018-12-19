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


#ifndef BASIC_FSASSET_H
#define BASIC_FSASSET_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

#define JET_FS_AUTHORING_TYPE_NAME	"JetFSAsset"
#define ATTRACTOR_FS_AUTHORING_TYPE_NAME	"AttractorFSAsset"
#define VORTEX_FS_AUTHORING_TYPE_NAME	"VortexFSAsset"
#define NOISE_FS_AUTHORING_TYPE_NAME "NoiseFSAsset"
#define WIND_FS_AUTHORING_TYPE_NAME	"WindFSAsset"

/**
 \brief BasicFS Asset class
 */
class BasicFSAsset : public Asset
{
protected:
	virtual ~BasicFSAsset() {}

public:
};

/**
 \brief BasicFS Asset Authoring class
 */
class BasicFSAssetAuthoring : public AssetAuthoring
{
protected:
	virtual ~BasicFSAssetAuthoring() {}

public:
};


PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // BASIC_FSASSET_H
