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
#include <RendererWindow.h>
#include <RendererMemoryMacros.h>
#include <SamplePlatform.h>
#include <stdio.h>

using namespace SampleRenderer;

RendererWindow::RendererWindow(void) : m_platform(NULL), m_isOpen(false)
{
	m_platform = SampleFramework::createPlatform(this);
}

bool RendererWindow::hasFocus() const
{
	return m_platform->hasFocus();
}

void RendererWindow::setFocus(bool b) 
{
	m_platform->setFocus(b);
}

RendererWindow::~RendererWindow(void)
{
	DELETESINGLE(m_platform);
}

bool RendererWindow::open(PxU32 width, PxU32 height, const char *title, bool fullscreen)
{
	bool ok         = false;
	RENDERER_ASSERT(width && height, "Attempting to open a window with invalid width and/or height.");
	if(width && height)
	{
		ok = m_platform->openWindow(width, height, title, fullscreen);
#if !defined(RENDERER_WINDOWS)
		m_isOpen = true;
#endif
	}
	return ok;
}

void RendererWindow::close(void)
{
	m_platform->closeWindow();
#if !defined(RENDERER_WINDOWS)
	if(isOpen())
	{
		m_isOpen = false;
		onClose();
	}
#endif
}

bool RendererWindow::isOpen(void) const
{
	bool open = m_platform->isOpen();
#if !defined(RENDERER_WINDOWS)
	open = m_isOpen;
#endif
	return open;
}

// update the window's state... handle messages, etc.
void RendererWindow::update(void)
{
	m_platform->update();
}

void RendererWindow::setSize(PxU32 width, PxU32 height)
{
	m_platform->setWindowSize(width, height);
}

// get the window's title...
void RendererWindow::getTitle(char *title, PxU32 maxLength) const
{
	m_platform->getTitle(title, maxLength);
}

// set the window's title...
void RendererWindow::setTitle(const char *title)
{
	m_platform->setTitle(title);
}
