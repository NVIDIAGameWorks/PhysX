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



#ifndef USER_RENDER_CALLBACK_H
#define USER_RENDER_CALLBACK_H

#include "RenderDataFormat.h"

/**
\brief Cuda graphics resource
*/
typedef struct CUgraphicsResource_st* CUgraphicsResource;


namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Render interop flags
*/
struct RenderInteropFlags
{
	/**
	\brief Enum of render interop flags
	*/
	enum Enum
	{
		NO_INTEROP = 0,
		CUDA_INTEROP,
	};
};

/**
\brief Render map type
*/
struct RenderMapType
{
	/**
	\brief Enum of render map type
	*/
	enum Enum
	{
		MAP_READ                 = 1,
		MAP_WRITE                = 2,
		MAP_READ_WRITE           = 3,
		MAP_WRITE_DISCARD        = 4,
		MAP_WRITE_NO_OVERWRITE   = 5 
	};
};

/**
\brief User render data
*/
class UserRenderData
{
public:
	
	/**
	\brief Add reference
	*/
	virtual void addRef() = 0;
	
	/**
	\brief release data
	*/
	virtual void release() = 0;
};

/**
\brief User render data holder
*/
class UserRenderDataHolder
{
public:
	UserRenderDataHolder()
		: userData(NULL)
	{
	}
	
	~UserRenderDataHolder()
	{
		if (userData)
		{
			userData->release();
		}
	}
	
	/**
	\brief Copy constructor for UserRenderDataHolder
	*/
	UserRenderDataHolder(const UserRenderDataHolder& other)
		: userData(other.userData)
	{
		if (userData)
		{
			userData->addRef();
		}
	}
	
	/**
	\brief Assignment operator for UserRenderDataHolder
	*/
	UserRenderDataHolder& operator=(const UserRenderDataHolder& other)
	{
		setUserData(other.userData);
		return *this;
	}

	/**
	\brief Set user render data
	\see UserRenderData
	*/
	void setUserData(UserRenderData* newUserData)
	{
		if (userData != newUserData)
		{
			if (userData)
			{
				userData->release();
			}
			userData = newUserData;
			if (userData)
			{
				userData->addRef();
			}
		}
	}
	
	/**
	\brief Get user render data
	\see UserRenderData
	*/
	UserRenderData* getUserData() const
	{
		return userData;
	}

private:

	/**
	\brief User render data
	\see UserRenderData
	*/
	UserRenderData* userData;
};

/**
\brief User render storage
*/
class UserRenderStorage
{
public:

	/**
	\brief Release user render storage
	*/
	virtual void release() = 0;

	/**
	\brief Enum of user render storage type
	*/
	enum Type
	{
		BUFFER = 0,
		SURFACE,
	};
	
	/**
	\brief Returns type of user render storage
	*/
	virtual Type getType() const = 0;
};

/**
\brief User render buffer desc
*/
struct UserRenderBufferDesc : public UserRenderDataHolder
{
	UserRenderBufferDesc(void)
	{
		setDefaults();
	}

	/**
	\brief Default values
	*/
	void setDefaults()
	{
		size = 0;
		setUserData(NULL);
		userFlags = 0;
		interopFlags = RenderInteropFlags::NO_INTEROP;
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		return (size > 0);
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const UserRenderBufferDesc& other) const
	{
		if (size != other.size) return false;
		if (userFlags != other.userFlags) return false;
		if (getUserData() != other.getUserData()) return false;
		if (interopFlags != other.interopFlags) return false;
		return true;
	}

	size_t						size; 			///< Buffer size
	uint32_t					userFlags; 		///< User flags
	RenderInteropFlags::Enum	interopFlags;	///< Interop flags
};

/**
\brief User render buffer
*/
class UserRenderBuffer : public UserRenderStorage
{
public:

	/**
	\brief Returns BUFFER type
	*/
	virtual Type getType() const
	{
		return BUFFER;
	}

	/**
	\brief Returns CPU memory pointer to buffer content
	\note CPU access
	*/
	virtual void* map(RenderMapType::Enum mapType, size_t offset = 0, size_t size = SIZE_MAX) = 0;
	
	/**
	\brief Flush CPU buffer to GPU
	*/
	virtual void unmap() = 0;

	/**
	\brief Returns graphics resource for CUDA Interop
	\note GPU access
	*/
	virtual bool getCUDAgraphicsResource(CUgraphicsResource &ret) = 0;
};

/**
\brief User render surface desc
*/
struct UserRenderSurfaceDesc : public UserRenderDataHolder
{
	UserRenderSurfaceDesc(void)
	{
		setDefaults();
	}

	/**
	\brief Default values
	*/
	void setDefaults()
	{
		width = 0;
		height = 0;
		depth = 0;
		format = RenderDataFormat::UNSPECIFIED;
		setUserData(NULL);
		userFlags = 0;
		interopFlags = RenderInteropFlags::NO_INTEROP;
	}

	/**
	\brief Check if parameter's values are correct
	*/
	bool isValid(void) const
	{
		uint32_t numFailed = 0;
		numFailed += (width == 0);
		numFailed += (format == RenderDataFormat::UNSPECIFIED);
		return (numFailed == 0);
	}

	/**
	\brief Check if this object is the same as other
	*/
	bool isTheSameAs(const UserRenderSurfaceDesc& other) const
	{
		if (width != other.width) return false;
		if (height != other.height) return false;
		if (depth != other.depth) return false;
		if (format != other.format) return false;
		if (userFlags != other.userFlags) return false;
		if (getUserData() != other.getUserData()) return false;
		if (interopFlags != other.interopFlags) return false;
		return true;
	}

	size_t						width; 			///< Surface width
	size_t						height;			///< Surface height
	size_t						depth;			///< Surface depth
	RenderDataFormat::Enum		format;			///< Render data format
	uint32_t					userFlags;		///< User flags
	RenderInteropFlags::Enum	interopFlags;	///< Interop flags
};

/**
\brief User render surface
*/
class UserRenderSurface : public UserRenderStorage
{
public:

	/**
	\brief Returns SURFACE type
	*/
	virtual Type getType() const
	{
		return SURFACE;
	}

	/**
	\brief Mapped information
	*/
	struct MappedInfo
	{
		void*		pData; 		///< Data pointer
		uint32_t	rowPitch; 	///< Row pitch
		uint32_t	depthPitch;	///< Depth pitch
	};

	/**
	\brief Returns CPU memory pointer to buffer content
	\note CPU access
	*/
	virtual bool map(RenderMapType::Enum mapType, MappedInfo& info) = 0;
	
	/**
	\brief Flush CPU buffer to GPU
	*/
	virtual void unmap() = 0;

	/**
	\brief Returns graphics resource for CUDA Interop
	\note GPU access
	*/
	virtual bool getCUDAgraphicsResource(CUgraphicsResource &ret) = 0;
};

/**
\brief User render callback
*/
class UserRenderCallback
{
public:
	
	/**
	\brief Creates render buffer with specified UserRenderBufferDesc
	*/
	virtual UserRenderBuffer* createRenderBuffer(const UserRenderBufferDesc& )
	{
		return NULL;
	}
	/**
	\brief Creates render surface with specified UserRenderSurfaceDesc
	*/
	virtual UserRenderSurface* createRenderSurface(const UserRenderSurfaceDesc& )
	{
		return NULL;
	}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
