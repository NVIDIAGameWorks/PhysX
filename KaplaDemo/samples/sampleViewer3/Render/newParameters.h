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

#pragma once
extern float seedDustRadius;
extern float seedDustNumPerSite;
extern float seedDustNumPerSiteCollisionEvent;
extern float applyForcePointAreaPerSample;
extern float applyForceFromPointsDrag;
extern float noiseKIsoDecay;
extern float noiseKAniDecay;
extern float noise3DScale;
extern float noise2DScale;
extern float dustMaxLife;
extern float dustMinLife;
extern float dustParticleOpacity;
extern float dustParticleRenderRadius;
extern float particleStartFade;
extern float octaveScaling;
extern float divStrength;
extern float curDivStrength;
extern float divStrengthReductionRate;


extern bool doGaussianBlur;
extern float blurVelSigmaFDX;
extern float blurVelCenterFactor;
extern float curlScale;
extern float areaPerDustSample;
extern float areaPerDebris;
extern float minSizeDecayRate;

extern float explodeVel; // Velocity of debris
extern float numParPerMeteor;

extern float minMeteorDustLife;
extern float maxMeteorDustLife;
extern float minMeteorDustSize;
extern float maxMeteorDustSize;

extern int maxNumDebrisPerType;
extern float sleepingThresholdRB;
extern float sleepingThresholdParticles;
extern int maxNumDebrisToAdd;
extern float minMeteorSizeDecayRate;