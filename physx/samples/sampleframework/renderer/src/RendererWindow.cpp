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
