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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include <Windows.h>
#include <stdio.h>

#pragma warning (disable:4371)
#pragma warning (disable:4946)

using namespace Platform;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

ref class AppActivation : IFrameworkView
{
public:
	virtual void Initialize( _In_ CoreApplicationView^ applicationView )
	{	
		applicationView->Activated += ref new TypedEventHandler< CoreApplicationView^, IActivatedEventArgs^ >( this, &AppActivation::OnActivated );
	}
	virtual void SetWindow( _In_ CoreWindow^ window )	{}
	virtual void Load( _In_ String^ entryPoint )		{}
	virtual void Run()									{}
	virtual void Uninitialize()							{}

private:
	void OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
	{
		CoreWindow::GetForCurrentThread()->Activate();
	}
};

ref class SnippetViewSource : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView()
	{
		return ref new AppActivation();
	}
};

void initPlatform()
{
	auto mySource = ref new SnippetViewSource();
	CoreApplication::Run(mySource);
}

extern int snippetMain(int, const char*const*);

void OutputDebugPrint(const char* format, ...)
{
	char buf[1024];

	va_list arg;
	va_start( arg, format );
	vsprintf_s(buf, sizeof buf, format, arg);
	va_end(arg);

	OutputDebugStringA(buf);
}

int main(Platform::Array<Platform::String^>^)
{
	initPlatform();
	snippetMain(0,NULL);
}
