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

extern const char *particleVS,*particleVSNoKill;
extern const char *particleSpherePS, *particleDebugPS, *particleDensityPS, *particleFoamPS, *particleFoamVS, *particleBubblePS;
extern const char *particleSurfacePS, *particleThicknessPS;
extern const char *mblurVS, *mblurGS, *mblurGSNoKill, *particleSprayPS;
extern const char *passThruVS;
extern const char *depthBlurPS , *depthBlur2DPS, *depthBlurSymPS, *displaySurfacePS, *displaySurfaceNewPS, *displaySurfaceChromePS, *displaySurfaceSolidPS;
extern const char *depthBlurViewIDPS, *depthBlurViewIDNonSepPS;
extern const char *textureRectPS, *dilatePS, *copyPS;

// Nutt
extern const char *depthToInitThicknessPS;
extern const char *hfDepthPS, *hfDepthVS, *hfThicknessVS, *hfThicknessPS;
extern const char *debugPS, *debugTriVS, *debugTriPS;
extern const char *hfThicknessAddPS, *displaySurfaceNutPS, *particleDumbFoamVS, *particleDumbFoamGS, *particleDumbFoamPS, *displaySurfaceNutEscapePS, *displaySurfaceNutEscapeDiffusePS, *particleSprayUseFOMPS, *particleSprayGenFOMPS ;

extern const char *texture2DPS;

extern const char *particleSmokeUseFOMPS;

extern const char *skyboxVS;
extern const char *skyboxPS;