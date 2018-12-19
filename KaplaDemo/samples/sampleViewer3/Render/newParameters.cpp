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

float seedDustRadius = 0.02f;
//float seedDustNumPerSite = 15;
//float seedDustNumPerSiteCollisionEvent = 3;
float seedDustNumPerSite = 20;
float seedDustNumPerSiteCollisionEvent = 4.5f;

float applyForcePointAreaPerSample = 0.005f;
float applyForceFromPointsDrag = 0.1f;
//float noise3DScale = 1.5f;
//float noise2DScale = 0.5f;
float noiseKIsoDecay = 0.99f;
float noiseKAniDecay = 0.99f;
float noise3DScale = 0.75f*0.5f;
float noise2DScale = 0.32f*0.5f;
float dustMaxLife = 5.0f;
float dustMinLife = 1.0f;
float dustParticleOpacity = 0.1f;
float dustParticleRenderRadius = 0.1f;
float particleStartFade = 5.0f;
float octaveScaling = 7.0f;
float divStrength = 5.0f;
float curDivStrength = 0.0f;
float divStrengthReductionRate = 8.0f/30;

bool doGaussianBlur = false;
float blurVelSigmaFDX = 1.0f;
float blurVelCenterFactor = 30.0f;
float curlScale = 10.0f;

float areaPerDustSample = 0.005f;
float areaPerDebris = 0.02f;

float minSizeDecayRate = 1.0f;
float explodeVel = 3.0f;
float numParPerMeteor = 3;

float minMeteorDustLife = 2.0f;
float maxMeteorDustLife = 2.3f;
float minMeteorDustSize = 0.5f;
float maxMeteorDustSize = 1.5f;
float minMeteorSizeDecayRate = 1.0f;

int maxNumDebrisPerType = 500; // Maximum number of debris per each type of debris
float sleepingThresholdRB = 0.1f;
float sleepingThresholdParticles = 0.1f;
int maxNumDebrisToAdd = 100;