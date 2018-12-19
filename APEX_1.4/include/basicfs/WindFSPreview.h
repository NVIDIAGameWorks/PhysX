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


#ifndef WIND_FSPREVIEW_H
#define WIND_FSPREVIEW_H

#include "Apex.h"
#include "AssetPreview.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class RenderDebugInterface;

/**
	\brief The APEX_WIND namespace contains the defines for setting the preview detail levels.

	WIND_DRAW_NOTHING - draw nothing<BR>
	WIND_DRAW_ASSET_INFO - draw the asset info in the screen space<BR>
	WIND_DRAW_FULL_DETAIL - draw all components of the preview<BR>
*/
namespace APEX_WIND
{
/**
	\def WIND_DRAW_NOTHING
	Draw no preview items.
*/
static const uint32_t WIND_DRAW_NOTHING = 0x00;
/**
	\def WIND_DRAW_ASSET_INFO
	Draw the Asset Info in Screen Space.  This displays the various asset members that are relevant to the current asset.
	parameters that are not relevant because they are disabled are not displayed.
*/
static const uint32_t WIND_DRAW_ASSET_INFO = 0x02;
/**
	\def WIND_DRAW_FULL_DETAIL
	Draw all of the preview rendering items.
*/
static const uint32_t WIND_DRAW_FULL_DETAIL = (WIND_DRAW_ASSET_INFO);
}

/**
\brief This class provides the asset preview for APEX WindFS Assets.  The class provides multiple levels of prevew
detail that can be selected individually.
*/
class WindFSPreview : public AssetPreview
{
public:
	/**
	Set the detail level of the preview rendering by selecting which features to enable.<BR>
	Any, all, or none of the following masks may be added together to select what should be drawn.<BR>
	The defines for the individual items are:<br>
		WIND_DRAW_NOTHING - draw nothing<BR>
		WIND_DRAW_ASSET_INFO - draw the asset info in the screen space<BR>
		WIND_DRAW_FULL_DETAIL - draw all components of the asset preview<BR>
	All items may be drawn using the macro DRAW_FULL_DETAIL.
	*/
	virtual void	setDetailLevel(uint32_t detail) = 0;
};


PX_POP_PACK

}
} // namespace nvidia

#endif // WIND_FSPREVIEW_H
