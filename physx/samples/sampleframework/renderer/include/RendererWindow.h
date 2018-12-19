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


#ifndef RENDERER_WINDOW_H
#define RENDERER_WINDOW_H

#include <RendererConfig.h>

namespace SampleFramework {
	class SamplePlatform;
}

namespace SampleRenderer
{

	class RendererWindow
	{
	public:
		RendererWindow(void);
		virtual ~RendererWindow(void);

		bool open(PxU32 width, PxU32 height, const char *title, bool fullscreen=false);
		void close(void);

		bool isOpen(void) const;

		// update the window's state... handle messages, etc.
		void update(void);

		// resize the window...
		void setSize(PxU32 width, PxU32 height);

		// get/set the window's title...
		void getTitle(char *title, PxU32 maxLength) const;
		void setTitle(const char *title);

		bool	hasFocus() const;
		void	setFocus(bool b);

		SampleFramework::SamplePlatform* getPlatform() { return m_platform; }
	public:
		// called just AFTER the window opens.
		virtual void onOpen(void) {}

		// called just BEFORE the window closes. return 'true' to confirm the window closure.
		virtual bool onClose(void) { return true; }

		// called just AFTER the window is resized.
		virtual void onResize(PxU32 /*width*/, PxU32 /*height*/) {}

		// called when the window's contents needs to be redrawn.
		virtual void onDraw(void) = 0;

	protected:
		SampleFramework::SamplePlatform*				m_platform;

	private:
		bool											m_isOpen;
	};

} // namespace SampleRenderer

#endif
