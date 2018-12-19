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



#ifndef FORCE_FIELD_PREVIEW_H
#define FORCE_FIELD_PREVIEW_H

#include "Apex.h"
#include "AssetPreview.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class RenderDebugInterface;

namespace APEX_FORCEFIELD
{
/**
	\def FORCEFIELD_DRAW_NOTHING
	Draw no items.
*/
static const uint32_t FORCEFIELD_DRAW_NOTHING = (0x00);
/**
	\def FORCEFIELD_DRAW_ICON
	Draw the icon.
*/
static const uint32_t FORCEFIELD_DRAW_ICON = (0x01);
/**
	\def FORCEFIELD_DRAW_BOUNDARIES
	Draw the ForceField include shapes.
*/
static const uint32_t FORCEFIELD_DRAW_BOUNDARIES = (0x2);
/**
	\def FORCEFIELD_DRAW_WITH_CYLINDERS
	Draw the explosion field boundaries.
*/
static const uint32_t FORCEFIELD_DRAW_WITH_CYLINDERS = (0x4);
/**
	\def FORCEFIELD_DRAW_FULL_DETAIL
	Draw all of the preview rendering items.
*/
static const uint32_t FORCEFIELD_DRAW_FULL_DETAIL = (FORCEFIELD_DRAW_ICON + FORCEFIELD_DRAW_BOUNDARIES);
/**
	\def FORCEFIELD_DRAW_FULL_DETAIL_BOLD
	Draw all of the preview rendering items using cylinders instead of lines to make the text and icon look BOLD.
*/
static const uint32_t FORCEFIELD_DRAW_FULL_DETAIL_BOLD = (FORCEFIELD_DRAW_FULL_DETAIL + FORCEFIELD_DRAW_WITH_CYLINDERS);
}

/**
\brief APEX asset preview force field asset.
*/
class ForceFieldPreview : public AssetPreview
{
public:
	/**
	Set the scale of the icon.
	The unscaled icon has is 1.0x1.0 game units.
	By default the scale of the icon is 1.0. (unscaled)
	*/
	virtual void	setIconScale(float scale) = 0;
	/**
	Set the detail level of the preview rendering by selecting which features to enable.
	Any, all, or none of the following masks may be added together to select what should be drawn.
	The defines for the individual items are FORCEFIELD_DRAW_NOTHING, FORCEFIELD_DRAW_ICON, FORCEFIELD_DRAW_BOUNDARIES,
	FORCEFIELD_DRAW_WITH_CYLINDERS.
	All items may be drawn using the macro FORCEFIELD_DRAW_FULL_DETAIL and FORCEFIELD_DRAW_FULL_DETAIL_BOLD.
	*/
	virtual void	setDetailLevel(uint32_t detail) = 0;

protected:
	ForceFieldPreview() {};
};


PX_POP_PACK

} // namespace apex
} // namespace nvidia

#endif // FORCE_FIELD_PREVIEW_H
