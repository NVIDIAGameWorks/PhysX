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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
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
// Copyright (c) 2008-2021 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "geometry/PxGeometry.h"
#include "PxcContactMethodImpl.h"

namespace physx
{
static bool PxcInvalidContactPair	(CONTACT_METHOD_ARGS_UNUSED)		{ return false;	}

// PT: IMPORTANT: do NOT remove the indirection! Using the Gu functions directly in the table produces massive perf problems.
static bool PxcContactSphereSphere		(GU_CONTACT_METHOD_ARGS)		{ return contactSphereSphere(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactSphereCapsule		(GU_CONTACT_METHOD_ARGS)		{ return contactSphereCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactSphereBox			(GU_CONTACT_METHOD_ARGS)		{ return contactSphereBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactSpherePlane		(GU_CONTACT_METHOD_ARGS)		{ return contactSpherePlane(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactSphereConvex		(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactSphereMesh		(GU_CONTACT_METHOD_ARGS)		{ return contactSphereMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactSphereHeightField	(GU_CONTACT_METHOD_ARGS)		{ return contactSphereHeightfield(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcContactPlaneBox			(GU_CONTACT_METHOD_ARGS)		{ return contactPlaneBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactPlaneCapsule		(GU_CONTACT_METHOD_ARGS)		{ return contactPlaneCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactPlaneConvex		(GU_CONTACT_METHOD_ARGS)		{ return contactPlaneConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactCapsuleCapsule	(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactCapsuleBox		(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactCapsuleConvex		(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactCapsuleMesh		(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactCapsuleHeightField(GU_CONTACT_METHOD_ARGS)		{ return contactCapsuleHeightfield(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcContactBoxBox			(GU_CONTACT_METHOD_ARGS)		{ return contactBoxBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);					}
static bool PxcContactBoxConvex			(GU_CONTACT_METHOD_ARGS)		{ return contactBoxConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactBoxMesh			(GU_CONTACT_METHOD_ARGS)		{ return contactBoxMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactBoxHeightField	(GU_CONTACT_METHOD_ARGS)		{ return contactBoxHeightfield(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactConvexConvex		(GU_CONTACT_METHOD_ARGS)		{ return contactConvexConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcContactConvexMesh		(GU_CONTACT_METHOD_ARGS)		{ return contactConvexMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcContactConvexHeightField	(GU_CONTACT_METHOD_ARGS)		{ return contactConvexHeightfield(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}

static bool PxcPCMContactSphereSphere		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereSphere(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactSpherePlane		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSpherePlane(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactSphereBox			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactSphereCapsule		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactSphereConvex		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactSphereMesh			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactSphereHeightField	(GU_CONTACT_METHOD_ARGS)	{ return pcmContactSphereHeightField(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);	}
static bool PxcPCMContactPlaneCapsule		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactPlaneCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactPlaneBox			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactPlaneBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactPlaneConvex		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactPlaneConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactCapsuleCapsule		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactCapsuleCapsule(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactCapsuleBox			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactCapsuleBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactCapsuleConvex		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactCapsuleConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactCapsuleMesh		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactCapsuleMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactCapsuleHeightField	(GU_CONTACT_METHOD_ARGS)	{ return pcmContactCapsuleHeightField(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);	}
static bool PxcPCMContactBoxBox				(GU_CONTACT_METHOD_ARGS)	{ return pcmContactBoxBox(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcPCMContactBoxConvex			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactBoxConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactBoxMesh			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactBoxMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);				}
static bool PxcPCMContactBoxHeightField		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactBoxHeightField(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactConvexConvex		(GU_CONTACT_METHOD_ARGS)	{ return pcmContactConvexConvex(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);		}
static bool PxcPCMContactConvexMesh			(GU_CONTACT_METHOD_ARGS)	{ return pcmContactConvexMesh(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);			}
static bool PxcPCMContactConvexHeightField	(GU_CONTACT_METHOD_ARGS)	{ return pcmContactConvexHeightField(shape0, shape1, transform0, transform1, params, cache, contactBuffer, renderOutput);	}

#define DYNAMIC_CONTACT_REGISTRATION(x) PxcInvalidContactPair
//#define DYNAMIC_CONTACT_REGISTRATION(x) x

//Table of contact methods for different shape-type combinations
PxcContactMethod g_ContactMethodTable[][PxGeometryType::eGEOMETRY_COUNT] = 
{
	//PxGeometryType::eSPHERE
	{
		PxcContactSphereSphere,			//PxGeometryType::eSPHERE
		PxcContactSpherePlane,			//PxGeometryType::ePLANE
		PxcContactSphereCapsule,		//PxGeometryType::eCAPSULE
		PxcContactSphereBox,			//PxGeometryType::eBOX
		PxcContactSphereConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactSphereMesh,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactSphereHeightField),	//PxGeometryType::eHEIGHTFIELD	//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::ePLANE
	{
		0,								//PxGeometryType::eSPHERE
		PxcInvalidContactPair,			//PxGeometryType::ePLANE
		PxcContactPlaneCapsule,			//PxGeometryType::eCAPSULE
		PxcContactPlaneBox,				//PxGeometryType::eBOX
		PxcContactPlaneConvex,			//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,			//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eCAPSULE
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		PxcContactCapsuleCapsule,		//PxGeometryType::eCAPSULE
		PxcContactCapsuleBox,			//PxGeometryType::eBOX
		PxcContactCapsuleConvex,		//PxGeometryType::eCONVEXMESH
		PxcContactCapsuleMesh,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactCapsuleHeightField),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eBOX
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		PxcContactBoxBox,				//PxGeometryType::eBOX
		PxcContactBoxConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactBoxMesh,				//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactBoxHeightField),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eCONVEXMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		PxcContactConvexConvex,			//PxGeometryType::eCONVEXMESH
		PxcContactConvexMesh,			//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcContactConvexHeightField),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,			//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},

	//PxGeometryType::eHEIGHTFIELD
	{
		0,								//PxGeometryType::eSPHERE
		0,								//PxGeometryType::ePLANE
		0,								//PxGeometryType::eCAPSULE
		0,								//PxGeometryType::eBOX
		0,								//PxGeometryType::eCONVEXMESH
		0,								//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,			//PxGeometryType::eHEIGHTFIELD
	},
};


//#if	PERSISTENT_CONTACT_MANIFOLD
//Table of contact methods for different shape-type combinations
PxcContactMethod g_PCMContactMethodTable[][PxGeometryType::eGEOMETRY_COUNT] = 
{
	//PxGeometryType::eSPHERE
	{
		PxcPCMContactSphereSphere,										//PxGeometryType::eSPHERE
		PxcPCMContactSpherePlane,										//PxGeometryType::ePLANE
		PxcPCMContactSphereCapsule,										//PxGeometryType::eCAPSULE
		PxcPCMContactSphereBox,											//PxGeometryType::eBOX
		PxcPCMContactSphereConvex,										//PxGeometryType::eCONVEXMESH
		PxcPCMContactSphereMesh,										//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactSphereHeightField),	//PxGeometryType::eHEIGHTFIELD	//TODO: make HF midphase that will mask this	
	},

	//PxGeometryType::ePLANE
	{
		0,															//PxGeometryType::eSPHERE
		PxcInvalidContactPair,										//PxGeometryType::ePLANE
		PxcPCMContactPlaneCapsule,									//PxGeometryType::eCAPSULE
		PxcPCMContactPlaneBox,										//PxGeometryType::eBOX  
		PxcPCMContactPlaneConvex,										//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,										//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,										//PxGeometryType::eHEIGHTFIELD
	},  

	//PxGeometryType::eCAPSULE
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		PxcPCMContactCapsuleCapsule,									//PxGeometryType::eCAPSULE
		PxcPCMContactCapsuleBox,										//PxGeometryType::eBOX
		PxcPCMContactCapsuleConvex,										//PxGeometryType::eCONVEXMESH
		PxcPCMContactCapsuleMesh,										//PxGeometryType::eTRIANGLEMESH	
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactCapsuleHeightField),	//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eBOX
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		PxcPCMContactBoxBox,											//PxGeometryType::eBOX
		PxcPCMContactBoxConvex,											//PxGeometryType::eCONVEXMESH
		PxcPCMContactBoxMesh,											//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactBoxHeightField),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this

	},

	//PxGeometryType::eCONVEXMESH
	{
		0,																	//PxGeometryType::eSPHERE
		0,																	//PxGeometryType::ePLANE
		0,																	//PxGeometryType::eCAPSULE
		0,																	//PxGeometryType::eBOX
		PxcPCMContactConvexConvex,											//PxGeometryType::eCONVEXMESH
		PxcPCMContactConvexMesh,											//PxGeometryType::eTRIANGLEMESH
		DYNAMIC_CONTACT_REGISTRATION(PxcPCMContactConvexHeightField),		//PxGeometryType::eHEIGHTFIELD		//TODO: make HF midphase that will mask this
	},

	//PxGeometryType::eTRIANGLEMESH
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		0,																//PxGeometryType::eBOX
		0,																//PxGeometryType::eCONVEXMESH
		PxcInvalidContactPair,											//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,											//PxGeometryType::eHEIGHTFIELD
	},   

	//PxGeometryType::eHEIGHTFIELD
	{
		0,																//PxGeometryType::eSPHERE
		0,																//PxGeometryType::ePLANE
		0,																//PxGeometryType::eCAPSULE
		0,																//PxGeometryType::eBOX
		0,																//PxGeometryType::eCONVEXMESH
		0,																//PxGeometryType::eTRIANGLEMESH
		PxcInvalidContactPair,											//PxGeometryType::eHEIGHTFIELD
	},
};

void PxvRegisterHeightFields()
{
	g_ContactMethodTable[PxGeometryType::eSPHERE][PxGeometryType::eHEIGHTFIELD]			= PxcContactSphereHeightField;
	g_ContactMethodTable[PxGeometryType::eCAPSULE][PxGeometryType::eHEIGHTFIELD]		= PxcContactCapsuleHeightField;
	g_ContactMethodTable[PxGeometryType::eBOX][PxGeometryType::eHEIGHTFIELD]			= PxcContactBoxHeightField;
	g_ContactMethodTable[PxGeometryType::eCONVEXMESH][PxGeometryType::eHEIGHTFIELD]		= PxcContactConvexHeightField;

	g_PCMContactMethodTable[PxGeometryType::eSPHERE][PxGeometryType::eHEIGHTFIELD]		= PxcPCMContactSphereHeightField;
	g_PCMContactMethodTable[PxGeometryType::eCAPSULE][PxGeometryType::eHEIGHTFIELD]		= PxcPCMContactCapsuleHeightField;
	g_PCMContactMethodTable[PxGeometryType::eBOX][PxGeometryType::eHEIGHTFIELD]			= PxcPCMContactBoxHeightField;
	g_PCMContactMethodTable[PxGeometryType::eCONVEXMESH][PxGeometryType::eHEIGHTFIELD]	= PxcPCMContactConvexHeightField;
}

}
