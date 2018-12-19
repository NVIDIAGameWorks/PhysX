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


#ifndef SCOPED_PHYS_XLOCK_H

#define SCOPED_PHYS_XLOCK_H

#include "foundation/PxPreprocessor.h"
#include "ApexDefs.h"


#include "PxScene.h"
#include "Scene.h"

namespace nvidia
{
	namespace apex
	{

		/**
		\brief This helper class creates a scoped read access to the PhysX SDK API

		This helper class is used to create a scoped read lock/unlock pair around a section of code
		which is trying to do read access against the PhysX SDK.
		*/

		class ScopedPhysXLockRead
		{
		public:

			/**
			\brief Constructor for ScopedPhysXLockRead
			\param[in] scene the APEX scene
			\param[in] fileName used to determine what file called the lock for debugging purposes
			\param[in] lineno used to determine what line number called the lock for debugging purposes
			*/
			ScopedPhysXLockRead(nvidia::Scene* scene, const char *fileName, int lineno) : mApexScene(scene), mPhysXScene(0)
			{
				if (mApexScene)
				{
					mApexScene->lockRead(fileName, (uint32_t)lineno);
				}
			}
			
			/**
			\brief Constructor for ScopedPhysXLockRead
			\param[in] scene the PhysX scene
			\param[in] fileName used to determine what file called the lock for debugging purposes
			\param[in] lineno used to determine what line number called the lock for debugging purposes
			*/
			ScopedPhysXLockRead(physx::PxScene* scene, const char *fileName, int lineno) : mPhysXScene(scene), mApexScene(0)
			{
				if (mPhysXScene)
				{
					mPhysXScene->lockRead(fileName, (uint32_t)lineno);
				}
			}
			
			~ScopedPhysXLockRead()
			{
				if (mApexScene)
				{
					mApexScene->unlockRead();
				}
				if (mPhysXScene)
				{
					mPhysXScene->unlockRead();
				}
			}
		private:
			nvidia::Scene* mApexScene;
			physx::PxScene* mPhysXScene;
		};

		/**
		\brief This helper class creates a scoped write access to the PhysX SDK API

		This helper class is used to create a scoped write lock/unlock pair around a section of code
		which is trying to do read access against the PhysX SDK.
		*/
		class ScopedPhysXLockWrite
		{
		public:
			/**
			\brief Constructor for ScopedPhysXLockWrite
			\param[in] scene the APEX scene
			\param[in] fileName used to determine what file called the lock for debugging purposes
			\param[in] lineno used to determine what line number called the lock for debugging purposes
			*/
			ScopedPhysXLockWrite(nvidia::Scene *scene, const char *fileName, int lineno) : mApexScene(scene), mPhysXScene(0)
			{
				if (mApexScene)
				{
					mApexScene->lockWrite(fileName, (uint32_t)lineno);
				}
			}
			
			/**
			\brief Constructor for ScopedPhysXLockWrite
			\param[in] scene the PhysX scene
			\param[in] fileName used to determine what file called the lock for debugging purposes
			\param[in] lineno used to determine what line number called the lock for debugging purposes
			*/
			ScopedPhysXLockWrite(physx::PxScene *scene, const char *fileName, int lineno) : mPhysXScene(scene), mApexScene(0)
			{
				if (mPhysXScene)
				{
					mPhysXScene->lockWrite(fileName, (uint32_t)lineno);
				}
			}
			
			~ScopedPhysXLockWrite()
			{
				if (mApexScene)
				{
					mApexScene->unlockWrite();
				}
				if (mPhysXScene)
				{
					mPhysXScene->unlockWrite();
				}
			}
		private:
			nvidia::Scene* mApexScene;
			physx::PxScene* mPhysXScene;
		};
	}
}


#if defined(_DEBUG) || PX_CHECKED
/**
\brief This macro creates a scoped write lock/unlock pair
*/
#define SCOPED_PHYSX_LOCK_WRITE(x) nvidia::apex::ScopedPhysXLockWrite _wlock(x,__FILE__,__LINE__);
#else
/**
\brief This macro creates a scoped write lock/unlock pair
*/
#define SCOPED_PHYSX_LOCK_WRITE(x) nvidia::apex::ScopedPhysXLockWrite _wlock(x,"",0);
#endif

#if defined(_DEBUG) || PX_CHECKED
/**
\brief This macro creates a scoped read lock/unlock pair
*/
#define SCOPED_PHYSX_LOCK_READ(x) nvidia::apex::ScopedPhysXLockRead _rlock(x,__FILE__,__LINE__);
#else
/**
\brief This macro creates a scoped read lock/unlock pair
*/
#define SCOPED_PHYSX_LOCK_READ(x) nvidia::apex::ScopedPhysXLockRead _rlock(x,"",0);
#endif



#endif
