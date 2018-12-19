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


#ifndef APEX_STRING_H
#define APEX_STRING_H

#include "ApexUsingNamespace.h"
#include "PsArray.h"
#include "PsString.h"
#include "PsArray.h"
#include "PsUserAllocated.h"
#include <PxFileBuf.h>

namespace nvidia
{
namespace apex
{

/**
 * ApexSimpleString - a simple string class
 */
class ApexSimpleString : public physx::Array<char>, public UserAllocated
{
public:
	ApexSimpleString() : physx::Array<char>(), length(0)
	{
	}

	explicit ApexSimpleString(const char* cStr) : physx::Array<char>(), length(0)
	{
		if (cStr)
		{
			length = (uint32_t)strlen(cStr);
			if (length > 0)
			{
				resize(length + 1);
				nvidia::strlcpy(begin(), size(), cStr);
			}
		}
	}

	ApexSimpleString(const ApexSimpleString& other) : physx::Array<char>()
	{
		length = other.length;
		if (length > 0)
		{
			resize(length + 1);
			nvidia::strlcpy(begin(), capacity(), other.c_str());
		}
		else
		{
			resize(0);
		}
	}

	ApexSimpleString(uint32_t number, uint32_t fixedLength = 0) : length(fixedLength)
	{
		if (fixedLength)
		{
			char format[5]; format[0] = '%'; format[1] = '0';
			char buffer[10];
			if (fixedLength > 9)
			{
				PX_ASSERT(fixedLength);
				fixedLength = 9;
			}
			physx::shdfnd::snprintf(format + 2, 2, "%d", fixedLength);
			format[3] = 'd'; format[4] = '\0';
			physx::shdfnd::snprintf(buffer, 10, format, number);
			resize(length + 1);
			nvidia::strlcpy(begin(), size(), buffer);
		}
		else
		{
			char buffer[10];
			physx::shdfnd::snprintf(buffer, 10, "%d", number);
			length = 1;
			while (number >= 10)
			{
				number /= 10;
				length++;
			}
			resize(length + 1);
			nvidia::strlcpy(begin(), size(), buffer);
		}
	}

	ApexSimpleString& operator = (const ApexSimpleString& other)
	{
		length = other.length;
		if (length > 0)
		{
			resize(length + 1);
			nvidia::strlcpy(begin(), capacity(), other.c_str());
		}
		else
		{
			resize(0);
		}
		return *this;
	}

	ApexSimpleString& operator = (const char* cStr)
	{
		if (!cStr)
		{
			erase();
		}
		else
		{
			length = (uint32_t)strlen(cStr);
			if (length > 0)
			{
				resize(length + 1);
				nvidia::strlcpy(begin(), capacity(), cStr);
			}
			else
			{
				resize(0);
			}
		}
		return *this;
	}

	void truncate(uint32_t newLength)
	{
		if (newLength < length)
		{
			length = newLength;
			begin()[length] = '\0';
		}
	}

	void serialize(physx::PxFileBuf& stream) const
	{
		stream.storeDword(length);
		stream.write(begin(), length);
	}

	void deserialize(physx::PxFileBuf& stream)
	{
		uint32_t len = stream.readDword();
		if (len > 0)
		{
			resize(len + 1);
			stream.read(begin(), len);
			begin()[len] = '\0';
			length = len;
		}
		else
		{
			erase();
		}
	}

	uint32_t	len() const
	{
		return length;
	}

	/* PH: Cast operator not allowed by coding guidelines, and evil in general anyways
	operator const char* () const
	{
	return capacity() ? begin() : "";
	}
	*/
	const char* c_str() const
	{
		return capacity() > 0 ? begin() : "";
	}

	bool operator==(const ApexSimpleString& s) const
	{
		return nvidia::strcmp(c_str(), s.c_str()) == 0;
	}
	bool operator!=(const ApexSimpleString& s) const
	{
		return ! this->operator==(s);
	}
	bool operator==(const char* s) const
	{
		return nvidia::strcmp(c_str(), s) == 0;
	}
	bool operator!=(const char* s) const
	{
		return ! this->operator==(s);
	}
	bool operator < (const ApexSimpleString& s) const
	{
		return nvidia::strcmp(c_str(), s.c_str()) < 0;
	}

	ApexSimpleString& operator += (const ApexSimpleString& s)
	{
		expandTo(length + s.length);
		nvidia::strlcpy(begin() + length, capacity() - length, s.c_str());
		length += s.length;
		return *this;
	}

	ApexSimpleString& operator += (char c)
	{
		expandTo(length + 1);
		begin()[length++] = c;
		begin()[length] = '\0';
		return *this;
	}

	ApexSimpleString operator + (const ApexSimpleString& s)
	{
		ApexSimpleString sum = *this;
		sum += s;
		return sum;
	}

	ApexSimpleString& 	clear()
	{
		if (capacity())
		{
			begin()[0] = '\0';
		}
		length = 0;
		return *this;
	}

	ApexSimpleString& 	erase()
	{
		resize(0);
		return clear();
	}

	static PX_INLINE void ftoa(float f, ApexSimpleString& s)
	{
		char buf[20];
		physx::shdfnd::snprintf(buf, sizeof(buf), "%g", f);
		s = buf;
	}

	static PX_INLINE void itoa(uint32_t i, ApexSimpleString& s)
	{
		char buf[20];
		physx::shdfnd::snprintf(buf, sizeof(buf), "%i", i);
		s = buf;
	}

private:

	void expandTo(uint32_t stringCapacity)
	{
		if (stringCapacity + 1 > capacity())
		{
			resize(2 * stringCapacity + 1);
		}
	}

	uint32_t length;
};

PX_INLINE ApexSimpleString operator + (const ApexSimpleString& s1, const ApexSimpleString& s2)
{
	ApexSimpleString result = s1;
	result += s2;
	return result;
}

} // namespace apex
} // namespace nvidia

#endif // APEX_STRING_H
