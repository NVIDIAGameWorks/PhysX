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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "SampleVehicle.h"
#include "RenderPhysX3Debug.h"
#include "PxRigidDynamic.h"
#include "SampleVehicle_VehicleManager.h"

using namespace SampleRenderer;

// PT: this file contains the part of SampleVehicle dealing with cars' debug rendering

void SampleVehicle::drawWheels()
{
	PxSceneReadLock scopedLock(*mScene);
	const RendererColor colorPurple(255, 0, 255);

	for(PxU32 i=0;i<mVehicleManager.getNbVehicles();i++)
	{
		//Draw a rotating arrow to get an idea of the wheel rotation speed.
		PxVehicleWheels* veh=mVehicleManager.getVehicle(i);
		const PxRigidDynamic* actor=veh->getRigidDynamicActor();
		PxShape* shapeBuffer[PX_MAX_NB_WHEELS];
		actor->getShapes(shapeBuffer,veh->mWheelsSimData.getNbWheels());
		const PxTransform vehGlobalPose=actor->getGlobalPose();
		const PxU32 numWheels=veh->mWheelsSimData.getNbWheels();
		for(PxU32 j=0;j<numWheels;j++)
		{
			const PxTransform wheelTransform=vehGlobalPose.transform(shapeBuffer[j]->getLocalPose());
			const PxF32 wheelRadius=veh->mWheelsSimData.getWheelData(j).mRadius;
			const PxF32 wheelHalfWidth=veh->mWheelsSimData.getWheelData(j).mWidth*0.5f;
			PxVec3 offset=wheelTransform.q.getBasisVector0()*wheelHalfWidth;
			offset*= (veh->mWheelsSimData.getWheelCentreOffset(j).x > 0) ? 1.0f : -1.0f;
			const PxVec3 arrow=wheelTransform.rotate(PxVec3(0,0,1));
			getDebugRenderer()->addLine(wheelTransform.p+offset, wheelTransform.p+offset+arrow*wheelRadius, colorPurple);
		}
	}
}

void SampleVehicle::drawVehicleDebug()
{
	PxSceneReadLock scopedLock(*mScene);
	const RendererColor colorColl(255, 0, 0);
	const RendererColor colorCol2(0, 255, 0);
	const RendererColor colorCol3(0, 0, 255);
	
#if PX_DEBUG_VEHICLE_ON
	const PxVec3* tireForceAppPoints=NULL;
	const PxVec3* suspForceAppPoints=NULL;
	switch(mPlayerVehicleType)
	{
	case ePLAYER_VEHICLE_TYPE_VEHICLE4W:
	case ePLAYER_VEHICLE_TYPE_TANK4W:
		tireForceAppPoints=mTelemetryData4W->getTireforceAppPoints();
		suspForceAppPoints=mTelemetryData4W->getSuspforceAppPoints();
		break;
	case ePLAYER_VEHICLE_TYPE_VEHICLE6W:
	case ePLAYER_VEHICLE_TYPE_TANK6W:
		tireForceAppPoints=mTelemetryData6W->getTireforceAppPoints();
		suspForceAppPoints=mTelemetryData6W->getSuspforceAppPoints();
		break;
	default:
		PX_ASSERT(false);
		break;
	}
#endif

	const PxVehicleWheels& vehicle4W=*mVehicleManager.getVehicle(mPlayerVehicle);
	const PxVehicleWheelQueryResult& vehicleWheelQueryResults=mVehicleManager.getVehicleWheelQueryResults(mPlayerVehicle);
	const PxRigidDynamic* actor=vehicle4W.getRigidDynamicActor();
	const PxU32 numWheels=vehicle4W.mWheelsSimData.getNbWheels();
	PxVec3 v[8];
	PxVec3 w[8];
	PxF32 l[8];
	for(PxU32 i=0;i<numWheels;i++)
	{
		v[i] = vehicleWheelQueryResults.wheelQueryResults[i].suspLineStart;
		w[i] = vehicleWheelQueryResults.wheelQueryResults[i].suspLineDir;
		l[i] = vehicleWheelQueryResults.wheelQueryResults[i].suspLineLength;
	}


	const PxTransform t=actor->getGlobalPose().transform(actor->getCMassLocalPose());
	const PxVec3 dirs[3]={t.rotate(PxVec3(1,0,0)),t.rotate(PxVec3(0,1,0)),t.rotate(PxVec3(0,0,1))};
	getDebugRenderer()->addLine(t.p, t.p + dirs[0]*4.0f, colorColl);
	getDebugRenderer()->addLine(t.p, t.p + dirs[1]*4.0f, colorColl);
	getDebugRenderer()->addLine(t.p, t.p + dirs[2]*4.0f, colorColl);

	for(PxU32 j=0;j<numWheels;j++)
	{
		getDebugRenderer()->addLine(v[j], v[j]+w[j]*l[j], colorColl);

#if PX_DEBUG_VEHICLE_ON
		//Draw all tire force app points.
		const PxVec3& appPoint = tireForceAppPoints[j];
		getDebugRenderer()->addLine(appPoint - dirs[0], appPoint + dirs[0], colorCol2);
		getDebugRenderer()->addLine(appPoint - dirs[2], appPoint + dirs[2], colorCol2);

		//Draw all susp force app points.
		const PxVec3& appPoint2 = suspForceAppPoints[j];
		getDebugRenderer()->addLine(appPoint2 - dirs[0], appPoint2 + dirs[0], colorCol3);
		getDebugRenderer()->addLine(appPoint2 - dirs[2], appPoint2 + dirs[2], colorCol3);
#endif
	}
}

static void drawBox2D(Renderer* renderer, PxF32 minX, PxF32 maxX, PxF32 minY, PxF32 maxY, const RendererColor& color, PxF32 alpha)
{
	ScreenQuad sq;
	sq.mX0				= minX;
	sq.mY0				= 1.0f - minY;
	sq.mX1				= maxX;
	sq.mY1				= 1.0f - maxY;
	sq.mLeftUpColor		= color;
	sq.mRightUpColor	= color;
	sq.mLeftDownColor	= color;
	sq.mRightDownColor	= color;
	sq.mAlpha			= alpha;

	renderer->drawScreenQuad(sq);
}

static void print(Renderer* renderer, PxF32 x, PxF32 y, PxF32 scale_, const char* text)
{
	PxU32 width, height;
	renderer->getWindowSize(width, height);

	y = 1.0f - y;

	const PxReal scale = scale_*20.0f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, 255);

	renderer->print(PxU32(x*PxF32(width)), PxU32(y*PxF32(height)), text, scale, shadowOffset, textColor);
}

void SampleVehicle::drawHud()
{
	const PxVehicleWheels& focusVehicle = *mVehicleManager.getVehicle(mPlayerVehicle);
	PxVehicleDriveDynData* driveDynData=NULL;
	PxVehicleDriveSimData* driveSimData=NULL;
	switch(focusVehicle.getVehicleType())
	{
	case PxVehicleTypes::eDRIVE4W:
		{
			PxVehicleDrive4W& vehDrive4W=(PxVehicleDrive4W&)focusVehicle;
			driveDynData=&vehDrive4W.mDriveDynData;
			driveSimData=&vehDrive4W.mDriveSimData;
		}
		break;
	case PxVehicleTypes::eDRIVENW:
		{
			PxVehicleDriveNW& vehDriveNW=(PxVehicleDriveNW&)focusVehicle;
			driveDynData=&vehDriveNW.mDriveDynData;
			driveSimData=&vehDriveNW.mDriveSimData;
		}
		break;
	case PxVehicleTypes::eDRIVETANK:
		{
			PxVehicleDriveTank& vehDriveTank=(PxVehicleDriveTank&)focusVehicle;
			driveDynData=&vehDriveTank.mDriveDynData;
			driveSimData=&vehDriveTank.mDriveSimData;
		}
		break;
	default:
		PX_ASSERT(false);
		break;
	}

	const PxU32 currentGear=driveDynData->getCurrentGear();
	const PxF32 vz=mForwardSpeedHud*3.6f;
	const PxF32 revs=driveDynData->getEngineRotationSpeed();
	const PxF32 maxRevs=driveSimData->getEngineData().mMaxOmega*60*0.5f/PxPi;//Convert from radians per second to rpm
	const PxF32 invMaxRevs=driveSimData->getEngineData().getRecipMaxOmega();

	//Draw gears and speed.

	const PxF32 x=0.5f;
	const PxF32 y=0.18f;
	const PxF32 length=0.1f;
	const PxF32 textheight=0.02f;

	Renderer* renderer = getRenderer();
	drawBox2D(renderer, x-length-textheight, x+length+textheight, y, y+length+textheight, RendererColor(255, 255, 255), 0.5f);

	//Gear
	char gear[PxVehicleGearsData::eGEARSRATIO_COUNT][64]=
	{
		"R","N","1","2","3","4","5"
	};
	print(renderer, x-0.25f*textheight, y+0.02f, 0.02f, gear[currentGear]);

	//Speed
	char speed[64];
	sprintf(speed, "%1.0f %s",  PxAbs(vz), "kmph");
	print(renderer, x-textheight, y+length-textheight, textheight, speed);

	//Revs
	{
		const PxF32 xy[4]={x, 1.0f-y, x-length, 1.0f-(y+length)};
		renderer->drawLines2D(2, xy, RendererColor(0, 0, 255));

		char buffer[64];
		sprintf(buffer, "%d \n", 0);
		print(renderer, x-length, y+length, textheight, buffer);
	}
	{
		const PxF32 xy[4]={x, 1.0f-y, x+length, 1.0f-(y+length)};
		renderer->drawLines2D(2, xy, RendererColor(0, 0, 255));

		char buffer[64];
		sprintf(buffer, "%1.0f \n", maxRevs);
		print(renderer, x+length-2*textheight, y+length, textheight, buffer);
	}
	{
		const PxF32 alpha=revs*invMaxRevs;
		const PxF32 dx=-(1.0f-alpha)*length + alpha*length;
		const PxF32 xy[4]={x, 1.0f-y, x+dx, 1.0f-(y+length)};
		renderer->drawLines2D(2, xy, RendererColor(255, 0, 0));
	}
}

#if PX_DEBUG_VEHICLE_ON

static PX_FORCE_INLINE RendererColor getColor(const PxVec3& c)
{
	return RendererColor(PxU8(c.x), PxU8(c.y), PxU8(c.z));
}

static PX_FORCE_INLINE void convertColors(const PxVec3* src, RendererColor* dst)
{
	for(PxU32 i=0;i<PxVehicleGraph::eMAX_NB_SAMPLES;i++)
		*dst++ = getColor(src[i]);
}

static void convertY(PxF32* xy)
{
	for(PxU32 i=0;i<PxVehicleGraph::eMAX_NB_SAMPLES;i++)
		xy[2*i+1]=1.0f-xy[2*i+1];
}

#endif //PX_DEBUG_VEHICLE_ON

#if PX_DEBUG_VEHICLE_ON

void drawGraphsAndPrintTireSurfaceTypesN
(const PxVehicleTelemetryData& telemetryData, const PxU32* tireTypes, const PxU32* surfaceTypes,
 const PxU32 activeEngineGraphChannel, const PxU32 activeWheelGraphChannel,
 SampleRenderer::Renderer* renderer)
{

	PxF32 xy[2*PxVehicleGraph::eMAX_NB_SAMPLES];
	PxVec3 color[PxVehicleGraph::eMAX_NB_SAMPLES];
	RendererColor rendererColor[PxVehicleGraph::eMAX_NB_SAMPLES];
	char title[PxVehicleGraph::eMAX_NB_TITLE_CHARS];

	const PxU32 numWheelGraphs=telemetryData.getNbWheelGraphs();

	for(PxU32 i=0;i<numWheelGraphs;i++)
	{
		PxF32 xMin,xMax,yMin,yMax;
		telemetryData.getWheelGraph(i).getBackgroundCoords(xMin,yMin,xMax,yMax);
		const PxVec3& backgroundColor=telemetryData.getWheelGraph(i).getBackgroundColor();
		const PxF32 alpha=telemetryData.getWheelGraph(i).getBackgroundAlpha();
		drawBox2D(renderer, xMin,xMax,yMin,yMax, getColor(backgroundColor),alpha);

		telemetryData.getWheelGraph(i).computeGraphChannel(activeWheelGraphChannel,xy,color,title);
		convertY(xy);
		convertColors(color, rendererColor);
		renderer->drawLines2D(PxVehicleGraph::eMAX_NB_SAMPLES, xy, rendererColor);

		print(renderer, xMin,yMax-0.02f, 0.02f, title);

		const PxU32 tireType=tireTypes[i];
		const PxU32 tireSurfaceType=surfaceTypes[i];

		if (PxVehicleDrivableSurfaceType::eSURFACE_TYPE_UNKNOWN!=tireSurfaceType)
		{
			const char* surfaceType= SurfaceTypeNames::getName(tireSurfaceType);
			const PxF32 friction=TireFrictionMultipliers::getValue(tireSurfaceType, tireType);
			char surfaceDetails[64];
			sprintf(surfaceDetails, "%s %1.2f \n", surfaceType, friction);
			print(renderer, xMin+0.1f, yMax-0.12f, 0.02f, surfaceDetails);
		}
	}

	PxF32 xMin,xMax,yMin,yMax;
	telemetryData.getEngineGraph().getBackgroundCoords(xMin,yMin,xMax,yMax);
	const PxVec3& backgroundColor=telemetryData.getEngineGraph().getBackgroundColor();
	const PxF32 alpha=telemetryData.getEngineGraph().getBackgroundAlpha();
	drawBox2D(renderer, xMin,xMax,yMin,yMax, getColor(backgroundColor),alpha);

	telemetryData.getEngineGraph().computeGraphChannel(activeEngineGraphChannel,xy,color,title);
	convertY(xy);
	convertColors(color, rendererColor);
	renderer->drawLines2D(PxVehicleGraph::eMAX_NB_SAMPLES, xy, rendererColor);

	print(renderer, xMin,yMax-0.02f,0.02f,title);
}

#endif //PX_DEBUG_VEHICLE_GRAPH_ON

void SampleVehicle::drawGraphsAndPrintTireSurfaceTypes(const PxVehicleWheels& focusVehicle, const PxVehicleWheelQueryResult& focusVehicleWheelQueryResults)
{
#if PX_DEBUG_VEHICLE_ON

	PxU32 tireTypes[8];
	PxU32 surfaceTypes[8];
	const PxU32 numWheels=focusVehicle.mWheelsSimData.getNbWheels();
	PX_ASSERT(numWheels<=8);
	for(PxU32 i=0;i<numWheels;i++)
	{
		tireTypes[i]=focusVehicle.mWheelsSimData.getTireData(i).mType;
		surfaceTypes[i]=focusVehicleWheelQueryResults.wheelQueryResults[i].tireSurfaceType;
	}

	PxVehicleTelemetryData* vehTelData=NULL;
	switch(mPlayerVehicleType)
	{
	case ePLAYER_VEHICLE_TYPE_VEHICLE4W:
	case ePLAYER_VEHICLE_TYPE_TANK4W:
		vehTelData=mTelemetryData4W;
		break;
	case ePLAYER_VEHICLE_TYPE_VEHICLE6W:
	case ePLAYER_VEHICLE_TYPE_TANK6W:
		vehTelData=mTelemetryData6W;
		break;
	default:
		PX_ASSERT(false);
		break;
	}

	drawGraphsAndPrintTireSurfaceTypesN(
		*vehTelData,tireTypes,surfaceTypes,
		mDebugRenderActiveGraphChannelEngine,mDebugRenderActiveGraphChannelWheel,
		getRenderer());

#endif
}


