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



#ifndef APEX_TEST_H
#define APEX_TEST_H

#define WARNING(exp, msg) if (!(exp)) {ret &= (exp); APEX_DEBUG_WARNING(msg);}
#define EXPECT_TRUE(exp) WARNING(exp, "Expected true: " #exp)
#define EXPECT_EQ(v1, v2) WARNING(v1 == v2, "Expected: " #v1 " == " #v2)
#define EXPECT_NE(v1, v2) WARNING(v1 != v2, "Expected: " #v1 " != " #v2)
#define EXPECT_GE(v1, v2) WARNING(v1 >= v2, "Expected: " #v1 " >= " #v2)
#define EXPECT_GT(v1, v2) WARNING(v1 > v2, "Expected: " #v1 " > " #v2)
#define EXPECT_LE(v1, v2) WARNING(v1 <= v2, "Expected: " #v1 " <= " #v2)
#define EXPECT_LT(v1, v2) WARNING(v1 < v2, "Expected: " #v1 " < " #v2)

#endif //APEX_TEST_H
