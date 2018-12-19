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

#include "PsHashMap.h"
#include "InternalRenderDebug.h"
#include "RenderDebug.h"
#include "RenderDebugImpl.h"
#include "ProcessRenderDebug.h"
#include "PsMemoryBuffer.h"

#include "PsPool.h"
#include "PsString.h"
#include "PxBounds3.h"
#include "StreamIO.h"

namespace RENDER_DEBUG
{

#define RENDER_STATE_STACK_SIZE 16 // maximum size of the render state stack
const float FM_PI = 3.1415926535897932384626433832795028841971693993751f;
const float FM_DEG_TO_RAD = ((2.0f * FM_PI) / 360.0f);
//const float FM_RAD_TO_DEG = (360.0f / (2.0f * FM_PI));


class PostRenderDebug
{
	PX_NOCOPY(PostRenderDebug)
public:

	PostRenderDebug(void) 
	{
		mSendBufferCount = 0;
		// since all of these commands are processed internally, in memory buffers, we don't need to be endian specific.
		mWorldSpace.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
		mScreenSpace.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
		mNoZ.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	}

	~PostRenderDebug(void)
	{
	}

	PX_INLINE void postRenderDebug(DebugPrimitive *p,const RenderState &rs)
	{
//		if ( p->mCommand == DebugCommand::DEBUG_GRAPH )
//			return;

		uint32_t plen = DebugCommand::getPrimtiveSize(*p);
		float dtime = rs.getDisplayTime();

		switch ( p->mCommand )
		{
			case DebugCommand::DEBUG_GRAPH:
				{
					DebugGraphStream *dgs = static_cast< DebugGraphStream *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugGraphStream *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_CREATE_TRIANGLE_MESH:
				{
					DebugCreateTriangleMesh *dgs = static_cast< DebugCreateTriangleMesh *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugCreateTriangleMesh *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_RENDER_TRIANGLE_MESH_INSTANCES:
				{
					DebugRenderTriangleMeshInstances *dgs = static_cast< DebugRenderTriangleMeshInstances *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugRenderTriangleMeshInstances *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_CONVEX_HULL:
				{
					DebugConvexHull *dgs = static_cast< DebugConvexHull *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugConvexHull *>(p);
					dgs->mSize = plen;
				}

				break;
			case DebugCommand::DEBUG_RENDER_POINTS:
				{
					DebugRenderPoints *dgs = static_cast< DebugRenderPoints *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugRenderPoints *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_RENDER_LINES:
				{
					DebugRenderLines *dgs = static_cast< DebugRenderLines *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugRenderLines *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_RENDER_TRIANGLES:
				{
					DebugRenderTriangles *dgs = static_cast< DebugRenderTriangles *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugRenderTriangles *>(p);
					dgs->mSize = plen;
				}
				break;
			case DebugCommand::DEBUG_REFRESH_TRIANGLE_MESH_VERTICES:
				{
					DebugRefreshTriangleMeshVertices *dgs = static_cast< DebugRefreshTriangleMeshVertices *>(p);
					plen = dgs->mSize;
					p = reinterpret_cast<DebugPrimitive *>(dgs->mBuffer);
					dgs = static_cast< DebugRefreshTriangleMeshVertices *>(p);
					dgs->mSize = plen;
				}
				break;
		}


		if ( rs.isScreen() || p->mCommand == DebugCommand::DEBUG_GRAPH )
		{
			processState(rs,mScreenSpaceRenderState,mScreenSpace,dtime);
			mScreenSpace.write(&dtime,sizeof(dtime));
			mScreenSpace.write(p,plen);
		}
		else if ( rs.isUseZ() )
		{
			processState(rs,mWorldSpaceRenderState,mWorldSpace,dtime);
			mWorldSpace.write(&dtime,sizeof(dtime));
			mWorldSpace.write(p,plen);
		}
		else
		{
			processState(rs,mNoZRenderState,mNoZ,dtime);
			mNoZ.write(&dtime,sizeof(dtime));
			mNoZ.write(p,plen);
		}
	}

	void release(void)
	{
		delete this;
	}


	bool processRenderDebug(ProcessRenderDebug *rd,float dtime,RENDER_DEBUG::RenderDebugInterface *iface)
	{

		PX_ASSERT(dtime>=0);
		process(mWorldSpace,rd,dtime,iface,ProcessRenderDebug::WORLD_SPACE);
		process(mNoZ,rd,dtime,iface,ProcessRenderDebug::WORLD_SPACE_NOZ);
		process(mScreenSpace,rd,dtime,iface,ProcessRenderDebug::SCREEN_SPACE);
		rd->flushFrame(iface);

		new ( &mWorldSpaceRenderState ) RenderState;
		new ( &mScreenSpaceRenderState ) RenderState;
		new ( &mNoZRenderState ) RenderState;

		return true;
	}

	PX_INLINE bool intersects(const uint8_t *a1,const uint8_t *a2,const uint8_t *b1,const uint8_t *b2)
	{
		bool ret = true;
		if ( a1 >= b2 || b1 >= a2 ) ret = false;
		return ret;
	}

	// This method processes the current stream of draw commands.
	// Each draw command is preceded by its 'lifetime'.
	// If the object is still alive after it is processed, it is retained in the buffer.
	void process(physx::PsMemoryBuffer &stream,
				ProcessRenderDebug *rd,
				float dtime,
				RENDER_DEBUG::RenderDebugInterface *iface,
				ProcessRenderDebug::DisplayType type)
	{
		if ( stream.tellWrite() == 0 ) return;

		uint8_t *dest = const_cast<uint8_t *>(stream.getWriteBuffer());

		const uint8_t *sendStart = NULL;
		const uint8_t *sendEnd   = NULL;

		float displayTime;
		uint32_t r = stream.read(&displayTime,sizeof(displayTime));

		BlockInfo *info = NULL;

		while ( r == sizeof(displayTime) )
		{
			displayTime-=dtime;
			bool alive = displayTime > 0.0f;
			uint32_t cmd;
			r = stream.peek(&cmd,sizeof(cmd));

			PX_ASSERT( r == sizeof(cmd) );
			PX_ASSERT( cmd < DebugCommand::DEBUG_LAST );

			switch ( cmd )
			{
				case DebugCommand::DEBUG_MESSAGE:
				case DebugCommand::DEBUG_CREATE_TRIANGLE_MESH:
				case DebugCommand::DEBUG_RELEASE_TRIANGLE_MESH:
				case DebugCommand::DEBUG_REFRESH_TRIANGLE_MESH_VERTICES:
				case DebugCommand::DEBUG_CREATE_CUSTOM_TEXTURE:
				case DebugCommand::DEBUG_REGISTER_INPUT_EVENT:
				case DebugCommand::DEBUG_UNREGISTER_INPUT_EVENT:
				case DebugCommand::DEBUG_RESET_INPUT_EVENTS:
				case DebugCommand::DEBUG_SKIP_FRAME:
					alive = false;
					break;
			}

			const uint8_t *readLoc = stream.getReadLoc();
			DebugPrimitive *dp = const_cast<DebugPrimitive*>(reinterpret_cast<const DebugPrimitive *>(readLoc));
			uint32_t plen = DebugCommand::getPrimtiveSize(*dp);
			stream.advanceReadLoc(plen);

			if ( sendStart == NULL )
			{
				sendStart = readLoc;
			}
			sendEnd = readLoc+plen;

			if ( dp->mCommand == DebugCommand::DEBUG_BLOCK_INFO )
			{
				DebugBlockInfo *db = static_cast< DebugBlockInfo *>(dp);
				info = db->mInfo;

				if ( info && info->mChanged )
				{
					db->mCurrentTransform = info->mPose;
					info->mChanged = false;
				}
				if ( info && info->mVisibleState )
				{
					DebugPrimitive *setCurrentTransform = static_cast< DebugPrimitive *>(&db->mCurrentTransform);
					mSendBuffer[mSendBufferCount] = setCurrentTransform;
					mSendBufferCount++;
				}
			}
			else if ( info )
			{
				if ( info->mVisibleState )
				{
					mSendBuffer[mSendBufferCount] = dp;
					mSendBufferCount++;
				}
				else if (isRenderState(dp))
				{
					mSendBuffer[mSendBufferCount] = dp;
					mSendBufferCount++; 
				}
			}
			else
			{
				mSendBuffer[mSendBufferCount] = dp;
				mSendBufferCount++;
			}

   			if ( mSendBufferCount == MAX_SEND_BUFFER )
   			{
   				rd->processRenderDebug(mSendBuffer,mSendBufferCount,iface,type);
   				mSendBufferCount = 0;
   				sendStart = sendEnd = NULL;
   			}

   			if ( alive )
   			{
				if ( sendStart && intersects(sendStart,sendEnd,dest,dest+plen+sizeof(float)) )
				{
					if ( mSendBufferCount )
					{
						rd->processRenderDebug(mSendBuffer,mSendBufferCount,iface,type);
					}
					mSendBufferCount = 0;
					sendStart = sendEnd = NULL;
				}

				// Store the new display time!
				float *fdest = reinterpret_cast<float *>(dest);
				*fdest = displayTime;
				dest+=sizeof(float);

   				if ( dest != reinterpret_cast<uint8_t *>(dp) ) // if the source and dest are the same, we don't need to move memory
   				{
 					memcpy(dest,dp,plen);
				}
				dest+=plen;
			}

			r = stream.read(&displayTime,sizeof(displayTime));
		}

		if ( mSendBufferCount )
		{
			rd->processRenderDebug(mSendBuffer,mSendBufferCount,iface,type);
			mSendBufferCount = 0;
		}

		if ( info )
		{
			info->mChanged = false;
		}

		stream.setWriteLoc(dest);
		rd->flush(iface,type);
		stream.seekRead(0);
	}

	void reset(BlockInfo *info)
	{
		reset(mWorldSpace,info);
		reset(mNoZ,info);
		reset(mScreenSpace,info);
	}

	// if resetInfo is NULL, then totally erase the persistent data stream.
	// if resetInfo is non-NULL, then erease all parts of the stream which are associated with this data block.
	void reset(physx::PsMemoryBuffer &stream,BlockInfo *resetInfo)
	{
   		if ( stream.tellWrite() == 0 ) return;
   		// remove all data
		if ( resetInfo )
		{
    		uint8_t *dest = const_cast<uint8_t *>(stream.getWriteBuffer());
    		float displayTime;
    		uint32_t r = stream.read(&displayTime,sizeof(displayTime));
    		BlockInfo *info = NULL;
    		while ( r == sizeof(displayTime) )
    		{
				char scratch[sizeof(DebugGraphStream)];
    			DebugGraphStream *dgs = reinterpret_cast<DebugGraphStream *>(scratch);
    			r = stream.peek(dgs,sizeof(DebugGraphStream));
    			PX_ASSERT( r >= 8);
    			PX_ASSERT( dgs->mCommand < DebugCommand::DEBUG_LAST );
    			uint32_t plen = DebugCommand::getPrimtiveSize(*dgs);
    			const uint8_t *readLoc = stream.getReadLoc();
    			DebugPrimitive *dp = reinterpret_cast<DebugPrimitive *>(const_cast<uint8_t*>(readLoc));
    			stream.advanceReadLoc(plen);
    			if ( dp->mCommand == DebugCommand::DEBUG_BLOCK_INFO )
    			{
    				DebugBlockInfo *db = static_cast< DebugBlockInfo *>(dp);
    				info = db->mInfo;
    			}
       			if ( info != resetInfo )
       			{
    				// Store the new display time!
    				float *fdest = reinterpret_cast<float *>(dest);
    				*fdest = displayTime;
    				dest+=sizeof(float);
       				if ( dest != reinterpret_cast<uint8_t *>(dp) ) // if the source and dest are the same, we don't need to move memory
       				{
     					memcpy(dest,dp,plen);
    				}
    				dest+=plen;

    			}
    			r = stream.read(&displayTime,sizeof(displayTime));
    		}
    		stream.setWriteLoc(dest);
    		stream.seekRead(0);
		}
		else
		{
			// kill all saved data!
			uint8_t *dest = const_cast<uint8_t *>(stream.getWriteBuffer());
			stream.setWriteLoc(dest);
			stream.seekRead(0);
		}
	}

	void updateBufferChangeCount(const RenderState &rs)
	{
		if ( rs.isScreen() /*|| p->mCommand == DebugCommand::DEBUG_GRAPH*/ )
			mScreenSpaceRenderState.mChangeCount = rs.getChangeCount();
		else if ( rs.isUseZ() )
			mWorldSpaceRenderState.mChangeCount = rs.getChangeCount();
		else
			mNoZRenderState.mChangeCount = rs.getChangeCount();
	}

	void updatePostDrawGroupPose (const RenderState &rs)
	{
		float dtime = rs.getDisplayTime();

		if ( rs.isScreen() /*|| p->mCommand == DebugCommand::DEBUG_GRAPH*/ )
			setCurrentTransform(rs, mScreenSpaceRenderState, mScreenSpace, dtime);
		else if ( rs.isUseZ() )
			setCurrentTransform(rs, mWorldSpaceRenderState, mWorldSpace, dtime);
		else
			setCurrentTransform(rs, mNoZRenderState, mNoZ, dtime);
	}

private:

	PX_INLINE void processState(const RenderState &source,
								RenderState &dest,
								physx::PsMemoryBuffer &stream,
								float lifeTime)
	{
		if ( dest.getChangeCount() == 0 )
		{
			setColor(source,dest,stream,lifeTime);
			setArrowColor(source,dest,stream,lifeTime);
			setTexture(source,dest,stream,lifeTime);
			setArrowSize(source,dest,stream,lifeTime);
			setRenderScale(source,dest,stream,lifeTime);
			setUserId(source,dest,stream,lifeTime);
			setStates(source,dest,stream,lifeTime);
			setTextScale(source,dest,stream,lifeTime);
			setCurrentTransform(source,dest,stream,lifeTime);
			setBlockInfo(source,dest,stream,lifeTime);
			dest.incrementChangeCount();
		}
		else if ( source.getChangeCount() != dest.getChangeCount() )
		{
			applyStateChanges(source,dest,stream,lifeTime);
		}
	}

	PX_INLINE void setColor(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_COLOR,source.mColor);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mColor = source.mColor;
	}

	PX_INLINE void setRenderScale(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveF32 d(DebugCommand::SET_CURRENT_RENDER_SCALE,source.mRenderScale);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mRenderScale = source.mRenderScale;
	}

	PX_INLINE void setArrowSize(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveF32 d(DebugCommand::SET_CURRENT_ARROW_SIZE,source.mArrowSize);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mArrowSize = source.mArrowSize;
	}

	PX_INLINE void setUserId(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_USER_ID,uint32_t(source.mUserId));
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mUserId = source.mUserId;
	}

	PX_INLINE void setArrowColor(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_ARROW_COLOR,source.mArrowColor);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mArrowColor = source.mArrowColor;
	}

	PX_INLINE void setTexture(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		if ( dest.mTexture1 != source.mTexture1 )
		{
			DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_TEXTURE1,source.mTexture1);
			stream.write(&lifeTime,sizeof(lifeTime));
			stream.write(&d,sizeof(d));
			dest.mTexture1 = source.mTexture1;
		}
		if ( dest.mTexture2 != source.mTexture2 )
		{
			DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_TEXTURE2,source.mTexture2);
			stream.write(&lifeTime,sizeof(lifeTime));
			stream.write(&d,sizeof(d));
			dest.mTexture2 = source.mTexture2;
		}
		if ( dest.mTileRate1 != source.mTileRate2 )
		{
			DebugPrimitiveF32 d(DebugCommand::SET_CURRENT_TILE1,source.mTileRate1);
			stream.write(&lifeTime,sizeof(lifeTime));
			stream.write(&d,sizeof(d));
			dest.mTileRate1 = source.mTileRate1;
		}
		if ( dest.mTileRate2 != source.mTileRate2 )
		{
			DebugPrimitiveF32 d(DebugCommand::SET_CURRENT_TILE2,source.mTileRate2);
			stream.write(&lifeTime,sizeof(lifeTime));
			stream.write(&d,sizeof(d));
			dest.mTileRate2 = source.mTileRate2;
		}

	}


	PX_INLINE void setStates(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveU32 d(DebugCommand::SET_CURRENT_RENDER_STATE,source.mStates);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mStates = source.mStates;
	}

	PX_INLINE void setTextScale(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugPrimitiveF32 d(DebugCommand::SET_CURRENT_TEXT_SCALE,source.mTextScale);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mTextScale = source.mTextScale;
	}

	PX_INLINE void setCurrentTransform(const RenderState &source,
										RenderState &dest,
										physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugSetCurrentTransform d(source.mPose ? *source.mPose : physx::PxMat44(physx::PxIdentity) );
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		if ( source.mPose )
		{
			dest.mCurrentPose = *source.mPose;
			dest.mPose = &dest.mCurrentPose;
		}
		else
		{
			dest.mPose = NULL;
		}
	}

	PX_INLINE void setBlockInfo(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		DebugBlockInfo d(source.mBlockInfo);
		stream.write(&lifeTime,sizeof(lifeTime));
		stream.write(&d,sizeof(d));
		dest.mBlockInfo = source.mBlockInfo;
	}


	void applyStateChanges(const RenderState &source,RenderState &dest,physx::PsMemoryBuffer &stream,float lifeTime)
	{
		if ( source.mColor != dest.mColor )
		{
			setColor(source,dest,stream,lifeTime);
		}
		if ( source.mArrowColor != dest.mArrowColor )
		{
			setArrowColor(source,dest,stream,lifeTime);
		}
		{
			setTexture(source,dest,stream,lifeTime);
		}
		if ( source.mRenderScale != dest.mRenderScale )
		{
			setRenderScale(source,dest,stream,lifeTime);
		}
		if ( source.mArrowSize != dest.mArrowSize )
		{
			setArrowSize(source,dest,stream,lifeTime);
		}
		if ( source.mUserId != dest.mUserId )
		{
			setUserId(source,dest,stream,lifeTime);
		}
		if ( source.mStates != dest.mStates )
		{
			setStates(source,dest,stream,lifeTime);
		}
		if ( source.mTextScale != dest.mTextScale )
		{
			setTextScale(source,dest,stream,lifeTime);
		}
		if ( source.mPose != dest.mPose )
		{
			if ( !sameMat44(source.mPose,dest.mPose) )
			{
				setCurrentTransform(source,dest,stream,lifeTime);
    		}
		}
		if ( source.mBlockInfo != dest.mBlockInfo )
		{
			setBlockInfo(source,dest,stream,lifeTime);
		}
		dest.mChangeCount = source.mChangeCount;
	}

	bool sameMat44(const physx::PxMat44 *a,const physx::PxMat44 *b) const
	{
		if ( a == b ) return true;
		if ( a == NULL || b == NULL ) return false;
		const float *f1 = a->front();
		const float *f2 = b->front();
		for (uint32_t i=0; i<16; i++)
		{
			float diff = physx::PxAbs(f1[i]-f2[i]);
			if ( diff > 0.000001f ) return false;
		}
		return true;
	}

	bool isRenderState(const DebugPrimitive * const dp) const
	{
		return( dp->mCommand == DebugCommand::SET_CURRENT_COLOR || 
				dp->mCommand == DebugCommand::SET_CURRENT_TILE1 || 
				dp->mCommand == DebugCommand::SET_CURRENT_TILE2 || 
				dp->mCommand == DebugCommand::SET_CURRENT_TEXTURE1 || 
				dp->mCommand == DebugCommand::SET_CURRENT_TEXTURE2 || 
				dp->mCommand == DebugCommand::SET_CURRENT_ARROW_COLOR ||
				dp->mCommand == DebugCommand::SET_CURRENT_ARROW_SIZE ||
				dp->mCommand == DebugCommand::SET_CURRENT_RENDER_SCALE ||
				dp->mCommand == DebugCommand::SET_CURRENT_USER_ID ||
				dp->mCommand == DebugCommand::SET_CURRENT_RENDER_STATE ||
				dp->mCommand == DebugCommand::SET_CURRENT_TEXT_SCALE ||
				dp->mCommand == DebugCommand::SET_CURRENT_TRANSFORM );
	}

	physx::PsMemoryBuffer	mWorldSpace;
	physx::PsMemoryBuffer  mScreenSpace;
	physx::PsMemoryBuffer	mNoZ;

	RenderState		mWorldSpaceRenderState;
	RenderState		mScreenSpaceRenderState;
	RenderState		mNoZRenderState;

	uint32_t			mSendBufferCount;
	const DebugPrimitive	*mSendBuffer[MAX_SEND_BUFFER];
};




class InternalRenderDebug : public RenderDebugImpl, public physx::shdfnd::UserAllocated, PostRenderDebug
{
public:

	InternalRenderDebug(ProcessRenderDebug *process,RenderDebugHook *renderDebugHook)
	{
		mCanSkip = true;
		mRenderDebugHook = renderDebugHook;
		mFrameTime = 1.0f / 60.0f;
		mBlockIndex = 0;
		mStackIndex = 0;
		mUpdateCount = 0;
		mOwnProcess = false;
		if ( process )
		{
			mProcessRenderDebug = process;
		}
		else
		{
			mProcessRenderDebug = createProcessRenderDebug();
			mOwnProcess = true;
		}
		initColors();
	}

	~InternalRenderDebug(void)
	{
		if ( mOwnProcess )
		{
			mProcessRenderDebug->release();
		}
	}

	virtual uint32_t getUpdateCount(void) const
	{
		return mUpdateCount;
	}

    virtual bool renderImpl(float dtime,RENDER_DEBUG::RenderDebugInterface *iface)
    {
    	bool ret = false;

		ret = PostRenderDebug::processRenderDebug(mProcessRenderDebug,dtime,iface);
		mUpdateCount++;

		mProcessRenderDebug->finalizeFrame();
		mCanSkip = true; // next frame state

    	return ret;
    }

	virtual void  reset(int32_t blockIndex=-1)  // -1 reset *everything*, 0 = reset everything except stuff inside blocks, > 0 reset a specific block of data.
	{
		if ( blockIndex == -1 )
		{
			PostRenderDebug::reset(NULL);
		}
		else
		{
			const physx::shdfnd::HashMap<uint32_t, BlockInfo *>::Entry *e = mBlocksHash.find(uint32_t(blockIndex));
			if ( e )
			{
				BlockInfo *b = e->second;
				PostRenderDebug::reset(b);
			}
		}
	}


	virtual void  drawGrid(bool zup=false,uint32_t gridSize=40)  // draw a grid.
	{
		DrawGrid d(zup,gridSize);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  pushRenderState(void)
	{
		PX_ASSERT( mStackIndex < RENDER_STATE_STACK_SIZE );
		if ( mStackIndex < RENDER_STATE_STACK_SIZE )
		{
			mRenderStateStack[mStackIndex] = mCurrentState;
			if ( mCurrentState.mPose )
			{
				mRenderStateStack[mStackIndex].mPose = &mRenderStateStack[mStackIndex].mCurrentPose;
			}
			mStackIndex++;
		}
	}

    virtual void  popRenderState(void)
    {
		PX_ASSERT(mStackIndex);
		if ( mStackIndex > 0 )
		{
			mStackIndex--;
			mCurrentState = mRenderStateStack[mStackIndex];
			if ( mRenderStateStack[mStackIndex].mPose )
			{
				mCurrentState.mPose = &mCurrentState.mCurrentPose;
			}
			PostRenderDebug::updateBufferChangeCount(mCurrentState);
			mCurrentState.incrementChangeCount();
		}
    }


	virtual void  setCurrentColor(uint32_t color=0xFFFFFF,uint32_t arrowColor=0xFF0000)
	{
		mCurrentState.mColor = color;
		mCurrentState.mArrowColor = arrowColor;
		mCurrentState.incrementChangeCount();
	}

	virtual uint32_t getCurrentColor(void) const
	{
		return mCurrentState.mColor;
	}

	/**
	\brief Set the current debug texture

	\param textureEnum1 Which predefined texture to use as the primary texture
	\param tileRate1 The tiling rate to use for the primary texture
	\param textureEnum2 Which (optional) predefined texture to use as the primary texture
	\param tileRate2 The tiling rate to use for the secondary texture
	*/
	virtual void setCurrentTexture(DebugTextures::Enum textureEnum1,float tileRate1,DebugTextures::Enum textureEnum2,float tileRate2) 
	{
		mCurrentState.mTexture1 = textureEnum1;
		mCurrentState.mTexture2 = textureEnum2;
		mCurrentState.mTileRate1 = tileRate1;
		mCurrentState.mTileRate2 = tileRate2;
		mCurrentState.incrementChangeCount();
	}

	/**
	\brief Gets the current tiling rate of the primary texture

	\return The current tiling rate of the primary texture
	*/
	virtual float getCurrentTile1(void) const 
	{
		return mCurrentState.mTileRate1;
	}

	/**
	\brief Gets the current tiling rate of the secondary texture

	\return The current tiling rate of the secondary texture
	*/
	virtual float getCurrentTile2(void) const
	{
		return mCurrentState.mTileRate2;
	}

	/**
	\brief Gets the current debug texture

	\return The current texture id
	*/
	virtual DebugTextures::Enum getCurrentTexture1(void) const 
	{
		return static_cast<DebugTextures::Enum>(mCurrentState.mTexture1);
	}

	/**
	\brief Gets the current debug texture

	\return The current texture id
	*/
	virtual DebugTextures::Enum getCurrentTexture2(void) const 
	{
		return static_cast<DebugTextures::Enum>(mCurrentState.mTexture2);
	}


	virtual uint32_t getCurrentArrowColor(void) const
	{
		return mCurrentState.mArrowColor;
	}


	virtual void  setCurrentUserId(int32_t userId)
	{
		mCurrentState.mUserId = userId;
		mCurrentState.incrementChangeCount();
	}

	virtual int32_t getCurrentUserId(void)
	{
		return mCurrentState.mUserId;
	}

	virtual void  setCurrentDisplayTime(float displayTime=0.0001f)
	{
		mCurrentState.mDisplayTime = displayTime;
		mCurrentState.incrementChangeCount();
	}

	virtual float getRenderScale(void)
	{
		return mCurrentState.mRenderScale;
	}

	virtual void  setRenderScale(float scale)
	{
		mCurrentState.mRenderScale = scale;
		mCurrentState.incrementChangeCount();
	}

	virtual void  setCurrentState(uint32_t states=0)
	{
		mCurrentState.mStates = states;
		mCurrentState.incrementChangeCount();
	}

	virtual void  addToCurrentState(RENDER_DEBUG::DebugRenderState::Enum state)  // OR this state flag into the current state.
	{
		mCurrentState.mStates|=state;
		mCurrentState.incrementChangeCount();
	}

	virtual void  removeFromCurrentState(RENDER_DEBUG::DebugRenderState::Enum state)  // Remove this bit flat from the current state
	{
		mCurrentState.mStates&=~state;
		mCurrentState.incrementChangeCount();
	}

	virtual void  setCurrentTextScale(float textScale)
	{
		mCurrentState.mTextScale = textScale;
		mCurrentState.incrementChangeCount();
	}

	virtual void  setCurrentArrowSize(float arrowSize)
	{
		mCurrentState.mArrowSize = arrowSize;
		mCurrentState.incrementChangeCount();
	}

	virtual uint32_t getCurrentState(void) const
	{
		return mCurrentState.mStates;
	}

	virtual void  setRenderState(uint32_t states=0,  // combination of render state flags
	                             uint32_t color=0xFFFFFF, // base color
                                 float displayTime=0.0001f, // duration of display items.
	                             uint32_t arrowColor=0xFF0000, // secondary color, usually used for arrow head
                                 float arrowSize=0.1f,
								 float renderScale=1.0f,
								 float textScale=1.0f)      // seconary size, usually used for arrow head size.
	{
		uint32_t saveChangeCount = mCurrentState.mChangeCount;
		new ( &mCurrentState ) RenderState(states,color,displayTime,arrowColor,arrowSize,renderScale,textScale);
		mCurrentState.mChangeCount = saveChangeCount;
		mCurrentState.incrementChangeCount();
	}


	virtual uint32_t getRenderState(uint32_t &color,float &displayTime,uint32_t &arrowColor,float &arrowSize,float &renderScale,float & /*textScale*/) const
	{
		color = mCurrentState.mColor;
		displayTime = mCurrentState.mDisplayTime;
		arrowColor = mCurrentState.mArrowColor;
		arrowSize = mCurrentState.mArrowSize;
		renderScale = mCurrentState.mRenderScale;
		return mCurrentState.mStates;
	}


	virtual void  endDrawGroup(void)
	{
		popRenderState();
		PostRenderDebug::updatePostDrawGroupPose(mCurrentState);
	}

	virtual void  setDrawGroupVisible(int32_t blockId,bool state)
	{
		const physx::shdfnd::HashMap<uint32_t, BlockInfo *>::Entry *e = mBlocksHash.find(uint32_t(blockId));
		if ( e )
		{
			BlockInfo *b = e->second;
			if ( b->mVisibleState != state )
			{
				b->mVisibleState = state;
			}
		}
	}

	virtual void  debugPolygon(uint32_t pcount,const physx::PxVec3 *points)
	{
		if ( mCurrentState.isSolid() )
		{
			PX_ASSERT( pcount >= 3 );
			PX_ASSERT( pcount <= 256 );
			bool wasOverlay = mCurrentState.hasRenderState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
			if( wasOverlay )
			{
				mCurrentState.clearRenderState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
				mCurrentState.incrementChangeCount();
			}
			const physx::PxVec3 *v1 = &points[0];
			const physx::PxVec3 *v2 = &points[1];
			const physx::PxVec3 *v3 = &points[2];
			debugTri(*v1, *v2, *v3);
			for (uint32_t i=3; i<pcount; i++)
			{
				v2 = v3;
				v3 = &points[i];
				debugTri(*v1, *v2, *v3);
			}
			if ( wasOverlay )
			{
        		for (uint32_t i=0; i<(pcount-1); i++)
        		{
        			debugLine( points[i], points[i+1] );
        		}
        		debugLine(points[pcount-1],points[0]);
				mCurrentState.setRenderState(RENDER_DEBUG::DebugRenderState::SolidWireShaded);
				mCurrentState.incrementChangeCount();
			}
		}
		else
		{
    		for (uint32_t i=0; i<(pcount-1); i++)
    		{
    			debugLine( points[i], points[i+1] );
    		}
    		debugLine(points[pcount-1],points[0]);
    	}
	}

	virtual void  debugLine(const physx::PxVec3 &p1,const physx::PxVec3 &p2)
	{
		DebugLine d(p1,p2);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}


	virtual void debugFrustum(const physx::PxMat44 &view,const physx::PxMat44 &proj)
	{
		DebugFrustum f(view,proj);
		PostRenderDebug::postRenderDebug(&f,mCurrentState);
	}

	virtual void debugCone(float length,float innerAngle,float outerAngle,uint32_t subDivision,bool closeEnd)
	{
		DebugCone c(length,innerAngle,outerAngle,subDivision,closeEnd);
		PostRenderDebug::postRenderDebug(&c,mCurrentState);
	}

	virtual void  debugBound(const physx::PxBounds3 &_b)
	{
		DebugBound b(_b.minimum,_b.maximum );
		PostRenderDebug::postRenderDebug(&b,mCurrentState);
	}

	virtual void  debugBound(const physx::PxVec3 &bmin,const physx::PxVec3 &bmax)
	{
		DebugBound b(bmin,bmax);
		PostRenderDebug::postRenderDebug(&b,mCurrentState);
	}

	virtual void  debugPoint(const physx::PxVec3 &pos,float radius)
	{
		DebugPoint d(pos,radius);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugQuad(const physx::PxVec3 &pos,const physx::PxVec2 &scale,float rotation)
	{
		DebugQuad d(pos,scale,rotation);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}


	virtual void  debugPoint(const physx::PxVec3 &pos,const physx::PxVec3 &radius)
	{
		DebugPointScale d(pos,radius);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}


	virtual void debugGraph(uint32_t /*numPoints*/, float * /*points*/, float /*graphMax*/, float /*graphXPos*/, float /*graphYPos*/, float /*graphWidth*/, float /*graphHeight*/, uint32_t /*colorSwitchIndex*/ = 0xFFFFFFFF)
	{
		PX_ALWAYS_ASSERT();
	}


	virtual void debugRect2d(float x1,float y1,float x2,float y2)
	{
		DebugRect2d d(x1,y1,x2,y2);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugGradientLine(const physx::PxVec3 &p1,const physx::PxVec3 &p2,const uint32_t &c1,const uint32_t &c2)
	{
		DebugGradientLine d(p1,p2,c1,c2);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugRay(const physx::PxVec3 &p1,const physx::PxVec3 &p2)
	{
		DebugRay d(p1,p2);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugCylinder(const physx::PxVec3 &p1,const physx::PxVec3 &p2,float radius)
	{
		DebugPointCylinder d(p1,p2,radius);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugThickRay(const physx::PxVec3 &p1,const physx::PxVec3 &p2,float raySize=0.02f, bool arrowTip=true)
	{
		DebugThickRay d(p1,p2,raySize,arrowTip);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugPlane(const physx::PxPlane &p,float radius1,float radius2)
	{
		DebugPlane d(p.n,p.d,radius1,radius2);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugTri(const physx::PxVec3 &p1,const physx::PxVec3 &p2,const physx::PxVec3 &p3)
	{
		DebugTri d(p1,p2,p3);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugTriNormals(const physx::PxVec3 &p1,const physx::PxVec3 &p2,const physx::PxVec3 &p3,const physx::PxVec3 &n1,const physx::PxVec3 &n2,const physx::PxVec3 &n3)
	{
		DebugTriNormals d(p1,p2,p3,n1,n2,n3);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugGradientTri(const physx::PxVec3 &p1,const physx::PxVec3 &p2,const physx::PxVec3 &p3,const uint32_t &c1,const uint32_t &c2,const uint32_t &c3)
	{
		DebugGradientTri d(p1,p2,p3,c1,c2,c3);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugGradientTriNormals(const physx::PxVec3 &p1,const physx::PxVec3 &p2,const physx::PxVec3 &p3,const physx::PxVec3 &n1,const physx::PxVec3 &n2,const physx::PxVec3 &n3,const uint32_t &c1,const uint32_t &c2,const uint32_t &c3)
	{
		DebugGradientTriNormals d(p1,p2,p3,n1,n2,n3,c1,c2,c3);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugSphere(const physx::PxVec3 &pos, float radius,uint32_t subdivision)
	{
		if ( subdivision < 1 )
		{
			subdivision = 1;
		}
		else if ( subdivision > 4 )
		{
			subdivision = 4;
		}
		physx::PxMat44 pose = physx::PxMat44(physx::PxIdentity);
		pose.setPosition(pos);
		debugOrientedSphere(radius, subdivision, pose);
	}

	void  debugOrientedSphere(float radius, uint32_t subdivision,const physx::PxMat44 &transform)
	{
		pushRenderState();
		if ( mCurrentState.mPose )
		{
			physx::PxMat44 xform = *mCurrentState.mPose*transform;
			setPose(xform);
		}
		else
		{
			setPose(transform);
		}

		DebugSphere d(radius, subdivision);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);

		popRenderState();
	}

	// a squashed sphere
	virtual void  debugOrientedSphere(const physx::PxVec3 &radius, uint32_t subdivision,const physx::PxMat44 &transform)
	{
		pushRenderState();
		if ( mCurrentState.mPose )
		{
			physx::PxMat44 xform = *mCurrentState.mPose*transform;
			setPose(xform);
		}
		else
		{
			setPose(transform);
		}

		DebugSquashedSphere d(radius, subdivision);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);

		popRenderState();
	}


	virtual void  debugCapsule(float radius,float height,uint32_t subdivision)
	{
		DebugCapsule d(radius,height,subdivision);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugCapsuleTapered(float radius1, float radius2, float height, uint32_t subdivision)
	{
		DebugTaperedCapsule d(radius1, radius2, height, subdivision);
		PostRenderDebug::postRenderDebug(&d, mCurrentState);
	}

	virtual void  debugCylinder(float radius,float height,bool closeSides,uint32_t subdivision)
	{
		DebugCylinder d(radius,height,subdivision,closeSides);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	virtual void  debugCircle(const physx::PxVec3 &center,float radius,uint32_t subdivision)
	{
		pushRenderState();
		physx::PxMat44 transform(physx::PxIdentity);
		transform.setPosition(center);
		if ( mCurrentState.mPose )
		{
			physx::PxMat44 xform = *mCurrentState.mPose*transform;
			setPose(xform);
		}
		else
		{
			setPose(transform);
		}
		DebugCircle d(radius,subdivision,false);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
		popRenderState();
	}

	virtual void  debugAxes(const physx::PxMat44 &transform,float distance=0.1f,float brightness=1.0f,bool showXYZ=false,bool showRotation=false, uint32_t axisSwitch=4, DebugAxesRenderMode::Enum renderMode = DebugAxesRenderMode::DEBUG_AXES_RENDER_SOLID)
	{
		DebugAxes d(transform,distance,brightness,showXYZ,showRotation,axisSwitch, renderMode);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

    virtual void debugArc(const physx::PxVec3 &center,const physx::PxVec3 &p1,const physx::PxVec3 &p2,float arrowSize=0.1f,bool showRoot=false)
    {
		DebugArc d(center,p1,p2,arrowSize,showRoot);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
    }

    virtual void debugThickArc(const physx::PxVec3 &center,const physx::PxVec3 &p1,const physx::PxVec3 &p2,float thickness=0.02f,bool showRoot=false)
    {
		DebugThickArc d(center,p1,p2,thickness,showRoot);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
    }

	virtual void  debugDetailedSphere(const physx::PxVec3 &pos,float radius,uint32_t stepCount)
	{
		DebugDetailedSphere d(pos,radius,stepCount);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	/**
	\brief Debug visualize a text string rendered using a simple 2d font.  

	\param x The X position of the text in normalized screen space 0 is the left of the screen and 1 is the right of the screen.
	\param y The Y position of the text in normalized screen space 0 is the top of the screen and 1 is the bottom of the screen.
	\param scale The scale of the font; these are not true-type fonts, so this is simple sprite scales.  A scale of 0.5 is a nice default value.
	\param shadowOffset The font is displayed with a drop shadow to make it easier to distinguish against the background; a value between 1 to 6 looks ok.
	\param forceFixWidthNumbers This bool controls whether numeric values are printed as fixed width or not.
	\param textColor the 32 bit ARGB value color to use for this piece of text.  
	\param fmt A printf style format string followed by optional arguments
	*/
	virtual void debugText2D(float	x,
							float	y,
							float	scale,
							float	shadowOffset,
							bool	forceFixWidthNumbers,
							uint32_t textColor,
							const char *fmt,...)
	{
		char wbuff[16384];
		wbuff[16383] = 0;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(wbuff,sizeof(wbuff)-1, fmt, arg);
		va_end(arg);
		DebugText2D dt(physx::PxVec2(x,y),scale,shadowOffset,forceFixWidthNumbers,textColor,wbuff);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}


    virtual void debugText(const physx::PxVec3 &pos,const char *fmt,...)
    {
		char wbuff[16384];
		wbuff[16383] = 0;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(wbuff,sizeof(wbuff)-1, fmt, arg);
		va_end(arg);
		physx::PxMat44 pose = mCurrentState.mPose ? *mCurrentState.mPose : physx::PxMat44(physx::PxIdentity);
		DebugText dt(pos,pose,wbuff);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
    }

	virtual void setViewMatrix(const physx::PxMat44 &view)
	{
		memcpy( mViewMatrix44, &view, 16*sizeof( float ) );
		updateVP();
		if ( mProcessRenderDebug )
		{
			const physx::PxMat44 *view44 = (reinterpret_cast<const physx::PxMat44 *>(mViewMatrix44));
			mProcessRenderDebug->setViewMatrix(*view44);
		}
	}

	virtual void setProjectionMatrix(const physx::PxMat44 &projection)
	{
		memcpy( mProjectionMatrix44, &projection, 16*sizeof( float ) );
		updateVP();
	}

	void updateVP(void)
	{
		float* e = mViewProjectionMatrix44;
		for( int c = 0; c < 4; ++c )
		{
			for( int r = 0; r < 4; ++r, ++e )
			{
				float sum = 0;
				for( int k = 0; k < 4; ++k )
				{
					sum += mProjectionMatrix44[r+(k<<2)]*mViewMatrix44[k+(c<<2)];
				}
				*e = sum;
			}
		}
		// grab the world-space eye position.
 		const physx::PxMat44 *view44 = (reinterpret_cast<const physx::PxMat44 *>(mViewMatrix44));
		physx::PxMat44 viewInverse = view44->inverseRT();
		mEyePos = viewInverse.transform( physx::PxVec3( 0.0f, 0.0f, 0.0f ));
	}

	virtual const float* getViewProjectionMatrix(void) const
	{
		return mViewProjectionMatrix44;
	}

	virtual const physx::PxMat44* getViewProjectionMatrixTyped(void) const
	{
		return (reinterpret_cast<const physx::PxMat44 *>(mViewProjectionMatrix44));
	}


	virtual const float *getViewMatrix(void) const
	{
		return mViewMatrix44;			// returns column major array
	}

	virtual const physx::PxMat44 *getViewMatrixTyped(void) const
	{
		return (reinterpret_cast<const physx::PxMat44 *>(mViewMatrix44));			// returns column major array
	}


	virtual const float *getProjectionMatrix(void) const
	{
		return mProjectionMatrix44;		// returns column major array
	}

	virtual const physx::PxMat44 *getProjectionMatrixTyped(void) const
	{
		return (reinterpret_cast<const physx::PxMat44 *>(mProjectionMatrix44));		// returns column major array
	}


	// quat to rotate v0 t0 v1
	PX_INLINE physx::PxQuat rotationArc(const physx::PxVec3& v0, const physx::PxVec3& v1)
	{
		const physx::PxVec3 cross = v0.cross(v1);
		const float d = v0.dot(v1);
		if(d<=-0.99999f)
			return (physx::PxAbs(v0.x)<0.1f ? physx::PxQuat(0.0f, v0.z, -v0.y, 0.0f) : physx::PxQuat(v0.y, -v0.x, 0.0, 0.0)).getNormalized();

		const float s = physx::PxSqrt((1+d)*2), r = 1/s;

		return physx::PxQuat(cross.x*r, cross.y*r, cross.z*r, s*0.5f).getNormalized();
	}


	/**
	\brief A convenience method to convert a position and direction vector into a 4x4 transform

	\param p0 Start position
	\param p1 Rotate to position
	\param xform Output transform

	*/
	virtual void rotationArc(const physx::PxVec3 &p0,const physx::PxVec3 &p1,physx::PxMat44 &xform) 
	{
		physx::PxVec3 dir = p1-p0;
		dir.normalize();

		physx::PxTransform t;
		t.p = p0;
		t.q = rotationArc(physx::PxVec3(0,1.0f,0),dir);
		xform = physx::PxMat44(t);
	}

	/**
	\brief A convenience method to convert a position and direction vector into a 4x4 transform

	\param p0 The origin
	\param p1 Rotate to position
	\param xform Ouput transform

	*/
	virtual void rotationArc(const float p0[3],const float p1[3],float xform[16]) 
	{
		rotationArc(*(reinterpret_cast<const physx::PxVec3 *>(p0)),*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<physx::PxMat44 *>(xform)));
	}

	virtual void  eulerToQuat(const physx::PxVec3 &angles, physx::PxQuat &q)  // angles are in degrees.
	{
		float roll  = angles.x*0.5f*FM_DEG_TO_RAD;
		float pitch = angles.y*0.5f*FM_DEG_TO_RAD;
		float yaw   = angles.z*0.5f*FM_DEG_TO_RAD;

		float cr = cosf(roll);
		float cp = cosf(pitch);
		float cy = cosf(yaw);

		float sr = sinf(roll);
		float sp = sinf(pitch);
		float sy = sinf(yaw);

		float cpcy = cp * cy;
		float spsy = sp * sy;
		float spcy = sp * cy;
		float cpsy = cp * sy;

		float x   = ( sr * cpcy - cr * spsy);
		float y   = ( cr * spcy + sr * cpsy);
		float z   = ( cr * cpsy - sr * spcy);
		float w   = cr * cpcy + sr * spsy;
		q = physx::PxQuat(x,y,z,w);
	}

	virtual int32_t beginDrawGroup(const physx::PxMat44 &pose)
	{
		pushRenderState();

		setRenderState(RENDER_DEBUG::DebugRenderState::InfiniteLifeSpan,0xFFFFFF,0.0001f,0xFF0000,0.1f,mCurrentState.mRenderScale,mCurrentState.mTextScale);

		mBlockIndex++;
		mCurrentState.mBlockInfo = mBlocks.construct();
		mCurrentState.mBlockInfo->mHashValue = mBlockIndex;
		mCurrentState.mBlockInfo->mPose = pose;
		mBlocksHash[mBlockIndex] =  mCurrentState.mBlockInfo;

		return int32_t(mBlockIndex);
	}

	virtual void  setDrawGroupPose(int32_t blockId,const physx::PxMat44 &pose)
	{
		const physx::shdfnd::HashMap<uint32_t, BlockInfo *>::Entry *e = mBlocksHash.find(uint32_t(blockId));
		if ( e )
		{
			BlockInfo *b = e->second;
			if ( memcmp(&pose,&b->mPose,sizeof(physx::PxMat44)) != 0 ) // if the pose has changed...
			{
				b->mPose = pose;
				b->mChanged = true;
			}
		}
	}

	/**
	\brief Sets the global pose for the current debug-rendering context. This is preserved on the state stack.

	\param pos Sets the translation component
	\param quat Sets the rotation component as a quat
	*/
	virtual void setPose(const float pos[3],const float quat[4]) 
	{
		physx::PxTransform t(*(reinterpret_cast<const physx::PxVec3 *>(pos)),*(reinterpret_cast<const physx::PxQuat *>(quat)));
		physx::PxMat44 xform(t);
		setPose(xform.front());
	}

	/**
	\brief Sets the global pose position only, does not change the rotation.

	\param pos Sets the translation component
	*/
	virtual void setPosition(const float pos[3]) 
	{
		setPosition( *(reinterpret_cast<const physx::PxVec3 *>(pos)));
	}

	/**
	\brief Sets the global pose orientation only, does not change the position

	\param quat Sets the orientation of the global pose
	*/
	virtual void setOrientation(const float quat[3]) 
	{
		setOrientation( *(reinterpret_cast<const physx::PxQuat *>(quat)));
	}

	/**
	\brief Sets the global pose back to identity
	*/
	virtual void setIdentityPose(void) 
	{
		mCurrentState.mPose = NULL;
		mCurrentState.mCurrentPose = physx::PxMat44(physx::PxIdentity);
		mCurrentState.incrementChangeCount();
	}

	/**
	\brief Sets the global pose for the current debug-rendering context. This is preserved on the state stack.

	\param pose Sets the pose from a position and quaternion rotation
	*/
	virtual void setPose(const physx::PxTransform &pose) 
	{
		physx::PxMat44 m(pose);
		setPose(m);
	}

	/**
	\brief Sets the global pose position only, does not change the rotation.

	\param position Sets the translation component
	*/
	virtual void setPosition(const physx::PxVec3 &position) 
	{
		physx::PxMat44 m = mCurrentState.mCurrentPose;
		m.setPosition(position);
		setPose(m);
	}

	/**
	\brief Sets the global pose orientation only, does not change the position

	\param rot Sets the orientation of the global pose
	*/
	virtual void setOrientation(const physx::PxQuat &rot) 
	{
		physx::PxMat44 m(rot);
		m.setPosition(mCurrentState.mCurrentPose.getPosition());
		setPose(m);
	}


	virtual void  setPose(const physx::PxMat44 &pose)
	{
		physx::PxMat44 id = physx::PxMat44(physx::PxIdentity);
		if ( id.column0 != pose.column0 ||
			 id.column1 != pose.column1 ||
			 id.column2 != pose.column2 ||
			 id.column3 != pose.column3 )
		{
			mCurrentState.mCurrentPose = pose;
			mCurrentState.mPose 	   = &mCurrentState.mCurrentPose;
		}
		else
		{
			mCurrentState.mPose = NULL;
			mCurrentState.mCurrentPose = physx::PxMat44(physx::PxIdentity);
		}
		mCurrentState.incrementChangeCount();
	}

	virtual const physx::PxMat44 * getPoseTyped(void) const
	{
		return &mCurrentState.mCurrentPose;
	}

	virtual const float *getPose(void) const
	{
		return reinterpret_cast<const float *>(&mCurrentState.mCurrentPose);
	}


	/* \brief Create an createDebugGraphDesc.  This is the manual way of setting up a graph.  Every parameter can
	and must be customized when using this constructor.
	*/
	virtual DebugGraphDesc* createDebugGraphDesc(void)
	{
		DebugGraphDesc* dGDPtr = static_cast<DebugGraphDesc *>(PX_ALLOC(sizeof(DebugGraphDesc), PX_DEBUG_EXP("DebugGraphDesc")));
		new  (dGDPtr ) DebugGraphDesc;

		dGDPtr->mNumPoints = 0;
		dGDPtr->mPoints = NULL;
		dGDPtr->mGraphXLabel = NULL;
		dGDPtr->mGraphYLabel = NULL;
		dGDPtr->mGraphColor = 0x00FF0000;
		dGDPtr->mArrowColor = 0x0000FF00;
		dGDPtr->mColorSwitchIndex = uint32_t(-1);
		dGDPtr->mGraphMax		= 0.0f;
		dGDPtr->mGraphXPos		= 0.0f;
		dGDPtr->mGraphYPos		= 0.0f;
		dGDPtr->mGraphWidth		= 0.0f;
		dGDPtr->mGraphHeight	= 0.0f;
		dGDPtr->mCutOffLevel	= 0.0f;
		return(dGDPtr);
	}

	virtual void releaseDebugGraphDesc(DebugGraphDesc *desc) 
	{
		PX_FREE(desc);
	}

	/**
	\brief Create an createDebugGraphDesc using the minimal amount of work.  This constructor provides for six custom
	graphs to be simultaneously drawn on the display at one time numbered 0 to 5.  The position, color, and size
	of the graphs are automatically set based on the graphNum argument.
	*/
	#define LEFT_X			(-0.9f)
	#define RIGHT_X			(+0.1f)
	#define TOP_Y			(+0.4f)
	#define MID_Y			(-0.2f)
	#define BOTTOM_Y		(-0.8f)
	virtual DebugGraphDesc* createDebugGraphDesc(uint32_t graphNum,uint32_t dataCount,const float *dataArray, float maxY, char* xLabel, char* yLabel)
	{
		static struct
		{
			float xPos, yPos;
		} graphData[MAX_GRAPHS] =
		{
			{LEFT_X,  TOP_Y},
			{LEFT_X,  MID_Y},
			{LEFT_X,  BOTTOM_Y},
			{RIGHT_X, TOP_Y},
			{RIGHT_X, MID_Y},
			{RIGHT_X, BOTTOM_Y}
		};
		PX_ASSERT(graphNum < MAX_GRAPHS);
		DebugGraphDesc* dGDPtr	= createDebugGraphDesc();

		dGDPtr->mGraphColor		= 0x00FF0000;
		dGDPtr->mArrowColor		= 0x0000FF00;
		dGDPtr->mColorSwitchIndex = uint32_t(-1);

		// no cut off line by default
		dGDPtr->mCutOffLevel	= 0.0f;
		dGDPtr->mNumPoints		= dataCount;
		dGDPtr->mPoints			= dataArray;
		dGDPtr->mGraphMax		= maxY;
		dGDPtr->mGraphXLabel	= xLabel;
		dGDPtr->mGraphYLabel	= yLabel;

		dGDPtr->mGraphXPos		= graphData[graphNum].xPos;
		dGDPtr->mGraphYPos		= graphData[graphNum].yPos;;
		dGDPtr->mGraphWidth		= GRAPH_WIDTH_DEFAULT;
		dGDPtr->mGraphHeight	= GRAPH_HEIGHT_DEFAULT;

		return(dGDPtr);
	}

	virtual void debugGraph(const DebugGraphDesc& graphDesc)
	{
		DebugGraphStream d(graphDesc);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	/**
	\brief Set a debug color value by name.
	*/
	virtual void setDebugColor(RENDER_DEBUG::DebugColors::Enum colorEnum, uint32_t value)
	{
		if( colorEnum < RENDER_DEBUG::DebugColors::NUM_COLORS )
		{
			colors[ colorEnum ] = value;
		}
	}

	/**
	\brief Return a debug color value by name.
	*/
	virtual uint32_t getDebugColor(RENDER_DEBUG::DebugColors::Enum colorEnum) const
	{
		if( colorEnum < RENDER_DEBUG::DebugColors::NUM_COLORS )
		{
			return colors[ colorEnum ];
		}
		return 0;
	}

	/**
	\brief Return a debug color value by RGB inputs
	*/
	virtual uint32_t getDebugColor(float red, float green, float blue) const
	{
		union
		{
			uint8_t colorChars[4];
			uint32_t color;
		};

		colorChars[3] = 0xff; // alpha
		colorChars[2] = uint8_t(red * 255);
		colorChars[1] = uint8_t(green * 255);
		colorChars[0] = uint8_t(blue * 255);

		return color;
	}

	virtual void debugMessage(const char *fmt,...)
	{
		mCanSkip = false;
		char wbuff[16384];
		wbuff[16383] = 0;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(wbuff,sizeof(wbuff)-1, fmt, arg);
		va_end(arg);
		DebugMessage dt(wbuff);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief Render a set of instanced triangle meshes.
	*/
	virtual void renderTriangleMeshInstances(uint32_t meshId,		// The ID of the previously created triangle mesh
											 uint32_t instanceCount,	// The number of instances to render
											 const RENDER_DEBUG::RenderDebugInstance *instances)	// The array of poses for each instance
	{
		DebugRenderTriangleMeshInstances dt(meshId,instanceCount,instances);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

		
	/**
	\brief Create a debug texture based on a filename

	\param id The id associated with this custom texture, must be greater than 28; the reserved ids for detail textures
	\param fname The name of the DDS file associated with this texture.
	*/
	virtual void createCustomTexture(uint32_t id,const char *fname)
	{
		mCanSkip = false;
		DebugCreateCustomTexture dt(id,fname);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}


	/**
	\brief Create a triangle mesh that we can render.  Assumes an indexed triangle mesh.  User provides a *unique* id.  If it is not unique, this will fail.
	*/
	virtual void createTriangleMesh(uint32_t meshId,	// The unique mesh ID
									uint32_t vcount,	// The number of vertices in the triangle mesh
									const RENDER_DEBUG::RenderDebugMeshVertex *meshVertices,	// The array of vertices
									uint32_t tcount,	// The number of triangles (indices must contain tcount*3 values)
									const uint32_t *indices)	// The array of triangle mesh indices
	{
		mCanSkip = false;
		DebugCreateTriangleMesh dt(meshId,vcount,meshVertices,tcount,indices);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief Refresh a sub-section of the vertices in a previously created triangle mesh.

	\param vcount This is the number of vertices to refresh.
	\param refreshVertices This is an array of revised vertex data.
	\param refreshIndices This is an array of indices which correspond to the original triangle mesh submitted.  There should be one index for each vertex.
	*/
	virtual void refreshTriangleMeshVertices(uint32_t meshId,
											uint32_t vcount,
											const RenderDebugMeshVertex *refreshVertices,
											const uint32_t *refreshIndices) 
	{
		if ( vcount && refreshVertices && refreshIndices )
		{
			DebugRefreshTriangleMeshVertices dt(meshId,vcount,refreshVertices,refreshIndices);
			PostRenderDebug::postRenderDebug(&dt,mCurrentState);
		}
	}

	/**
	\brief Release a previously created triangle mesh
	*/
	virtual void releaseTriangleMesh(uint32_t meshId) 
	{
		mCanSkip = false;
		DebugReleaseTriangleMesh d(meshId);
		PostRenderDebug::postRenderDebug(&d,mCurrentState);
	}

	/**
	\brief Render a set of data points, either as wire-frame cross hairs or as a small solid instanced mesh.

	This callback provides the ability to (as rapidly as possible) render a large number of 

	\param mode Determines what mode to render the point data
	\param meshId The ID of the previously created triangle mesh if rendering in mesh mode
	\param textureId1 The ID of the primary texture
	\param textureTile1 The UV tiling rate of the primary texture
	\param textureId2 The ID of the secondary texture
	\param textureTile2 The UV tiling rate of the secondary texture
	\param pointCount The number of points to render
	\param points The array of points to render
	*/
	virtual void debugPoints(PointRenderMode mode,
							uint32_t meshId,
							uint32_t textureId1,
							float textureTile1,
							uint32_t textureId2,
							float textureTile2,
							uint32_t	pointCount,
							const float *points)
	{
		DebugRenderPoints dt(meshId,mode,textureId1,textureTile1,textureId2,textureTile2,pointCount,points);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief This method will produce a debug visualization of a convex hull; either as lines or solid triang;es depending on the current debug render state

	\param planeCount The number of planes in the convex hull
	\param planes The array of plane equations in the convex hull
	*/
	virtual void debugConvexHull(uint32_t planeCount,
								const float *planes)
	{
		DebugConvexHull dt(planeCount,planes);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief Implement this method to display lines output from the RenderDebug library

	\param lcount The number of lines to draw (vertex count=lines*2)
	\param vertices The pairs of vertices for each line segment 
	\param useZ Whether or not these should be zbuffered
	\param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
	*/
	virtual void debugRenderLines(uint32_t lcount,		
								const RenderDebugVertex *vertices, 
								bool useZ,				
								bool isScreenSpace) 
	{
		DebugRenderLines dt(lcount,vertices,useZ ? 1U : 0U,isScreenSpace ? 1U : 0U);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief Implement this method to display solid shaded triangles without any texture surce.

	\param tcount The number of triangles to render. (vertex count=tcount*2)
	\param vertices The vertices for each triangle
	\param useZ Whether or not these should be zbuffered
	\param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
	*/
	virtual void debugRenderTriangles(uint32_t tcount,
								const RenderDebugSolidVertex *vertices,	
								bool useZ,				
								bool isScreenSpace)
	{
		DebugRenderTriangles dt(tcount,vertices,useZ ? 1U : 0U,isScreenSpace ? 1U : 0U);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

private:

	void initColors()
	{
#define INIT_COLOR( name, defVal, minVal, maxVal )	\
	colorsDefValue[ name ] = uint32_t(defVal);		\
	colorsMinValue[ name ] = uint32_t(minVal);		\
	colorsMaxValue[ name ] = uint32_t(maxVal);

		INIT_COLOR(RENDER_DEBUG::DebugColors::Default,		0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::PoseArrows,		0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::MeshStatic,		0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::MeshDynamic,	0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Shape,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Text0,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Text1,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::ForceArrowsLow,	0xffffff00,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::ForceArrowsNorm,0xff00ff00,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::ForceArrowsHigh,0xffff0000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color0,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color1,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color2,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color3,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color4,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Color5,			0x00000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Red,			0xffff0000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Green,			0xff00ff00,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Blue,			0xff0000ff,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::DarkRed,		0xff800000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::DarkGreen,		0xff008000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::DarkBlue,		0xff000080,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::LightRed,		0xffff8080,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::LightGreen,		0xff80ff00,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::LightBlue,		0xff00ffff,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Purple,			0xffff00ff,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::DarkPurple,		0xff800080,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Yellow,			0xffffff00,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Orange,			0xffff8000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Gold,			0xff808000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Emerald,		0xff008080,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::White,			0xffffffff,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Black,			0xff000000,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::Gray,			0xff808080,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::LightGray,		0xffC0C0C0,		0,		UINT32_MAX);
		INIT_COLOR(RENDER_DEBUG::DebugColors::DarkGray,		0xff404040,		0,		UINT32_MAX);
#undef INIT_COLOR

		memcpy( colors, colorsDefValue, sizeof(uint32_t) * RENDER_DEBUG::DebugColors::NUM_COLORS );
	}

	void releaseRenderDebug(void)
	{
		delete this;
	}


//*****************************************************
//**** Non-typed version of methods
//*****************************************************

	virtual void  debugPolygon(uint32_t pcount,const float *points) 
	{
		debugPolygon(pcount,(reinterpret_cast<const physx::PxVec3 *>(points)));
	}

	virtual void  debugLine(const float p1[3],const float p2[3]) 
	{
		debugLine( *(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)));
	}

	virtual void  debugGradientLine(const float p1[3],const float p2[3],const uint32_t &c1,const uint32_t &c2) 
	{
		debugGradientLine(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),c1,c2);
	}

	virtual void  debugRay(const float p1[3],const float p2[3]) 
	{
		debugRay(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)));
	}

	virtual void  debugCylinder(const float p1[3],const float p2[3],float radius) 
	{
		debugCylinder(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),radius);
	}

	virtual void  debugThickRay(const float p1[3],const float p2[3],float raySize=0.02f,bool includeArrow=true) 
	{
		debugThickRay(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),raySize,includeArrow);
	}

	virtual void  debugPlane(const float normal[3],float dCoff,float radius1,float radius2) 
	{
		physx::PxPlane p(normal[0],normal[1],normal[2],dCoff);
		debugPlane(p,radius1,radius2);
	}

	virtual void  debugTri(const float p1[3],const float p2[3],const float p3[3]) 
	{
		debugTri(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),*(reinterpret_cast<const physx::PxVec3 *>(p3)));
	}

	virtual void  debugTriNormals(const float p1[3],const float p2[3],const float p3[3],const float n1[3],const float n2[3],const float n3[3]) 
	{
		debugTriNormals(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),*(reinterpret_cast<const physx::PxVec3 *>(p3)),*(reinterpret_cast<const physx::PxVec3 *>(n1)),*(reinterpret_cast<const physx::PxVec3 *>(n2)),*(reinterpret_cast<const physx::PxVec3 *>(n3)));
	}

	virtual void  debugGradientTri(const float p1[3],const float p2[3],const float p3[3],const uint32_t &c1,const uint32_t &c2,const uint32_t &c3) 
	{
		debugGradientTri(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),*(reinterpret_cast<const physx::PxVec3 *>(p3)),c1,c2,c3);
	}

	virtual void  debugGradientTriNormals(const float p1[3],const float p2[3],const float p3[3],const float n1[3],const float n2[3],const float n3[3],const uint32_t &c1,const uint32_t &c2,const uint32_t &c3) 
	{
		debugGradientTriNormals(*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),*(reinterpret_cast<const physx::PxVec3 *>(p3)),*(reinterpret_cast<const physx::PxVec3 *>(n1)),*(reinterpret_cast<const physx::PxVec3 *>(n2)),*(reinterpret_cast<const physx::PxVec3 *>(n3)),c1,c2,c3);
	}

	virtual void  debugBound(const float bmin[3],const float bmax[3]) 
	{
		physx::PxBounds3 b(*(reinterpret_cast<const physx::PxVec3 *>(bmin)),*(reinterpret_cast<const physx::PxVec3 *>(bmax)));
		debugBound(b);
	}

	virtual void  debugSphere(const float pos[3],float radius,uint32_t subdivision) 
	{
		debugSphere(*(reinterpret_cast<const physx::PxVec3 *>(pos)),radius,subdivision);
	}


	virtual void  debugCircle(const float center[3],float radius,uint32_t subdivision) 
	{
		debugCircle(*(reinterpret_cast<const physx::PxVec3 *>(center)),radius,subdivision);
	}

	virtual void  debugPoint(const float pos[3],float radius) 
	{
		debugPoint(*(reinterpret_cast<const physx::PxVec3 *>(pos)),radius);
	}

	virtual void  debugPoint(const float pos[3],const float scale[3]) 
	{
		debugPoint(*(reinterpret_cast<const physx::PxVec3 *>(pos)),*(reinterpret_cast<const physx::PxVec3 *>(scale)));
	}

	virtual void  debugQuad(const float pos[3],const float scale[2],float orientation) 
	{
		debugQuad(*(reinterpret_cast<const physx::PxVec3 *>(pos)),*reinterpret_cast<const physx::PxVec2 *>(scale),orientation);
	}

	virtual void  debugAxes(const float transform[16],float distance=0.1f,float brightness=1.0f,bool showXYZ=false,bool showRotation=false,uint32_t axisSwitch=4, DebugAxesRenderMode::Enum renderMode = DebugAxesRenderMode::DEBUG_AXES_RENDER_SOLID) 
	{
		debugAxes(*(reinterpret_cast<const physx::PxMat44 *>(transform)),distance,brightness,showXYZ,showRotation,axisSwitch,renderMode);
	}

	virtual void debugArc(const float center[3],const float p1[3],const float p2[3],float arrowSize=0.1f,bool showRoot=false) 
	{
		debugArc(*(reinterpret_cast<const physx::PxVec3 *>(center)),*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),arrowSize,showRoot);
	}

	virtual void debugThickArc(const float center[3],const float p1[3],const float p2[3],float thickness=0.02f,bool showRoot=false) 
	{
		debugThickArc(*(reinterpret_cast<const physx::PxVec3 *>(center)),*(reinterpret_cast<const physx::PxVec3 *>(p1)),*(reinterpret_cast<const physx::PxVec3 *>(p2)),thickness,showRoot);
	}

	virtual void debugText(const float pos[3],const char *fmt,...) 
	{
		char wbuff[16384];
		wbuff[16383] = 0;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(wbuff,sizeof(wbuff)-1, fmt, arg);
		va_end(arg);
		debugText(*(reinterpret_cast<const physx::PxVec3 *>(pos)),"%s",wbuff);
	}

	virtual void setViewMatrix(const float view[16]) 
	{
		setViewMatrix(*(reinterpret_cast<const physx::PxMat44 *>(view)));
	}

	virtual void setProjectionMatrix(const float projection[16]) 
	{
		setProjectionMatrix(*(reinterpret_cast<const physx::PxMat44 *>(projection)));
	}

	virtual void  eulerToQuat(const float angles[3],float q[4])  // angles are in degrees.
	{
		eulerToQuat(*(reinterpret_cast<const physx::PxVec3 *>(angles)),*(reinterpret_cast<physx::PxQuat *>(q)));
	}

	virtual int32_t beginDrawGroup(const float pose[16]) 
	{
		return beginDrawGroup(*(reinterpret_cast<const physx::PxMat44 *>(pose)));
	}

	virtual void  setDrawGroupPose(int32_t blockId,const float pose[16]) 
	{
		setDrawGroupPose(blockId,*(reinterpret_cast<const physx::PxMat44 *>(pose)));
	}

	virtual void  debugDetailedSphere(const float pos[3],float radius,uint32_t stepCount) 
	{
		debugDetailedSphere(*(reinterpret_cast<const physx::PxVec3 *>(pos)),radius,stepCount);
	}

	virtual void  setPose(const float pose[16]) 
	{
		setPose(*(reinterpret_cast<const physx::PxMat44 *>(pose)));
	}

	virtual void debugFrustum(const float view[16],const float proj[16]) 
	{
		debugFrustum( *(reinterpret_cast<const physx::PxMat44 *>(view)),*(reinterpret_cast<const physx::PxMat44 *>(proj)));
	}


//*****************************************************
// ** Methods which get hooked back through the main parent interface.  
	/**
	\brief Begins a file-playback session. Returns the number of recorded frames in the recording file.  Zero if the file was not valid.
	*/
	virtual uint32_t setFilePlayback(const char *fileName)
	{
		return mRenderDebugHook->setFilePlayback(fileName);
	}

	/**
	\brief Set's the file playback to a specific frame.  Returns true if successful.
	*/
	virtual bool setPlaybackFrame(uint32_t playbackFrame)
	{
		return mRenderDebugHook->setPlaybackFrame(playbackFrame);
	}

	/**
	\brief Returns the number of recorded frames in the debug render recording file.
	*/
	virtual uint32_t getPlaybackFrameCount(void) const 
	{
		return mRenderDebugHook->getPlaybackFrameCount();
	}

	/**
	\brief Stops the current recording playback.
	*/
	virtual void stopPlayback(void) 
	{
		mRenderDebugHook->stopPlayback();
	}

	/**
	\brief Do a 'try' lock on the global render debug mutex.  This is simply provided as an optional convenience if you are accessing RenderDebug from multiple threads and want to prevent contention.
	*/
	virtual bool trylock(void)
	{
		return mRenderDebugHook->trylock();
	}

	/**
	\brief Lock the global render-debug mutex to avoid thread contention.
	*/
	virtual void lock(void) 
	{
		mRenderDebugHook->lock();
	}

	/**
	\brief Unlock the global render-debug mutex
	*/
	virtual void unlock(void) 
	{
		mRenderDebugHook->unlock();
	}

	/**
	\brief Convenience method to return a unique mesh id number (simply a global counter to avoid clashing with other ids
	*/
	virtual uint32_t getMeshId(void) 
	{
		return mRenderDebugHook->getMeshId();
	}

	/**
	\brief Send a command from the server to the client.  This could be any arbitrary console command, it can also be mouse drag events, debug visualization events, etc.
	* the client receives this command in argc/argv format.
	*/
	virtual bool sendRemoteCommand(const char *fmt,...)
	{
		char buff[16384];
		buff[16383] = 0;
		va_list arg;
		va_start( arg, fmt );
		physx::shdfnd::vsnprintf(buff,sizeof(buff)-1, fmt, arg);
		va_end(arg);
		return mRenderDebugHook->sendRemoteCommand("%s",buff);
	}

	/**
	\brief If running in client mode, poll this method to retrieve any pending commands from the server.  If it returns NULL then there are no more commands.
	*/
	virtual const char ** getRemoteCommand(uint32_t &argc) 
	{
		return mRenderDebugHook->getRemoteCommand(argc);
	}

	/**
	\brief Transmits an arbitrary block of binary data to the remote machine.  The block of data can have a command and id associated with it.

	It is important to note that due to the fact the RenderDebug system is synchronized every single frame, it is strongly recommended
	that you only use this feature for relatively small data items; probably on the order of a few megabytes at most.  If you try to do
	a very large transfer, in theory it would work, but it might take a very long time to complete and look like a hang since it will
	essentially be blocking.

	\param nameSpace An arbitrary command associated with this data transfer, for example this could indicate a remote file request.
	\param resourceName An arbitrary id associated with this data transfer, for example the id could be the file name of a file transfer request.
	\param data The block of binary data to transmit, you are responsible for maintaining endian correctness of the internal data if necessary.
	\param dlen The length of the lock of data to transmit.

	\return Returns true if the data was queued to be transmitted, false if it failed.
	*/
	virtual bool sendRemoteResource(const char *nameSpace,
									const char *resourceName,
									const void *data,
									uint32_t dlen) 
	{
		return mRenderDebugHook->sendRemoteResource(nameSpace,resourceName,data,dlen);
	}


	/**
	\brief Retrieves a block of remotely transmitted binary data.

	\param nameSpace A a reference to a pointer which will store the namespace (type) associated with this data transfer, for example this could indicate a remote file request.
	\param resourceName A reference to a pointer which will store the resource name associated with this data transfer, for example the resource name could be the file name of a file transfer request.
	\param dlen A reference that will contain length of the lock of data received.
	\param remoteIsBigEndian A reference to a boolean which will be set to true if the remote machine that sent this data is big endian format.

	\retrun A pointer to the block of data received.
	*/
	virtual const void * getRemoteResource(const char *&nameSpace,
										const char *&resourceName,
										uint32_t &dlen,
										bool &remoteIsBigEndian)
	{
		return mRenderDebugHook->getRemoteResource(nameSpace,resourceName,dlen,remoteIsBigEndian);
	}

	/**
	\brief This function allows you to request a file from the remote machine by name.  If successful it will be returned via 'getRemoteData'

	\param command The command field associated with this request which will be returned by 'getRemoteData'
	\param fileName The filename being requested from the remote machine.

	\return Returns true if the request was queued to be transmitted, false if it failed.
	*/
	virtual bool requestRemoteResource(const char *command,
									const char *fileName)
	{
		return mRenderDebugHook->requestRemoteResource(command,fileName);
	}

	/**
	\brief Report what 'Run' mode we are operation gin.
	*/
	virtual RenderDebug::RunMode getRunMode(void)
	{
		return mRenderDebugHook->getRunMode();
	}

	/**
	\brief Returns true if we still have a valid connection to the server.
	*/
	virtual bool isConnected(void) const 
	{
		return mRenderDebugHook->isConnected();
	}

	/**
	\brief Returns the current synchronized frame between client/server communications.  Returns zero if no active connection exists.
	*/
	virtual uint32_t getCommunicationsFrame(void) const 
	{
		return mRenderDebugHook->getCommunicationsFrame();
	}

	virtual const char *getRemoteApplicationName(void) 
	{
		return mRenderDebugHook->getRemoteApplicationName();
	}

	/**
	\brief Returns the optional typed methods for various render debug routines.
	*/
	virtual RenderDebugTyped *getRenderDebugTyped(void)
	{
		return mRenderDebugHook->getRenderDebugTyped();
	}

	/**
	\brief Release the render debug class
	*/
	virtual void release(void)
	{
		mRenderDebugHook->release();
	}

	virtual bool render(float dtime,RENDER_DEBUG::RenderDebugInterface *iface) 
	{
		return mRenderDebugHook->render(dtime,iface);
	}

	/**
	\brief register a digital input event (maps to windows keyboard commands)

	\param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
	\param inputId This the pre-defined keyboard code for this input event.
	*/
	virtual void registerDigitalInputEvent(InputEventIds::Enum eventId,InputIds::Enum inputId)
	{
		mCanSkip = false;
		DebugRegisterInputEvent dt(true,eventId,0.0f,inputId);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	virtual void unregisterInputEvent(InputEventIds::Enum eventId)
	{
		mCanSkip = false;
		DebugUnregisterInputEvent dt(eventId);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}


	/**
	\brief register a digital input event (maps to windows keyboard commands)

	\param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
	\param sensitivity The sensitivity value associated with this anaglog devent; default value is 1.0
	\param inputId This the pre-defined analog code for this input event.
	*/
	virtual void registerAnalogInputEvent(InputEventIds::Enum eventId,float sensitivity,InputIds::Enum inputId) 
	{
		mCanSkip = false;
		DebugRegisterInputEvent dt(false,eventId,sensitivity,inputId);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	/**
	\brief Reset all input events to an empty state
	*/
	virtual void resetInputEvents(void) 
	{
		mCanSkip = false;
		DebugPrimitiveU32 dt(DebugCommand::DEBUG_RESET_INPUT_EVENTS,0);
		PostRenderDebug::postRenderDebug(&dt,mCurrentState);
	}

	virtual void sendInputEvent(const InputEvent &ev) 
	{
		if ( mRenderDebugHook )
		{
			mRenderDebugHook->sendInputEvent(ev);
		}
	}

	/**
	\brief Returns any incoming input event for processing purposes
	*/
	virtual const InputEvent *getInputEvent(bool flush)
	{
		const InputEvent *e = NULL;

		if ( mRenderDebugHook )
		{
			e = mRenderDebugHook->getInputEvent(flush);
		}
		return e;
	}

	/**
	\brief Set the base file name to record communications tream; or NULL to disable it.

	\param fileName The base name of the file to record the communications channel stream to, or NULL to disable it.
	*/
	virtual bool setStreamFilename(const char *fileName) 
	{
		bool ret = false;
		if ( mRenderDebugHook )
		{
			ret = mRenderDebugHook->setStreamFilename(fileName);
		}
		return ret;
	}

	/**
	\brief Begin playing back a communications stream recording

	\param fileName The name of the previously captured communications stream file
	*/
	virtual bool setStreamPlayback(const char *fileName) 
	{
		bool ret = false;
		if ( mRenderDebugHook )
		{
			ret = mRenderDebugHook->setStreamPlayback(fileName);
		}
		return ret;
	}

	/**
	\brief Special case command that affects how the server processes the previous frame of data.

	\return Returns true if it is safe to skip this frame of data, false if there are required commands that must be executed.
	*/
	virtual bool	trySkipFrame(void) 
	{
		if ( mCanSkip )
		{
			DebugPrimitiveU32 dt(DebugCommand::DEBUG_SKIP_FRAME,0);
			PostRenderDebug::postRenderDebug(&dt,mCurrentState);
		}
		return mCanSkip;
	}

	bool					mCanSkip;
	RenderDebugHook			*mRenderDebugHook;
	uint32_t				 mUpdateCount;
	ProcessRenderDebug	*mProcessRenderDebug;
	RenderState				mCurrentState;
	uint32_t				mStackIndex;
	RenderState				mRenderStateStack[RENDER_STATE_STACK_SIZE];
	mutable float			mViewMatrix44[16];
	mutable float			mProjectionMatrix44[16];
	mutable float                     mViewProjectionMatrix44[16];
	physx::PxVec3                            mEyePos;
	//Colors for debug rendering purposes
	uint32_t							colors[RENDER_DEBUG::DebugColors::NUM_COLORS];
	uint32_t							colorsMinValue[RENDER_DEBUG::DebugColors::NUM_COLORS];
	uint32_t							colorsMaxValue[RENDER_DEBUG::DebugColors::NUM_COLORS];
	uint32_t							colorsDefValue[RENDER_DEBUG::DebugColors::NUM_COLORS];

	uint32_t							mBlockIndex;
	physx::shdfnd::HashMap<uint32_t, BlockInfo *>		mBlocksHash;
	physx::shdfnd::Pool< BlockInfo >				mBlocks;
	float							mFrameTime;
	bool							mOwnProcess;

};

DebugGraphStream::DebugGraphStream(const DebugGraphDesc &d) : DebugPrimitive(DebugCommand::DEBUG_GRAPH)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << d.mNumPoints;
	stream.storeString(d.mGraphXLabel ? d.mGraphXLabel : "",true);
	stream.storeString(d.mGraphYLabel ? d.mGraphYLabel : "",true);

	mb.alignWrite(4);

	for (uint32_t i=0; i<d.mNumPoints; i++)
	{
		stream << d.mPoints[i];
	}
	stream << d.mCutOffLevel;
	stream << d.mGraphMax;
	stream << d.mGraphXPos;
	stream << d.mGraphYPos;
	stream << d.mGraphWidth;
	stream << d.mGraphHeight;
	stream << d.mGraphColor;
	stream << d.mArrowColor;
	stream << d.mColorSwitchIndex;

	mBuffer = mb.getWriteBufferOwnership(mSize);
	mAllocated = true;
}

DebugGraphStream::~DebugGraphStream(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}


DebugCreateTriangleMesh::DebugCreateTriangleMesh(uint32_t meshId,
	uint32_t vcount,	// The number of vertices in the triangle mesh
	const RENDER_DEBUG::RenderDebugMeshVertex *meshVertices,	// The array of vertices
	uint32_t tcount,	// The number of triangles (indices must contain tcount*3 values)
	const uint32_t *indices) : DebugPrimitive(DebugCommand::DEBUG_CREATE_TRIANGLE_MESH)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << meshId;
	stream << vcount;
	stream << tcount;

	if ( vcount )
	{
		mb.write(meshVertices,sizeof(RENDER_DEBUG::RenderDebugMeshVertex)*vcount);
	}

	if ( tcount )
	{
		mb.write(indices,sizeof(uint32_t)*tcount*3);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugCreateTriangleMesh::~DebugCreateTriangleMesh(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}


DebugRefreshTriangleMeshVertices::DebugRefreshTriangleMeshVertices(uint32_t meshId,
	uint32_t vcount,	// The number of vertices in the triangle mesh
	const RENDER_DEBUG::RenderDebugMeshVertex *meshVertices,	// The array of vertices
	const uint32_t *indices) : DebugPrimitive(DebugCommand::DEBUG_REFRESH_TRIANGLE_MESH_VERTICES)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << meshId;
	stream << vcount;

	if ( vcount )
	{
		mb.write(meshVertices,sizeof(RENDER_DEBUG::RenderDebugMeshVertex)*vcount);
	}

	if ( vcount )
	{
		mb.write(indices,sizeof(uint32_t)*vcount);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugRefreshTriangleMeshVertices::~DebugRefreshTriangleMeshVertices(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}


DebugRenderTriangleMeshInstances::DebugRenderTriangleMeshInstances(uint32_t meshId,
	uint32_t instanceCount,
	const RENDER_DEBUG::RenderDebugInstance *instances) : DebugPrimitive(DebugCommand::DEBUG_RENDER_TRIANGLE_MESH_INSTANCES)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << meshId;
	stream << instanceCount;

	if ( instanceCount )
	{
		mb.write(instances,sizeof(RENDER_DEBUG::RenderDebugInstance)*instanceCount);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugRenderTriangleMeshInstances::~DebugRenderTriangleMeshInstances(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}

DebugConvexHull::DebugConvexHull(uint32_t planeCount,const float *planes) : DebugPrimitive(DebugCommand::DEBUG_CONVEX_HULL)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << planeCount;
	if ( planeCount )
	{
		mb.write(planes,sizeof(float)*4*planeCount);
	}
	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugConvexHull::~DebugConvexHull(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}

DebugRenderPoints::DebugRenderPoints(uint32_t meshId,
	PointRenderMode mode,
	uint32_t textureId1,
	float textureTile1,
	uint32_t textureId2,
	float textureTile2,
	uint32_t	pointCount,
	const float *points) : DebugPrimitive(DebugCommand::DEBUG_RENDER_POINTS)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << meshId;
	stream << uint32_t(mode);
	stream << textureId1;
	stream << textureTile1;
	stream << textureId2;
	stream << textureTile2;
	stream << pointCount;

	if ( pointCount )
	{
		mb.write(points,sizeof(float)*3*pointCount);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugRenderPoints::~DebugRenderPoints(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}

DebugRenderLines::DebugRenderLines(uint32_t lcount,		
	const RenderDebugVertex *vertices, 
	uint32_t useZ,				
	uint32_t isScreenSpace) : DebugPrimitive(DebugCommand::DEBUG_RENDER_LINES)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << lcount;
	stream << useZ;
	stream << isScreenSpace;

	if ( lcount )
	{
		mb.write(vertices,sizeof(RenderDebugVertex)*2*lcount);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t *>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugRenderLines::~DebugRenderLines(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}


DebugRenderTriangles::DebugRenderTriangles(uint32_t tcount,
	const RenderDebugSolidVertex *vertices,	
	uint32_t useZ,				
	uint32_t isScreenSpace) : DebugPrimitive(DebugCommand::DEBUG_RENDER_TRIANGLES)
{
	physx::PsMemoryBuffer mb;
	mb.setEndianMode(physx::PxFileBuf::ENDIAN_NONE);
	nvidia::StreamIO stream(mb,0);

	mSize = 0;
	stream << mCommand;
	stream << mSize;
	stream << tcount;
	stream << useZ;
	stream << isScreenSpace;

	if ( tcount )
	{
		mb.write(vertices,sizeof(RenderDebugSolidVertex)*3*tcount);
	}

	mBuffer = mb.getWriteBufferOwnership(mSize);

	uint32_t *dest = reinterpret_cast<uint32_t*>(mBuffer);
	dest[1] = mSize;

	mAllocated = true;
}

DebugRenderTriangles::~DebugRenderTriangles(void)
{
	if ( mAllocated )
	{
		PX_FREE(mBuffer);
	}
}




//

RenderDebugImpl * createInternalRenderDebug(ProcessRenderDebug *process,RenderDebugHook *renderDebugHook)
{
	InternalRenderDebug *m = PX_NEW(InternalRenderDebug)(process,renderDebugHook);
	return static_cast< RenderDebugImpl *>(m);
}



} // end of namespace
