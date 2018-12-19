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
//--------------------------------------------------------------------------------------
//
// Implementation of different antialiasing methods.
// - typical MSAA
// - CSAA
// - Hardware AA mixed with FBO for supersampling pass
//   - simple downsampling
//   - downsampling with 1 or 2 kernel filters
//
// AABox is the class that will handle everything related to supersampling through 
// an offscreen surface defined thanks to FBO
// Basic use is :
//
//  Initialize()
//  ...
//  Activate(int x=0, int y=0)
//    Draw the scene (so, in the offscreen supersampled buffer)
//  Deactivate()
//  Draw() : downsample to backbuffer
//  ...
//  Destroy()
//
//--------------------------------------------------------------------------------------
#define FB_SS 0
#include <Cg/CgGL.h>
#include <string>
class AABox
{
public:
  AABox(std::string path); 
  ~AABox();

  bool Initialize(int w, int h, float ssfact, int depthSamples, int coverageSamples);
  void Destroy();

  void Activate(int x=0, int y=0);
  void Deactivate();
  void Rebind();
  void Draw(int technique);
  int getBufW() const {return bufw;};
  int getBufH() const {return bufh;};
  GLuint getTextureID() {return textureID;};

  CGparameter   cgBlendFactor;
//protected:
  bool          bValid;
  bool          bCSAA;

  int           vpx, vpy, vpw, vph;
  int           posx, posy;
  int           bufw, bufh;

  CGcontext     cgContext;
  CGeffect      cgEffect;
  CGtechnique   cgTechnique[4];
  CGpass        cgPassDownSample;
  CGpass        cgPassDrawFinal;
  GLuint        textureID;
  GLuint        textureDepthID;
  CGparameter   cgSrcSampler;
  CGparameter   cgSSsampler;
  CGparameter   cgDepthSSsampler;
  CGparameter	cgTexelSize;
  GLuint        fb;
  GLuint        fbms;
  GLuint        depth_rb;
  GLuint        color_rb;
  GLuint		oldFbo;
  std::string path;

  bool          initRT(int depthSamples, int coverageSamples);
};