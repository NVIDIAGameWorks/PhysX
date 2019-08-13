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
#ifndef D3D11_RENDERER_RESOURCE_MANAGER_H
#define D3D11_RENDERER_RESROUCE_MAANGER_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include <algorithm>
#include <typeinfo>

#include "D3D11RendererTraits.h"
#include "D3D11RendererMemoryMacros.h"

namespace SampleRenderer
{

#define USE_ANY_AS_CONTAINER 1
class Any
{
public:
	class Visitor;
private:
	class Proxy;

public:
	Any() : mpProxy(NULL) { }
	~Any() { delete mpProxy; }

public:
	Any(const Any &other): mpProxy(other.mpProxy ? other.mpProxy->clone() : 0) { }
	Any &swap(Any &rhs) { std::swap(mpProxy, rhs.mpProxy); return *this; }
	Any &operator=(const Any &rhs) { Any rhsTemp(rhs); return swap(rhsTemp); }

	template<typename value_type>
	Any(const value_type &value) : mpProxy(new ProxyImpl<value_type>(value)) { }

	template<typename value_type>
	Any &operator=(const value_type &rhs) { Any rhsTemp(rhs); return swap(rhsTemp); }

	void assign(Proxy *otherProxy) { mpProxy = otherProxy; }

public:
	const std::type_info &type_info() const { return mpProxy ? mpProxy->type_info() : typeid(void); }
	operator const void *() const { return mpProxy; }

	template<typename value_type>
	bool copy_to(value_type &value) const { 
		const value_type *copyable = to_ptr<value_type>();
		if(copyable) value = *copyable;
		return copyable;
	}
	template<typename value_type>
	const value_type *to_ptr() const
	{
		return type_info() == typeid(value_type) ? &static_cast<ProxyImpl<value_type> *>(mpProxy)->mValue : 0;
	}
	template<typename value_type>
	value_type *to_ptr() 
	{
		return type_info() == typeid(value_type) ? &static_cast<ProxyImpl<value_type> *>(mpProxy)->mValue : 0;
	}
	
#if !USE_ANY_AS_CONTAINER
	bool operator==(const Any& other) const { 
		return (mpProxy &&  other.mpProxy) ? (*mpProxy == *other.mpProxy) 
			:((!mpProxy && !other.mpProxy) ?  true : false); }
#endif

	void release() { if(mpProxy) mpProxy->release(); }
	//void accept(Visitor& visitor) {  if (mpProxy) mpProxy->accept(visitor); }

public:
	class Visitor
	{
	public:
		virtual void visit(Proxy& proxy) = 0;
	};

private:


	template<typename T>
	struct Releaser	{
	public:
		static void release(T& t) { }
	};

	template <typename T>
	struct Releaser<T*>
	{
	public:
		static void release(T*& t) { dxSafeRelease(t); }
	};

	template<typename T1,typename T2>
	struct Releaser< std::pair<T1*,T2*> >
	{
	public:
		static void release(std::pair<T1*,T2*>& t) { dxSafeRelease(t.first); dxSafeRelease(t.second); }
	};

	template<typename T1,typename T2>
	struct Releaser< std::map<T1,T2> > : public Visitor
	{
	public:
		static void release(std::map<T1,T2>& t) 
		{
			for (typename std::map<T1,T2>::iterator it = t.begin(); it != t.end(); ++it)
			{
				Releaser<T2>::release(it->second);
			}
		}
	};

	class Proxy
	{
	public:
		virtual const std::type_info& type_info() const = 0;
		virtual Proxy *clone() const = 0;
		virtual void release() = 0;
#if !USE_ANY_AS_CONTAINER
		virtual bool operator<(const Proxy&) const = 0;
		virtual bool operator==(const Proxy&) const = 0;
#endif
	};
	template<typename value_type>
	class ProxyImpl : public Proxy
	{
	public:
		ProxyImpl(const value_type &value) : mValue(value) { }
		virtual const std::type_info &type_info() const { return typeid(value_type); }
		virtual Proxy *clone() const { return new ProxyImpl(mValue); }
		virtual void release() { Releaser<value_type>::release(mValue); }

#if !USE_ANY_AS_CONTAINER
		virtual bool operator<(const Proxy& rhs) const { return mValue < static_cast< const ProxyImpl<value_type>& >(rhs).mValue; }
		virtual bool operator==(const Proxy& rhs) const { return mValue == static_cast< const ProxyImpl<value_type>& >(rhs).mValue; }
#endif
		value_type mValue;
	private:
		ProxyImpl &operator=(const ProxyImpl&);
	};

public:

#if !USE_ANY_AS_CONTAINER
	struct Comp
	{
		bool operator()(const Any& lhs, const Any& rhs) const { return *(lhs.mpProxy) < *(rhs.mpProxy); }
	};
#endif

	// Let's us use the stack instead of calling new every time we use Any
	template<typename value_type>
	class Temp
	{
	public:
		Temp(const value_type& value) : mProxy(value) { mAny.assign(&mProxy); }
		~Temp() { mAny.assign(NULL); }

		Any& operator()(void) { return mAny; }
		const Any& operator()(void) const { return mAny; }

	protected:
		ProxyImpl<value_type> mProxy;
		Any                   mAny;
	};


private:
	Proxy *mpProxy;
};

template<typename value_type>
value_type& any_cast(Any &operand) {
	return *operand.to_ptr<value_type>();
}

template<typename value_type>
const value_type& any_cast(const Any &operand) {
	return *operand.to_ptr<value_type>();
}

class D3D11RendererResourceManager
{
public:
#if     USE_ANY_AS_CONTAINER
	typedef Any                           CacheType;
#else
	typedef std::map<Any, Any, Any::Comp> CacheType;
#endif
	D3D11RendererResourceManager() 
	{ 
#if USE_ANY_AS_CONTAINER
		mResources[D3DTraits<ID3D11VertexShader>::getType()] = 
			std::map< typename D3DTraits<ID3D11VertexShader>::key_type,     typename D3DTraits<ID3D11VertexShader>::value_type >();
		mResources[D3DTraits<ID3D11PixelShader>::getType()] = 
			std::map< typename D3DTraits<ID3D11PixelShader>::key_type,      typename D3DTraits<ID3D11PixelShader>::value_type >();
		mResources[D3DTraits<ID3D11GeometryShader>::getType()] = 
			std::map< typename D3DTraits<ID3D11GeometryShader>::key_type,   typename D3DTraits<ID3D11GeometryShader>::value_type >();
		mResources[D3DTraits<ID3D11HullShader>::getType()] = 
			std::map< typename D3DTraits<ID3D11HullShader>::key_type,       typename D3DTraits<ID3D11HullShader>::value_type >();
		mResources[D3DTraits<ID3D11DomainShader>::getType()] = 
			std::map< typename D3DTraits<ID3D11DomainShader>::key_type,     typename D3DTraits<ID3D11DomainShader>::value_type >();
		mResources[D3DTraits<ID3D11InputLayout>::getType()] = 
			std::map< typename D3DTraits<ID3D11InputLayout>::key_type,      typename D3DTraits<ID3D11InputLayout>::value_type >();
		mResources[D3DTraits<ID3D11RasterizerState>::getType()] = 
			std::map< typename D3DTraits<ID3D11RasterizerState>::key_type,  typename D3DTraits<ID3D11RasterizerState>::value_type >();
		mResources[D3DTraits<ID3D11DepthStencilState>::getType()] = 
			std::map< typename D3DTraits<ID3D11DepthStencilState>::key_type, typename D3DTraits<ID3D11DepthStencilState>::value_type >();
		mResources[D3DTraits<ID3D11BlendState>::getType()] = 
			std::map< typename D3DTraits<ID3D11BlendState>::key_type,        typename D3DTraits<ID3D11BlendState>::value_type >();
#endif
	}
	~D3D11RendererResourceManager() 
	{
#if USE_ANY_AS_CONTAINER
		for (PxU32 i = 0; i < D3DTypes::NUM_TYPES; ++i)
		{
			mResources[i].release();
		}
#else
		for (PxU32 i = 0; i < D3DTypes::NUM_TYPES; ++i)
		{
			for (CacheType::iterator it = mResources[i].begin(); 
				it != mResources[i].end(); 
				++it)
			{
				it->second.release();
			}
		}
#endif
	}

public:

	template<typename d3d_type>
	typename D3DTraits<d3d_type>::value_type 
	hasResource(const typename D3DTraits<d3d_type>::key_type& key)
	{
		typedef typename D3DTraits<d3d_type>::key_type   key_type;
		typedef typename D3DTraits<d3d_type>::value_type value_type;
		typedef std::map<key_type,value_type>            cache_type;
		static const int resourceID = D3DTraits<d3d_type>::getType();
		RENDERER_ASSERT(resourceID != D3DTypes::INVALID, "Invalid D3D resource type");
#if USE_ANY_AS_CONTAINER
		RENDERER_ASSERT(!(mResources[resourceID] == Any()), "Invalid D3D resource container");
		cache_type& resources = any_cast< cache_type >(mResources[resourceID]);
		typename cache_type::iterator it = resources.find(key);
		return (it != resources.end()) ? it->second : NullTraits<value_type>::get();
#else
		Any::Temp<key_type> tempAny(key);
		typename CacheType::iterator it = mResources[resourceID].find(tempAny());
		return (it != mResources[resourceID].end()) ? any_cast<value_type>(it->second) : NullTraits<value_type>::get();
#endif
	}

	template<typename d3d_type>
	const typename D3DTraits<d3d_type>::value_type
	hasResource(const typename D3DTraits<d3d_type>::key_type& key) const
	{
		typedef typename D3DTraits<d3d_type>::key_type   key_type;
		typedef typename D3DTraits<d3d_type>::value_type value_type;
		typedef std::map<key_type,value_type>            cache_type;
		static const int resourceID = D3DTraits<d3d_type>::getType();
		RENDERER_ASSERT(resourceID != D3DTypes::INVALID, "Invalid D3D resource type");
#if USE_ANY_AS_CONTAINER
		RENDERER_ASSERT(!(mResources[resourceID] == Any()), "Invalid D3D resource container");
		const cache_type& resources = any_cast< cache_type >(mResources[resourceID]);
		typename cache_type::const_iterator it = resources.find(key);
		return (it != resources.end()) ? it->second : NullTraits<value_type>::get();
#else
		Any::Temp<key_type> tempAny(key); 
		typename CacheType::iterator it = mResources[resourceID].find(tempAny());
		return (it != mResources[resourceID].end()) ? any_cast<value_type>(it->second) : NullTraits<value_type>::get();
#endif
	}

	template<typename d3d_type>
	void registerResource(const typename D3DTraits<d3d_type>::key_type&   key, 
		                  const typename D3DTraits<d3d_type>::value_type& value)
	{
		typedef typename D3DTraits<d3d_type>::key_type   key_type;
		typedef typename D3DTraits<d3d_type>::value_type value_type;
		typedef std::map<key_type,value_type>            cache_type;
		static const int resourceID = D3DTraits<d3d_type>::getType();
		RENDERER_ASSERT(resourceID != D3DTypes::INVALID, "Invalid D3D resource type");
#if USE_ANY_AS_CONTAINER
		RENDERER_ASSERT(!(mResources[resourceID] == Any()), "Invalid D3D resource container");
		cache_type& resources = any_cast< cache_type >(mResources[resourceID]);
		//resources[key] = value;
		resources.insert(std::make_pair(key, value));
#else
		Any::Temp<key_type> tempAny(key);
		//mResources[resourceID][ tempAny() ] = value;
		mResources[resourceID].insert(std::make_pair(tempAny(), value));
#endif
	}

private:
	CacheType mResources[D3DTypes::NUM_TYPES];
};

} // namespace SampleRenderer

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)

#endif
