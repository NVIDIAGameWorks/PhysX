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



#ifndef __glext_h_
#define __glext_h_

#ifdef __cplusplus
extern "C" {
#endif

	/*
	** License Applicability. Except to the extent portions of this file are
	** made subject to an alternative license as permitted in the SGI Free
	** Software License B, Version 1.1 (the "License"), the contents of this
	** file are subject only to the provisions of the License. You may not use
	** this file except in compliance with the License. You may obtain a copy
	** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
	** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
	**
	** http://oss.sgi.com/projects/FreeB
	**
	** Note that, as provided in the License, the Software is distributed on an
	** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
	** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
	** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
	** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
	**
	** Original Code. The Original Code is: OpenGL Sample Implementation,
	** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
	** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
	** Copyright in any portions created by third parties is as indicated
	** elsewhere herein. All Rights Reserved.
	**
	** Additional Notice Provisions: This software was created using the
	** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
	** not been independently verified as being compliant with the OpenGL(R)
	** version 1.2.1 Specification.
	*/

#if defined(_WIN32) && !defined(APIENTRY) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

	/*************************************************************/

	/* Header file version number, required by OpenGL ABI for Linux */
#define GL_GLEXT_VERSION 6

#ifndef GL_VERSION_1_2
#define GL_CONSTANT_COLOR                 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_BLEND_COLOR                    0x8005
#define GL_FUNC_ADD                       0x8006
#define GL_MIN                            0x8007
#define GL_MAX                            0x8008
#define GL_BLEND_EQUATION                 0x8009
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_CONVOLUTION_1D                 0x8010
#define GL_CONVOLUTION_2D                 0x8011
#define GL_SEPARABLE_2D                   0x8012
#define GL_CONVOLUTION_BORDER_MODE        0x8013
#define GL_CONVOLUTION_FILTER_SCALE       0x8014
#define GL_CONVOLUTION_FILTER_BIAS        0x8015
#define GL_REDUCE                         0x8016
#define GL_CONVOLUTION_FORMAT             0x8017
#define GL_CONVOLUTION_WIDTH              0x8018
#define GL_CONVOLUTION_HEIGHT             0x8019
#define GL_MAX_CONVOLUTION_WIDTH          0x801A
#define GL_MAX_CONVOLUTION_HEIGHT         0x801B
#define GL_POST_CONVOLUTION_RED_SCALE     0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE   0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE    0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE   0x801F
#define GL_POST_CONVOLUTION_RED_BIAS      0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS    0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS     0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS    0x8023
#define GL_HISTOGRAM                      0x8024
#define GL_PROXY_HISTOGRAM                0x8025
#define GL_HISTOGRAM_WIDTH                0x8026
#define GL_HISTOGRAM_FORMAT               0x8027
#define GL_HISTOGRAM_RED_SIZE             0x8028
#define GL_HISTOGRAM_GREEN_SIZE           0x8029
#define GL_HISTOGRAM_BLUE_SIZE            0x802A
#define GL_HISTOGRAM_ALPHA_SIZE           0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE       0x802C
#define GL_HISTOGRAM_SINK                 0x802D
#define GL_MINMAX                         0x802E
#define GL_MINMAX_FORMAT                  0x802F
#define GL_MINMAX_SINK                    0x8030
#define GL_TABLE_TOO_LARGE                0x8031
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_RESCALE_NORMAL                 0x803A
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_COLOR_MATRIX                   0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH       0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH   0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE    0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE  0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE   0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE  0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS     0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS   0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS    0x80BA
#define GL_COLOR_TABLE                    0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE   0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE  0x80D2
#define GL_PROXY_COLOR_TABLE              0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE 0x80D5
#define GL_COLOR_TABLE_SCALE              0x80D6
#define GL_COLOR_TABLE_BIAS               0x80D7
#define GL_COLOR_TABLE_FORMAT             0x80D8
#define GL_COLOR_TABLE_WIDTH              0x80D9
#define GL_COLOR_TABLE_RED_SIZE           0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE         0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE          0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE         0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE     0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE     0x80DF
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_LIGHT_MODEL_COLOR_CONTROL      0x81F8
#define GL_SINGLE_COLOR                   0x81F9
#define GL_SEPARATE_SPECULAR_COLOR        0x81FA

#define GL_COMBINE                    0x8570
#define GL_COMBINE_RGB                0x8571
#define GL_COMBINE_ALPHA              0x8572
#define GL_RGB_SCALE                  0x8573
#define GL_ADD_SIGNED                 0x8574
#define GL_INTERPOLATE                0x8575
#define GL_CONSTANT                   0x8576
#define GL_PRIMARY_COLOR              0x8577
#define GL_PREVIOUS                   0x8578
#define GL_SOURCE0_RGB                0x8580
#define GL_SOURCE1_RGB                0x8581
#define GL_SOURCE2_RGB                0x8582
#define GL_SOURCE3_RGB                0x8583
#define GL_SOURCE4_RGB                0x8584
#define GL_SOURCE5_RGB                0x8585
#define GL_SOURCE6_RGB                0x8586
#define GL_SOURCE7_RGB                0x8587
#define GL_SOURCE0_ALPHA              0x8588
#define GL_SOURCE1_ALPHA              0x8589
#define GL_SOURCE2_ALPHA              0x858A
#define GL_SOURCE3_ALPHA              0x858B
#define GL_SOURCE4_ALPHA              0x858C
#define GL_SOURCE5_ALPHA              0x858D
#define GL_SOURCE6_ALPHA              0x858E
#define GL_SOURCE7_ALPHA              0x858F
#define GL_OPERAND0_RGB               0x8590
#define GL_OPERAND1_RGB               0x8591
#define GL_OPERAND2_RGB               0x8592
#define GL_OPERAND3_RGB               0x8593
#define GL_OPERAND4_RGB               0x8594
#define GL_OPERAND5_RGB               0x8595
#define GL_OPERAND6_RGB               0x8596
#define GL_OPERAND7_RGB               0x8597
#define GL_OPERAND0_ALPHA             0x8598
#define GL_OPERAND1_ALPHA             0x8599
#define GL_OPERAND2_ALPHA             0x859A
#define GL_OPERAND3_ALPHA             0x859B
#define GL_OPERAND4_ALPHA             0x859C
#define GL_OPERAND5_ALPHA             0x859D
#define GL_OPERAND6_ALPHA             0x859E
#define GL_OPERAND7_ALPHA             0x859F
#define GL_SUBTRACT                   0x84E7
#define GL_DOT3_RGB                   0x86AE
#define GL_DOT3_RGBA                  0x86AF

#define GL_TEXTURE0                   0x84C0
#define GL_TEXTURE1                   0x84C1
#define GL_TEXTURE2                   0x84C2
#define GL_TEXTURE3                   0x84C3
#define GL_TEXTURE4                   0x84C4
#define GL_TEXTURE5                   0x84C5
#define GL_TEXTURE6                   0x84C6
#define GL_TEXTURE7                   0x84C7
#define GL_TEXTURE8                   0x84C8
#define GL_TEXTURE9                   0x84C9
#define GL_TEXTURE10                  0x84CA
#define GL_TEXTURE11                  0x84CB
#define GL_TEXTURE12                  0x84CC
#define GL_TEXTURE13                  0x84CD
#define GL_TEXTURE14                  0x84CE
#define GL_TEXTURE15                  0x84CF
#define GL_TEXTURE16                  0x84D0
#define GL_TEXTURE17                  0x84D1
#define GL_TEXTURE18                  0x84D2
#define GL_TEXTURE19                  0x84D3
#define GL_TEXTURE20                  0x84D4
#define GL_TEXTURE21                  0x84D5
#define GL_TEXTURE22                  0x84D6
#define GL_TEXTURE23                  0x84D7
#define GL_TEXTURE24                  0x84D8
#define GL_TEXTURE25                  0x84D9
#define GL_TEXTURE26                  0x84DA
#define GL_TEXTURE27                  0x84DB
#define GL_TEXTURE28                  0x84DC
#define GL_TEXTURE29                  0x84DD
#define GL_TEXTURE30                  0x84DE
#define GL_TEXTURE31                  0x84DF
#define GL_ACTIVE_TEXTURE             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE      0x84E1
#define GL_MAX_TEXTURE_UNITS          0x84E2
#define GL_CLAMP_TO_BORDER            0x812D

#define GL_BGR			              0x80E0
#define GL_BGRA         		      0x80E1
#endif

#ifndef GL_APPLE_specular_vector
#define GL_LIGHT_MODEL_SPECULAR_VECTOR_APPLE 0x85B0
#endif

#ifndef GL_APPLE_transform_hint
#define GL_TRANSFORM_HINT_APPLE           0x85B1
#endif

#ifndef GL_ARB_depth_texture
#define GL_DEPTH_COMPONENT16_ARB          0x81A5
#define GL_DEPTH_COMPONENT24_ARB          0x81A6
#define GL_DEPTH_COMPONENT32_ARB          0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB         0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB         0x884B
#endif

#ifndef GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB                     0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB             0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB             0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB             0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB      0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB      0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB      0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB         0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB         0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB         0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB  0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB  0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB  0x8810
#define GL_MAX_TEXTURE_COORDS_ARB                   0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB              0x8872
#endif

#ifndef GL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042
#define GL_MULTISAMPLE_ARB                0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB   0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB        0x809F
#define GL_SAMPLE_COVERAGE_ARB            0x80A0
#define GL_SAMPLE_BUFFERS_ARB             0x80A8
#define GL_SAMPLES_ARB                    0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB      0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB     0x80AB
#define GL_MULTISAMPLE_BIT_ARB            0x20000000
#endif

#ifndef GL_ARB_multitexture
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2
#endif

#ifndef GL_ARB_point_parameters
#define GL_POINT_SIZE_MIN_ARB                0x8126
#define GL_POINT_SIZE_MAX_ARB                0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB     0x8128
#define GL_POINT_DISTANCE_ATTENUATION_ARB    0x8129
#endif

#ifndef GL_ARB_shadow
#define GL_TEXTURE_COMPARE_MODE_ARB       0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB       0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB       0x884E
#endif

#ifndef GL_ARB_texture_border_clamp
#define GL_CLAMP_TO_BORDER_ARB            0x812D
#endif

#ifndef GL_ARB_texture_compression
#define GL_COMPRESSED_ALPHA_ARB           0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB       0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB       0x84EC
#define GL_COMPRESSED_RGB_ARB             0x84ED
#define GL_COMPRESSED_RGBA_ARB            0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB   0x84EF
#define GL_TEXTURE_IMAGE_SIZE_ARB         0x86A0
#define GL_TEXTURE_COMPRESSED_ARB         0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3
#endif

#ifndef GL_ARB_texture_cube_map
#define GL_NORMAL_MAP_ARB                 0x8511
#define GL_REFLECTION_MAP_ARB             0x8512
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C
#endif

#ifndef GL_ARB_texture_env_add
#endif

#ifndef GL_ARB_texture_env_combine
#define GL_COMBINE_ARB                    0x8570
#define GL_COMBINE_RGB_ARB                0x8571
#define GL_COMBINE_ALPHA_ARB              0x8572
#define GL_RGB_SCALE_ARB                  0x8573
#define GL_ADD_SIGNED_ARB                 0x8574
#define GL_INTERPOLATE_ARB                0x8575
#define GL_CONSTANT_ARB                   0x8576
#define GL_PRIMARY_COLOR_ARB              0x8577
#define GL_PREVIOUS_ARB                   0x8578
#define GL_SOURCE0_RGB_ARB                0x8580
#define GL_SOURCE1_RGB_ARB                0x8581
#define GL_SOURCE2_RGB_ARB                0x8582
#define GL_SOURCE0_ALPHA_ARB              0x8588
#define GL_SOURCE1_ALPHA_ARB              0x8589
#define GL_SOURCE2_ALPHA_ARB              0x858A
#define GL_OPERAND0_RGB_ARB               0x8590
#define GL_OPERAND1_RGB_ARB               0x8591
#define GL_OPERAND2_RGB_ARB               0x8592
#define GL_OPERAND0_ALPHA_ARB             0x8598
#define GL_OPERAND1_ALPHA_ARB             0x8599
#define GL_OPERAND2_ALPHA_ARB             0x859A
#define GL_SUBTRACT_ARB                   0x84E7
#endif

#ifndef GL_ARB_texture_env_dot3
#define GL_DOT3_RGB_ARB                   0x86AE
#define GL_DOT3_RGBA_ARB                  0x86AF
#endif

#ifndef GL_ARB_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_ARB            0x8370
#endif

#ifndef GL_ARB_transpose_matrix
#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB 0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB 0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB   0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX_ARB     0x84E6
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB                             0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB                     0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB                     0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB             0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB              0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB              0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB               0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB               0x8899
#define TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB          0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB           0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB     0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB      0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB              0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB       0x889F
#define GL_STREAM_DRAW_ARB                              0x88E0
#define GL_STREAM_READ_ARB                              0x88E1
#define GL_STREAM_COPY_ARB                              0x88E2
#define GL_STATIC_DRAW_ARB                              0x88E4
#define GL_STATIC_READ_ARB                              0x88E5
#define GL_STATIC_COPY_ARB                              0x88E6
#define GL_DYNAMIC_DRAW_ARB                             0x88E8
#define GL_DYNAMIC_READ_ARB                             0x88E9
#define GL_DYNAMIC_COPY_ARB                             0x88EA
#define GL_READ_ONLY_ARB                                0x88B8
#define GL_WRITE_ONLY_ARB                               0x88B9
#define GL_READ_WRITE_ARB                               0x88BA
#define GL_BUFFER_SIZE_ARB                              0x8764
#define GL_BUFFER_USAGE_ARB                             0x8765
#define GL_BUFFER_ACCESS_ARB                            0x88BB
#define GL_BUFFER_MAPPED_ARB                            0x88BC
#define GL_BUFFER_MAP_POINTER_ARB                       0x88BD
#endif

#ifndef GL_ARB_vertex_program
#define GL_VERTEX_PROGRAM_ARB                       0x8620
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB            0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB              0x8643
#define GL_COLOR_SUM_ARB                            0x8458
#define GL_PROGRAM_FORMAT_ASCII_ARB                 0x8875
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB          0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB             0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB           0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB             0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB       0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB                0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB          0x8645
#define GL_PROGRAM_LENGTH_ARB                       0x8627
#define GL_PROGRAM_FORMAT_ARB                       0x8876
#define GL_PROGRAM_BINDING_ARB                      0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB                 0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB             0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB          0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB      0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB                  0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB              0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB           0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB       0x88A7
#define GL_PROGRAM_PARAMETERS_ARB                   0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB               0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB            0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB        0x88AB
#define GL_PROGRAM_ATTRIBS_ARB                      0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB                  0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB               0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB           0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB            0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB        0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB     0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB         0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB           0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB          0x88B6
#define GL_PROGRAM_STRING_ARB                       0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB               0x864B
#define GL_CURRENT_MATRIX_ARB                       0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB             0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB           0x8640
#define GL_MAX_VERTEX_ATTRIBS_ARB                   0x8869
#define GL_MAX_PROGRAM_MATRICES_ARB                 0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB       0x862E
#define GL_PROGRAM_ERROR_STRING_ARB                 0x8874
#define GL_MATRIX0_ARB                              0x88C0
#define GL_MATRIX1_ARB                              0x88C1
#define GL_MATRIX2_ARB                              0x88C2
#define GL_MATRIX3_ARB                              0x88C3
#define GL_MATRIX4_ARB                              0x88C4
#define GL_MATRIX5_ARB                              0x88C5
#define GL_MATRIX6_ARB                              0x88C6
#define GL_MATRIX7_ARB                              0x88C7
#define GL_MATRIX8_ARB                              0x88C8
#define GL_MATRIX9_ARB                              0x88C9
#define GL_MATRIX10_ARB                             0x88CA
#define GL_MATRIX11_ARB                             0x88CB
#define GL_MATRIX12_ARB                             0x88CC
#define GL_MATRIX13_ARB                             0x88CD
#define GL_MATRIX14_ARB                             0x88CE
#define GL_MATRIX15_ARB                             0x88CF
#define GL_MATRIX16_ARB                             0x88D0
#define GL_MATRIX17_ARB                             0x88D1
#define GL_MATRIX18_ARB                             0x88D2
#define GL_MATRIX19_ARB                             0x88D3
#define GL_MATRIX20_ARB                             0x88D4
#define GL_MATRIX21_ARB                             0x88D5
#define GL_MATRIX22_ARB                             0x88D6
#define GL_MATRIX23_ARB                             0x88D7
#define GL_MATRIX24_ARB                             0x88D8
#define GL_MATRIX25_ARB                             0x88D9
#define GL_MATRIX26_ARB                             0x88DA
#define GL_MATRIX27_ARB                             0x88DB
#define GL_MATRIX28_ARB                             0x88DC
#define GL_MATRIX29_ARB                             0x88DD
#define GL_MATRIX30_ARB                             0x88DE
#define GL_MATRIX31_ARB                             0x88DF
#endif

#ifndef GL_ARB_window_pos
#endif

#ifndef GL_EXT_422_pixels
#define GL_422_EXT                        0x80CC
#define GL_422_REV_EXT                    0x80CD
#define GL_422_AVERAGE_EXT                0x80CE
#define GL_422_REV_AVERAGE_EXT            0x80CF
#endif

#ifndef GL_EXT_abgr
#define GL_ABGR_EXT                       0x8000
#endif

#ifndef GL_EXT_bgra
#define GL_BGR_EXT                        0x80E0
#define GL_BGRA_EXT                       0x80E1
#endif

#ifndef GL_EXT_blend_color
#define GL_CONSTANT_COLOR_EXT             0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT   0x8002
#define GL_CONSTANT_ALPHA_EXT             0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT   0x8004
#define GL_BLEND_COLOR_EXT                0x8005
#endif

#ifndef GL_EXT_blend_func_separate
#define GL_BLEND_DST_RGB_EXT              0x80C8
#define GL_BLEND_SRC_RGB_EXT              0x80C9
#define GL_BLEND_DST_ALPHA_EXT            0x80CA
#define GL_BLEND_SRC_ALPHA_EXT            0x80CB
#endif

#ifndef GL_EXT_blend_logic_op
#endif

#ifndef GL_EXT_blend_minmax
#define GL_FUNC_ADD_EXT                   0x8006
#define GL_MIN_EXT                        0x8007
#define GL_MAX_EXT                        0x8008
#define GL_BLEND_EQUATION_EXT             0x8009
#endif

#ifndef GL_EXT_blend_subtract
#define GL_FUNC_SUBTRACT_EXT              0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT      0x800B
#endif

#ifndef GL_EXT_polygon_offset
#define GL_POLYGON_OFFSET_EXT             0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT      0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT        0x8039
#endif

#ifndef GL_EXT_clip_volume_hint
#define GL_CLIP_VOLUME_CLIPPING_HINT_EXT  0x80F0
#endif

#ifndef GL_EXT_cmyka
#define GL_CMYK_EXT                       0x800C
#define GL_CMYKA_EXT                      0x800D
#define GL_PACK_CMYK_HINT_EXT             0x800E
#define GL_UNPACK_CMYK_HINT_EXT           0x800F
#endif

#ifndef GL_EXT_color_subtable
#endif

#ifndef GL_EXT_compiled_vertex_array
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT   0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT   0x81A9
#endif

#ifndef GL_EXT_convolution
#define GL_CONVOLUTION_1D_EXT             0x8010
#define GL_CONVOLUTION_2D_EXT             0x8011
#define GL_SEPARABLE_2D_EXT               0x8012
#define GL_CONVOLUTION_BORDER_MODE_EXT    0x8013
#define GL_CONVOLUTION_FILTER_SCALE_EXT   0x8014
#define GL_CONVOLUTION_FILTER_BIAS_EXT    0x8015
#define GL_REDUCE_EXT                     0x8016
#define GL_CONVOLUTION_FORMAT_EXT         0x8017
#define GL_CONVOLUTION_WIDTH_EXT          0x8018
#define GL_CONVOLUTION_HEIGHT_EXT         0x8019
#define GL_MAX_CONVOLUTION_WIDTH_EXT      0x801A
#define GL_MAX_CONVOLUTION_HEIGHT_EXT     0x801B
#define GL_POST_CONVOLUTION_RED_SCALE_EXT 0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT 0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT 0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT 0x801F
#define GL_POST_CONVOLUTION_RED_BIAS_EXT  0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT 0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT 0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT 0x8023
#endif

#ifndef GL_EXT_copy_texture
#endif

#ifndef GL_EXT_cull_vertex
#define GL_CULL_VERTEX_EXT                0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT   0x81AB
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT 0x81AC
#endif

#ifndef GL_EXT_draw_range_elements
#define GL_MAX_ELEMENTS_VERTICES_EXT      0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT       0x80E9
#endif

#ifndef GL_EXT_fog_coord
#define GL_FOG_COORDINATE_SOURCE_EXT      0x8450
#define GL_FOG_COORDINATE_EXT             0x8451
#define GL_FRAGMENT_DEPTH_EXT             0x8452
#define GL_CURRENT_FOG_COORDINATE_EXT     0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT  0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT 0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT 0x8456
#define GL_FOG_COORDINATE_ARRAY_EXT       0x8457
#endif

#ifndef GL_EXT_histogram
#define GL_HISTOGRAM_EXT                  0x8024
#define GL_PROXY_HISTOGRAM_EXT            0x8025
#define GL_HISTOGRAM_WIDTH_EXT            0x8026
#define GL_HISTOGRAM_FORMAT_EXT           0x8027
#define GL_HISTOGRAM_RED_SIZE_EXT         0x8028
#define GL_HISTOGRAM_GREEN_SIZE_EXT       0x8029
#define GL_HISTOGRAM_BLUE_SIZE_EXT        0x802A
#define GL_HISTOGRAM_ALPHA_SIZE_EXT       0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT   0x802C
#define GL_HISTOGRAM_SINK_EXT             0x802D
#define GL_MINMAX_EXT                     0x802E
#define GL_MINMAX_FORMAT_EXT              0x802F
#define GL_MINMAX_SINK_EXT                0x8030
#define GL_TABLE_TOO_LARGE_EXT            0x8031
#endif

#ifndef GL_EXT_misc_attribute
#endif

#ifndef GL_EXT_index_array_formats
#define GL_IUI_V2F_EXT                    0x81AD
#define GL_IUI_V3F_EXT                    0x81AE
#define GL_IUI_N3F_V2F_EXT                0x81AF
#define GL_IUI_N3F_V3F_EXT                0x81B0
#define GL_T2F_IUI_V2F_EXT                0x81B1
#define GL_T2F_IUI_V3F_EXT                0x81B2
#define GL_T2F_IUI_N3F_V2F_EXT            0x81B3
#define GL_T2F_IUI_N3F_V3F_EXT            0x81B4
#endif

#ifndef GL_EXT_index_func
#define GL_INDEX_TEST_EXT                 0x81B5
#define GL_INDEX_TEST_FUNC_EXT            0x81B6
#define GL_INDEX_TEST_REF_EXT             0x81B7
#endif

#ifndef GL_EXT_index_material
#define GL_INDEX_MATERIAL_EXT             0x81B8
#define GL_INDEX_MATERIAL_PARAMETER_EXT   0x81B9
#define GL_INDEX_MATERIAL_FACE_EXT        0x81BA
#endif

#ifndef GL_EXT_index_texture
#endif

#ifndef GL_EXT_light_texture
#define GL_FRAGMENT_MATERIAL_EXT          0x8349
#define GL_FRAGMENT_NORMAL_EXT            0x834A
#define GL_FRAGMENT_COLOR_EXT             0x834C
#define GL_ATTENUATION_EXT                0x834D
#define GL_SHADOW_ATTENUATION_EXT         0x834E
#define GL_TEXTURE_APPLICATION_MODE_EXT   0x834F
#define GL_TEXTURE_LIGHT_EXT              0x8350
#define GL_TEXTURE_MATERIAL_FACE_EXT      0x8351
#define GL_TEXTURE_MATERIAL_PARAMETER_EXT 0x8352
	/* reuse GL_FRAGMENT_DEPTH_EXT */
#endif

#ifndef GL_EXT_multi_draw_arrays
#endif

#ifndef GL_EXT_packed_pixels
#define GL_UNSIGNED_BYTE_3_3_2_EXT        0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT     0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT     0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT       0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT    0x8036
#endif

#ifndef GL_EXT_paletted_texture
#define GL_COLOR_INDEX1_EXT               0x80E2
#define GL_COLOR_INDEX2_EXT               0x80E3
#define GL_COLOR_INDEX4_EXT               0x80E4
#define GL_COLOR_INDEX8_EXT               0x80E5
#define GL_COLOR_INDEX12_EXT              0x80E6
#define GL_COLOR_INDEX16_EXT              0x80E7
#define GL_TEXTURE_INDEX_SIZE_EXT         0x80ED
#endif

#ifndef GL_EXT_pixel_transform
#define GL_PIXEL_TRANSFORM_2D_EXT         0x8330
#define GL_PIXEL_MAG_FILTER_EXT           0x8331
#define GL_PIXEL_MIN_FILTER_EXT           0x8332
#define GL_PIXEL_CUBIC_WEIGHT_EXT         0x8333
#define GL_CUBIC_EXT                      0x8334
#define GL_AVERAGE_EXT                    0x8335
#define GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 0x8336
#define GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 0x8337
#define GL_PIXEL_TRANSFORM_2D_MATRIX_EXT  0x8338
#endif

#ifndef GL_EXT_pixel_transform_color_table
#endif

#ifndef GL_EXT_point_parameters
#define GL_POINT_SIZE_MIN_EXT             0x8126
#define GL_POINT_SIZE_MAX_EXT             0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT  0x8128
#define GL_DISTANCE_ATTENUATION_EXT       0x8129
#endif

#ifndef GL_EXT_rescale_normal
#define GL_RESCALE_NORMAL_EXT             0x803A
#endif

#ifndef GL_EXT_secondary_color
#define GL_COLOR_SUM_EXT                  0x8458
#define GL_CURRENT_SECONDARY_COLOR_EXT    0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT 0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT 0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT 0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT 0x845D
#define GL_SECONDARY_COLOR_ARRAY_EXT      0x845E
#endif

#ifndef GL_EXT_separate_specular_color
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT  0x81F8
#define GL_SINGLE_COLOR_EXT               0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_EXT    0x81FA
#endif

#ifndef GL_EXT_shadow_funcs
#endif

#ifndef GL_EXT_shared_texture_palette
#define GL_SHARED_TEXTURE_PALETTE_EXT     0x81FB
#endif

#ifndef GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT      0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT        0x8911
#endif

#ifndef GL_EXT_stencil_wrap
#define GL_INCR_WRAP_EXT                  0x8507
#define GL_DECR_WRAP_EXT                  0x8508
#endif

#ifndef GL_EXT_subtexture
#endif

#ifndef GL_EXT_texture
#define GL_ALPHA4_EXT                     0x803B
#define GL_ALPHA8_EXT                     0x803C
#define GL_ALPHA12_EXT                    0x803D
#define GL_ALPHA16_EXT                    0x803E
#define GL_LUMINANCE4_EXT                 0x803F
#define GL_LUMINANCE8_EXT                 0x8040
#define GL_LUMINANCE12_EXT                0x8041
#define GL_LUMINANCE16_EXT                0x8042
#define GL_LUMINANCE4_ALPHA4_EXT          0x8043
#define GL_LUMINANCE6_ALPHA2_EXT          0x8044
#define GL_LUMINANCE8_ALPHA8_EXT          0x8045
#define GL_LUMINANCE12_ALPHA4_EXT         0x8046
#define GL_LUMINANCE12_ALPHA12_EXT        0x8047
#define GL_LUMINANCE16_ALPHA16_EXT        0x8048
#define GL_INTENSITY_EXT                  0x8049
#define GL_INTENSITY4_EXT                 0x804A
#define GL_INTENSITY8_EXT                 0x804B
#define GL_INTENSITY12_EXT                0x804C
#define GL_INTENSITY16_EXT                0x804D
#define GL_RGB2_EXT                       0x804E
#define GL_RGB4_EXT                       0x804F
#define GL_RGB5_EXT                       0x8050
#define GL_RGB8_EXT                       0x8051
#define GL_RGB10_EXT                      0x8052
#define GL_RGB12_EXT                      0x8053
#define GL_RGB16_EXT                      0x8054
#define GL_RGBA2_EXT                      0x8055
#define GL_RGBA4_EXT                      0x8056
#define GL_RGB5_A1_EXT                    0x8057
#define GL_RGBA8_EXT                      0x8058
#define GL_RGB10_A2_EXT                   0x8059
#define GL_RGBA12_EXT                     0x805A
#define GL_RGBA16_EXT                     0x805B
#define GL_TEXTURE_RED_SIZE_EXT           0x805C
#define GL_TEXTURE_GREEN_SIZE_EXT         0x805D
#define GL_TEXTURE_BLUE_SIZE_EXT          0x805E
#define GL_TEXTURE_ALPHA_SIZE_EXT         0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_EXT     0x8060
#define GL_TEXTURE_INTENSITY_SIZE_EXT     0x8061
#define GL_REPLACE_EXT                    0x8062
#define GL_PROXY_TEXTURE_1D_EXT           0x8063
#define GL_PROXY_TEXTURE_2D_EXT           0x8064
#define GL_TEXTURE_TOO_LARGE_EXT          0x8065
#endif

#ifndef GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif

#ifndef GL_EXT_texture_cube_map
#define GL_NORMAL_MAP_EXT                 0x8511
#define GL_REFLECTION_MAP_EXT             0x8512
#define GL_TEXTURE_CUBE_MAP_EXT           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT  0x851C
#endif

#ifndef GL_EXT_coordinate_frame
#define GL_TANGENT_ARRAY_EXT              0x8439
#define GL_BINORMAL_ARRAY_EXT             0x843A
#define GL_CURRENT_TANGENT_EXT            0x843B
#define GL_CURRENT_BINORMAL_EXT           0x843C
#define GL_TANGENT_ARRAY_TYPE_EXT         0x843E
#define GL_TANGENT_ARRAY_STRIDE_EXT       0x843F
#define GL_BINORMAL_ARRAY_TYPE_EXT        0x8440
#define GL_BINORMAL_ARRAY_STRIDE_EXT      0x8441
#define GL_TANGENT_ARRAY_POINTER_EXT      0x8442
#define GL_BINORMAL_ARRAY_POINTER_EXT     0x8443
#define GL_MAP1_TANGENT_EXT               0x8444
#define GL_MAP2_TANGENT_EXT               0x8445
#define GL_MAP1_BINORMAL_EXT              0x8446
#define GL_MAP2_BINORMAL_EXT              0x8447
#endif

#ifndef GL_EXT_texture_edge_clamp
#define CLAMP_TO_EDGE_EXT                 0x812F
#endif

#ifndef GL_EXT_texture_env_add
#endif

#ifndef GL_EXT_texture_env_combine
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_RGB_SCALE_EXT                  0x8573
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_CONSTANT_EXT                   0x8576
#define GL_PRIMARY_COLOR_EXT              0x8577
#define GL_PREVIOUS_EXT                   0x8578
#define GL_SOURCE0_RGB_EXT                0x8580
#define GL_SOURCE1_RGB_EXT                0x8581
#define GL_SOURCE2_RGB_EXT                0x8582
#define GL_SOURCE3_RGB_EXT                0x8583
#define GL_SOURCE4_RGB_EXT                0x8584
#define GL_SOURCE5_RGB_EXT                0x8585
#define GL_SOURCE6_RGB_EXT                0x8586
#define GL_SOURCE7_RGB_EXT                0x8587
#define GL_SOURCE0_ALPHA_EXT              0x8588
#define GL_SOURCE1_ALPHA_EXT              0x8589
#define GL_SOURCE2_ALPHA_EXT              0x858A
#define GL_SOURCE3_ALPHA_EXT              0x858B
#define GL_SOURCE4_ALPHA_EXT              0x858C
#define GL_SOURCE5_ALPHA_EXT              0x858D
#define GL_SOURCE6_ALPHA_EXT              0x858E
#define GL_SOURCE7_ALPHA_EXT              0x858F
#define GL_OPERAND0_RGB_EXT               0x8590
#define GL_OPERAND1_RGB_EXT               0x8591
#define GL_OPERAND2_RGB_EXT               0x8592
#define GL_OPERAND3_RGB_EXT               0x8593
#define GL_OPERAND4_RGB_EXT               0x8594
#define GL_OPERAND5_RGB_EXT               0x8595
#define GL_OPERAND6_RGB_EXT               0x8596
#define GL_OPERAND7_RGB_EXT               0x8597
#define GL_OPERAND0_ALPHA_EXT             0x8598
#define GL_OPERAND1_ALPHA_EXT             0x8599
#define GL_OPERAND2_ALPHA_EXT             0x859A
#define GL_OPERAND3_ALPHA_EXT             0x859B
#define GL_OPERAND4_ALPHA_EXT             0x859C
#define GL_OPERAND5_ALPHA_EXT             0x859D
#define GL_OPERAND6_ALPHA_EXT             0x859E
#define GL_OPERAND7_ALPHA_EXT             0x859F
#endif

#ifndef GL_EXT_texture_env_dot3
#define GL_DOT3_RGB_EXT                   0x8740
#define GL_DOT3_RGBA_EXT                  0x8741
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_EXT_texture_lod_bias
#define GL_MAX_TEXTURE_LOD_BIAS_EXT       0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT     0x8500
#define GL_TEXTURE_LOD_BIAS_EXT           0x8501
#endif

#ifndef GL_EXT_texture_object
#define GL_TEXTURE_PRIORITY_EXT           0x8066
#define GL_TEXTURE_RESIDENT_EXT           0x8067
#define GL_TEXTURE_1D_BINDING_EXT         0x8068
#define GL_TEXTURE_2D_BINDING_EXT         0x8069
#define GL_TEXTURE_3D_BINDING_EXT         0x806A
#endif

#ifndef GL_EXT_texture_perturb_normal
#define GL_PERTURB_EXT                    0x85AE
#define GL_TEXTURE_NORMAL_EXT             0x85AF
#endif

#ifndef GL_EXT_texture3D
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073
#endif

#ifndef GL_EXT_vertex_array
#define GL_VERTEX_ARRAY_EXT               0x8074
#define GL_NORMAL_ARRAY_EXT               0x8075
#define GL_COLOR_ARRAY_EXT                0x8076
#define GL_INDEX_ARRAY_EXT                0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT        0x8078
#define GL_EDGE_FLAG_ARRAY_EXT            0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT          0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT          0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT        0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT         0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT          0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT        0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT         0x8080
#define GL_COLOR_ARRAY_SIZE_EXT           0x8081
#define GL_COLOR_ARRAY_TYPE_EXT           0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT         0x8083
#define GL_COLOR_ARRAY_COUNT_EXT          0x8084
#define GL_INDEX_ARRAY_TYPE_EXT           0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT         0x8086
#define GL_INDEX_ARRAY_COUNT_EXT          0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT   0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT   0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT 0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT  0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT     0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT      0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT       0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT       0x808F
#define GL_COLOR_ARRAY_POINTER_EXT        0x8090
#define GL_INDEX_ARRAY_POINTER_EXT        0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT 0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT    0x8093
#endif

#ifndef GL_EXT_vertex_weighting
#define GL_MODELVIEW0_STACK_DEPTH_EXT     GL_MODELVIEW_STACK_DEPTH
#define GL_MODELVIEW1_STACK_DEPTH_EXT     0x8502
#define GL_MODELVIEW0_MATRIX_EXT          GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX1_EXT          0x8506
#define GL_VERTEX_WEIGHTING_EXT           0x8509
#define GL_MODELVIEW0_EXT                 GL_MODELVIEW
#define GL_MODELVIEW1_EXT                 0x850A
#define GL_CURRENT_VERTEX_WEIGHT_EXT      0x850B
#define GL_VERTEX_WEIGHT_ARRAY_EXT        0x850C
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT   0x850D
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT   0x850E
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT 0x850F
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT 0x8510
#endif

#ifndef GL_HP_occlusion_test
#define GL_OCCLUSION_TEST_HP              0x8165
#define GL_OCCLUSION_TEST_RESULT_HP       0x8166
#endif

#ifndef GL_HP_convolution_border_modes
#define GL_IGNORE_BORDER_HP               0x8150
#define GL_CONSTANT_BORDER_HP             0x8151
#define GL_REPLICATE_BORDER_HP            0x8153
#define GL_CONVOLUTION_BORDER_COLOR_HP    0x8154
#endif

#ifndef GL_HP_image_transform
#define GL_IMAGE_SCALE_X_HP               0x8155
#define GL_IMAGE_SCALE_Y_HP               0x8156
#define GL_IMAGE_TRANSLATE_X_HP           0x8157
#define GL_IMAGE_TRANSLATE_Y_HP           0x8158
#define GL_IMAGE_ROTATE_ANGLE_HP          0x8159
#define GL_IMAGE_ROTATE_ORIGIN_X_HP       0x815A
#define GL_IMAGE_ROTATE_ORIGIN_Y_HP       0x815B
#define GL_IMAGE_MAG_FILTER_HP            0x815C
#define GL_IMAGE_MIN_FILTER_HP            0x815D
#define GL_IMAGE_CUBIC_WEIGHT_HP          0x815E
#define GL_CUBIC_HP                       0x815F
#define GL_AVERAGE_HP                     0x8160
#define GL_IMAGE_TRANSFORM_2D_HP          0x8161
#define GL_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP 0x8162
#define GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP 0x8163
#endif

#ifndef GL_HP_texture_lighting
#define GL_TEXTURE_LIGHTING_MODE_HP       0x8167
#define GL_TEXTURE_POST_SPECULAR_HP       0x8168
#define GL_TEXTURE_PRE_SPECULAR_HP        0x8169
#endif

#ifndef GL_IBM_cull_vertex
#define GL_CULL_VERTEX_IBM                103050
#endif

#ifndef GL_IBM_multimode_draw_arrays
#endif

#ifndef GL_IBM_rasterpos_clip
#define GL_RASTER_POSITION_UNCLIPPED_IBM  0x19262
#endif

#ifndef GL_IBM_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_IBM            0x8370
#endif

#ifndef GL_IBM_vertex_array_lists
#define GL_VERTEX_ARRAY_LIST_IBM          103070
#define GL_NORMAL_ARRAY_LIST_IBM          103071
#define GL_COLOR_ARRAY_LIST_IBM           103072
#define GL_INDEX_ARRAY_LIST_IBM           103073
#define GL_TEXTURE_COORD_ARRAY_LIST_IBM   103074
#define GL_EDGE_FLAG_ARRAY_LIST_IBM       103075
#define GL_FOG_COORDINATE_ARRAY_LIST_IBM  103076
#define GL_SECONDARY_COLOR_ARRAY_LIST_IBM 103077
#define GL_VERTEX_ARRAY_LIST_STRIDE_IBM   103080
#define GL_NORMAL_ARRAY_LIST_STRIDE_IBM   103081
#define GL_COLOR_ARRAY_LIST_STRIDE_IBM    103082
#define GL_INDEX_ARRAY_LIST_STRIDE_IBM    103083
#define GL_TEXTURE_COORD_ARRAY_LIST_STRIDE_IBM 103084
#define GL_EDGE_FLAG_ARRAY_LIST_STRIDE_IBM 103085
#define GL_FOG_COORDINATE_ARRAY_LIST_STRIDE_IBM 103086
#define GL_SECONDARY_COLOR_ARRAY_LIST_STRIDE_IBM 103087
#endif

#ifndef GL_INGR_color_clamp
#define GL_RED_MIN_CLAMP_INGR             0x8560
#define GL_GREEN_MIN_CLAMP_INGR           0x8561
#define GL_BLUE_MIN_CLAMP_INGR            0x8562
#define GL_ALPHA_MIN_CLAMP_INGR           0x8563
#define GL_RED_MAX_CLAMP_INGR             0x8564
#define GL_GREEN_MAX_CLAMP_INGR           0x8565
#define GL_BLUE_MAX_CLAMP_INGR            0x8566
#define GL_ALPHA_MAX_CLAMP_INGR           0x8567
#endif

#ifndef GL_INGR_interlace_read
#define GL_INTERLACE_READ_INGR            0x8568
#endif

#ifndef GL_INGR_palette_buffer
#endif

#ifndef GL_INTEL_parallel_arrays
#define GL_PARALLEL_ARRAYS_INTEL          0x83F4
#define GL_VERTEX_ARRAY_PARALLEL_POINTERS_INTEL 0x83F5
#define GL_NORMAL_ARRAY_PARALLEL_POINTERS_INTEL 0x83F6
#define GL_COLOR_ARRAY_PARALLEL_POINTERS_INTEL 0x83F7
#define GL_TEXTURE_COORD_ARRAY_PARALLEL_POINTERS_INTEL 0x83F8
#endif

#ifndef GL_INTEL_texture_scissor
#endif

#ifndef GL_MESA_resize_buffers
#endif

#ifndef GL_MESA_window_pos
#endif

#ifndef GL_NV_blend_square
#endif

#ifndef GL_NV_copy_depth_to_color
#define GL_DEPTH_STENCIL_TO_RGBA_NV       0x886E
#define GL_DEPTH_STENCIL_TO_BGRA_NV       0x886F
#endif

#ifndef GL_NV_depth_clamp
#define GL_DEPTH_CLAMP_NV                 0x864F
#endif

#ifndef GL_NV_element_array
#define GL_ELEMENT_ARRAY_TYPE_NV          0x8769
#define GL_ELEMENT_ARRAY_POINTER_NV       0x876A
#endif

#ifndef GL_NV_fence
#define GL_ALL_COMPLETED_NV               0x84F2
#define GL_FENCE_STATUS_NV                0x84F3
#define GL_FENCE_CONDITION_NV             0x84F4
#endif

#ifndef GL_NV_float_buffer
#define GL_FLOAT_R_NV                     0x8880
#define GL_FLOAT_RG_NV                    0x8881
#define GL_FLOAT_RGB_NV                   0x8882
#define GL_FLOAT_RGBA_NV                  0x8883
#define GL_FLOAT_R16_NV                   0x8884
#define GL_FLOAT_R32_NV                   0x8885
#define GL_FLOAT_RG16_NV                  0x8886
#define GL_FLOAT_RG32_NV                  0x8887
#define GL_FLOAT_RGB16_NV                 0x8888
#define GL_FLOAT_RGB32_NV                 0x8889
#define GL_FLOAT_RGBA16_NV                0x888A
#define GL_FLOAT_RGBA32_NV                0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV    0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV     0x888D
#define GL_FLOAT_RGBA_MODE_NV             0x888E
#endif

#ifndef GL_NV_fog_distance
#define GL_FOG_DISTANCE_MODE_NV           0x855A
#define GL_EYE_RADIAL_NV                  0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV          0x855C
	/* reuse GL_EYE_PLANE */
#endif

#ifndef GL_NV_fragment_program
#define GL_FRAGMENT_PROGRAM_NV            0x8870
#define GL_MAX_TEXTURE_COORDS_NV          0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV     0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV    0x8873
#define GL_PROGRAM_ERROR_STRING_NV        0x8874
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV 0x8868
#endif

#ifndef GL_NV_half_float
#define GL_HALF_FLOAT_NV                  0x140B
#endif

#ifndef GL_NV_light_max_exponent
#define GL_MAX_SHININESS_NV               0x8504
#define GL_MAX_SPOT_EXPONENT_NV           0x8505
#endif

#ifndef GL_NV_multisample_filter_hint
#define GL_MULTISAMPLE_FILTER_HINT_NV     0x8534
#endif

#ifndef GL_NV_occlusion_query
#define GL_PIXEL_COUNTER_BITS_NV          0x8864
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV  0x8865
#define GL_PIXEL_COUNT_NV                 0x8866
#define GL_PIXEL_COUNT_AVAILABLE_NV       0x8867
#endif

#ifndef GL_NV_packed_depth_stencil
#define GL_DEPTH_STENCIL_NV               0x84F9
#define GL_UNSIGNED_INT_24_8_NV           0x84FA
#endif

#ifndef GL_NV_pixel_data_range
#define GL_WRITE_PIXEL_DATA_RANGE_NV      0x8878
#define GL_READ_PIXEL_DATA_RANGE_NV       0x8879
#define GL_WRITE_PIXEL_DATA_RANGE_LENGTH_NV 0x887A
#define GL_READ_PIXEL_DATA_RANGE_LENGTH_NV 0x887B
#define GL_WRITE_PIXEL_DATA_RANGE_POINTER_NV 0x887C
#define GL_READ_PIXEL_DATA_RANGE_POINTER_NV 0x887D
#endif

#ifndef GL_NV_point_sprite
#define GL_POINT_SPRITE_NV                0x8861
#define GL_COORD_REPLACE_NV               0x8862
#define GL_POINT_SPRITE_R_MODE_NV         0x8863
#endif

#ifndef GL_NV_primitive_restart
#define GL_PRIMITIVE_RESTART_NV           0x8558
#define GL_PRIMITIVE_RESTART_INDEX_NV     0x8559
#endif

#ifndef GL_NV_register_combiners
#define GL_REGISTER_COMBINERS_NV          0x8522
#define GL_VARIABLE_A_NV                  0x8523
#define GL_VARIABLE_B_NV                  0x8524
#define GL_VARIABLE_C_NV                  0x8525
#define GL_VARIABLE_D_NV                  0x8526
#define GL_VARIABLE_E_NV                  0x8527
#define GL_VARIABLE_F_NV                  0x8528
#define GL_VARIABLE_G_NV                  0x8529
#define GL_CONSTANT_COLOR0_NV             0x852A
#define GL_CONSTANT_COLOR1_NV             0x852B
#define GL_PRIMARY_COLOR_NV               0x852C
#define GL_SECONDARY_COLOR_NV             0x852D
#define GL_SPARE0_NV                      0x852E
#define GL_SPARE1_NV                      0x852F
#define GL_DISCARD_NV                     0x8530
#define GL_E_TIMES_F_NV                   0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV 0x8532
#define GL_UNSIGNED_IDENTITY_NV           0x8536
#define GL_UNSIGNED_INVERT_NV             0x8537
#define GL_EXPAND_NORMAL_NV               0x8538
#define GL_EXPAND_NEGATE_NV               0x8539
#define GL_HALF_BIAS_NORMAL_NV            0x853A
#define GL_HALF_BIAS_NEGATE_NV            0x853B
#define GL_SIGNED_IDENTITY_NV             0x853C
#define GL_SIGNED_NEGATE_NV               0x853D
#define GL_SCALE_BY_TWO_NV                0x853E
#define GL_SCALE_BY_FOUR_NV               0x853F
#define GL_SCALE_BY_ONE_HALF_NV           0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV   0x8541
#define GL_COMBINER_INPUT_NV              0x8542
#define GL_COMBINER_MAPPING_NV            0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV    0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV     0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV     0x8546
#define GL_COMBINER_MUX_SUM_NV            0x8547
#define GL_COMBINER_SCALE_NV              0x8548
#define GL_COMBINER_BIAS_NV               0x8549
#define GL_COMBINER_AB_OUTPUT_NV          0x854A
#define GL_COMBINER_CD_OUTPUT_NV          0x854B
#define GL_COMBINER_SUM_OUTPUT_NV         0x854C
#define GL_MAX_GENERAL_COMBINERS_NV       0x854D
#define GL_NUM_GENERAL_COMBINERS_NV       0x854E
#define GL_COLOR_SUM_CLAMP_NV             0x854F
#define GL_COMBINER0_NV                   0x8550
#define GL_COMBINER1_NV                   0x8551
#define GL_COMBINER2_NV                   0x8552
#define GL_COMBINER3_NV                   0x8553
#define GL_COMBINER4_NV                   0x8554
#define GL_COMBINER5_NV                   0x8555
#define GL_COMBINER6_NV                   0x8556
#define GL_COMBINER7_NV                   0x8557
	/* reuse GL_TEXTURE0_ARB */
	/* reuse GL_TEXTURE1_ARB */
	/* reuse GL_ZERO */
	/* reuse GL_NONE */
	/* reuse GL_FOG */
#endif

#ifndef GL_NV_register_combiners2
#define GL_PER_STAGE_CONSTANTS_NV         0x8535
#endif

#ifndef GL_NV_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_NV       0x8910
#define GL_ACTIVE_STENCIL_FACE_NV         0x8911
#endif

#ifndef GL_NV_texgen_emboss
#define GL_EMBOSS_LIGHT_NV                0x855D
#define GL_EMBOSS_CONSTANT_NV             0x855E
#define GL_EMBOSS_MAP_NV                  0x855F
#endif

#ifndef GL_NV_texgen_reflection
#define GL_NORMAL_MAP_NV                  0x8511
#define GL_REFLECTION_MAP_NV              0x8512
#endif

#ifndef GL_NV_texture_compression_vtc
#endif

#ifndef GL_NV_texture_env_combine4
#define GL_COMBINE4_NV                    0x8503
#define GL_SOURCE3_RGB_NV                 0x8583
#define GL_SOURCE3_ALPHA_NV               0x858B
#define GL_OPERAND3_RGB_NV                0x8593
#define GL_OPERAND3_ALPHA_NV              0x859B
#endif

#ifndef GL_NV_texture_rectangle
#define GL_TEXTURE_RECTANGLE_NV           0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV   0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV     0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV  0x84F8
#endif

#ifndef GL_NV_texture_shader
#define GL_OFFSET_TEXTURE_RECTANGLE_NV    0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV 0x864D
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV 0x864E
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV      0x86DA
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV  0x86DB
#define GL_DSDT_MAG_INTENSITY_NV          0x86DC
#define GL_SHADER_CONSISTENT_NV           0x86DD
#define GL_TEXTURE_SHADER_NV              0x86DE
#define GL_SHADER_OPERATION_NV            0x86DF
#define GL_CULL_MODES_NV                  0x86E0
#define GL_OFFSET_TEXTURE_MATRIX_NV       0x86E1
#define GL_OFFSET_TEXTURE_SCALE_NV        0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV         0x86E3
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV    GL_OFFSET_TEXTURE_MATRIX_NV
#define GL_OFFSET_TEXTURE_2D_SCALE_NV     GL_OFFSET_TEXTURE_SCALE_NV
#define GL_OFFSET_TEXTURE_2D_BIAS_NV      GL_OFFSET_TEXTURE_BIAS_NV
#define GL_PREVIOUS_TEXTURE_INPUT_NV      0x86E4
#define GL_CONST_EYE_NV                   0x86E5
#define GL_PASS_THROUGH_NV                0x86E6
#define GL_CULL_FRAGMENT_NV               0x86E7
#define GL_OFFSET_TEXTURE_2D_NV           0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV     0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV     0x86EA
#define GL_DOT_PRODUCT_NV                 0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV   0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV      0x86EE
#define GL_DOT_PRODUCT_TEXTURE_3D_NV      0x86EF
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV 0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV 0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV 0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV 0x86F3
#define GL_HILO_NV                        0x86F4
#define GL_DSDT_NV                        0x86F5
#define GL_DSDT_MAG_NV                    0x86F6
#define GL_DSDT_MAG_VIB_NV                0x86F7
#define GL_HILO16_NV                      0x86F8
#define GL_SIGNED_HILO_NV                 0x86F9
#define GL_SIGNED_HILO16_NV               0x86FA
#define GL_SIGNED_RGBA_NV                 0x86FB
#define GL_SIGNED_RGBA8_NV                0x86FC
#define GL_SIGNED_RGB_NV                  0x86FE
#define GL_SIGNED_RGB8_NV                 0x86FF
#define GL_SIGNED_LUMINANCE_NV            0x8701
#define GL_SIGNED_LUMINANCE8_NV           0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV      0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV    0x8704
#define GL_SIGNED_ALPHA_NV                0x8705
#define GL_SIGNED_ALPHA8_NV               0x8706
#define GL_SIGNED_INTENSITY_NV            0x8707
#define GL_SIGNED_INTENSITY8_NV           0x8708
#define GL_DSDT8_NV                       0x8709
#define GL_DSDT8_MAG8_NV                  0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV       0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV   0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV 0x870D
#define GL_HI_SCALE_NV                    0x870E
#define GL_LO_SCALE_NV                    0x870F
#define GL_DS_SCALE_NV                    0x8710
#define GL_DT_SCALE_NV                    0x8711
#define GL_MAGNITUDE_SCALE_NV             0x8712
#define GL_VIBRANCE_SCALE_NV              0x8713
#define GL_HI_BIAS_NV                     0x8714
#define GL_LO_BIAS_NV                     0x8715
#define GL_DS_BIAS_NV                     0x8716
#define GL_DT_BIAS_NV                     0x8717
#define GL_MAGNITUDE_BIAS_NV              0x8718
#define GL_VIBRANCE_BIAS_NV               0x8719
#define GL_TEXTURE_BORDER_VALUES_NV       0x871A
#define GL_TEXTURE_HI_SIZE_NV             0x871B
#define GL_TEXTURE_LO_SIZE_NV             0x871C
#define GL_TEXTURE_DS_SIZE_NV             0x871D
#define GL_TEXTURE_DT_SIZE_NV             0x871E
#define GL_TEXTURE_MAG_SIZE_NV            0x871F
#endif

#ifndef GL_NV_texture_shader2
#define GL_DOT_PRODUCT_TEXTURE_3D_NV      0x86EF
#endif

#ifndef GL_NV_texture_shader3
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV 0x8850
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_SCALE_NV 0x8851
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8852
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_SCALE_NV 0x8853
#define GL_OFFSET_HILO_TEXTURE_2D_NV      0x8854
#define GL_OFFSET_HILO_TEXTURE_RECTANGLE_NV 0x8855
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_2D_NV 0x8856
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8857
#define GL_DEPENDENT_HILO_TEXTURE_2D_NV   0x8858
#define GL_DEPENDENT_RGB_TEXTURE_3D_NV    0x8859
#define GL_DEPENDENT_RGB_TEXTURE_CUBE_MAP_NV 0x885A
#define GL_DOT_PRODUCT_PASS_THROUGH_NV    0x885B
#define GL_DOT_PRODUCT_TEXTURE_1D_NV      0x885C
#define GL_DOT_PRODUCT_AFFINE_DEPTH_REPLACE_NV 0x885D
#define GL_HILO8_NV                       0x885E
#define GL_SIGNED_HILO8_NV                0x885F
#define GL_FORCE_BLUE_TO_ONE_NV           0x8860
#endif

#ifndef GL_NV_vertex_array_range
#define GL_VERTEX_ARRAY_RANGE_NV          0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV   0x851E
#define GL_VERTEX_ARRAY_RANGE_VALID_NV    0x851F
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV 0x8520
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV  0x8521
#endif

#ifndef GL_NV_vertex_array_range2
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV 0x8533
#endif

#ifndef GL_NV_vertex_program
#define GL_VERTEX_PROGRAM_NV              0x8620
#define GL_VERTEX_STATE_PROGRAM_NV        0x8621
#define GL_ATTRIB_ARRAY_SIZE_NV           0x8623
#define GL_ATTRIB_ARRAY_STRIDE_NV         0x8624
#define GL_ATTRIB_ARRAY_TYPE_NV           0x8625
#define GL_CURRENT_ATTRIB_NV              0x8626
#define GL_PROGRAM_LENGTH_NV              0x8627
#define GL_PROGRAM_STRING_NV              0x8628
#define GL_MODELVIEW_PROJECTION_NV        0x8629
#define GL_IDENTITY_NV                    0x862A
#define GL_INVERSE_NV                     0x862B
#define GL_TRANSPOSE_NV                   0x862C
#define GL_INVERSE_TRANSPOSE_NV           0x862D
#define GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV 0x862E
#define GL_MAX_TRACK_MATRICES_NV          0x862F
#define GL_MATRIX0_NV                     0x8630
#define GL_MATRIX1_NV                     0x8631
#define GL_MATRIX2_NV                     0x8632
#define GL_MATRIX3_NV                     0x8633
#define GL_MATRIX4_NV                     0x8634
#define GL_MATRIX5_NV                     0x8635
#define GL_MATRIX6_NV                     0x8636
#define GL_MATRIX7_NV                     0x8637
#define GL_CURRENT_MATRIX_STACK_DEPTH_NV  0x8640
#define GL_CURRENT_MATRIX_NV              0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_NV   0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_NV     0x8643
#define GL_PROGRAM_PARAMETER_NV           0x8644
#define GL_ATTRIB_ARRAY_POINTER_NV        0x8645
#define GL_PROGRAM_TARGET_NV              0x8646
#define GL_PROGRAM_RESIDENT_NV            0x8647
#define GL_TRACK_MATRIX_NV                0x8648
#define GL_TRACK_MATRIX_TRANSFORM_NV      0x8649
#define GL_VERTEX_PROGRAM_BINDING_NV      0x864A
#define GL_PROGRAM_ERROR_POSITION_NV      0x864B
#define GL_VERTEX_ATTRIB_ARRAY0_NV        0x8650
#define GL_VERTEX_ATTRIB_ARRAY1_NV        0x8651
#define GL_VERTEX_ATTRIB_ARRAY2_NV        0x8652
#define GL_VERTEX_ATTRIB_ARRAY3_NV        0x8653
#define GL_VERTEX_ATTRIB_ARRAY4_NV        0x8654
#define GL_VERTEX_ATTRIB_ARRAY5_NV        0x8655
#define GL_VERTEX_ATTRIB_ARRAY6_NV        0x8656
#define GL_VERTEX_ATTRIB_ARRAY7_NV        0x8657
#define GL_VERTEX_ATTRIB_ARRAY8_NV        0x8658
#define GL_VERTEX_ATTRIB_ARRAY9_NV        0x8659
#define GL_VERTEX_ATTRIB_ARRAY10_NV       0x865A
#define GL_VERTEX_ATTRIB_ARRAY11_NV       0x865B
#define GL_VERTEX_ATTRIB_ARRAY12_NV       0x865C
#define GL_VERTEX_ATTRIB_ARRAY13_NV       0x865D
#define GL_VERTEX_ATTRIB_ARRAY14_NV       0x865E
#define GL_VERTEX_ATTRIB_ARRAY15_NV       0x865F
#define GL_MAP1_VERTEX_ATTRIB0_4_NV       0x8660
#define GL_MAP1_VERTEX_ATTRIB1_4_NV       0x8661
#define GL_MAP1_VERTEX_ATTRIB2_4_NV       0x8662
#define GL_MAP1_VERTEX_ATTRIB3_4_NV       0x8663
#define GL_MAP1_VERTEX_ATTRIB4_4_NV       0x8664
#define GL_MAP1_VERTEX_ATTRIB5_4_NV       0x8665
#define GL_MAP1_VERTEX_ATTRIB6_4_NV       0x8666
#define GL_MAP1_VERTEX_ATTRIB7_4_NV       0x8667
#define GL_MAP1_VERTEX_ATTRIB8_4_NV       0x8668
#define GL_MAP1_VERTEX_ATTRIB9_4_NV       0x8669
#define GL_MAP1_VERTEX_ATTRIB10_4_NV      0x866A
#define GL_MAP1_VERTEX_ATTRIB11_4_NV      0x866B
#define GL_MAP1_VERTEX_ATTRIB12_4_NV      0x866C
#define GL_MAP1_VERTEX_ATTRIB13_4_NV      0x866D
#define GL_MAP1_VERTEX_ATTRIB14_4_NV      0x866E
#define GL_MAP1_VERTEX_ATTRIB15_4_NV      0x866F
#define GL_MAP2_VERTEX_ATTRIB0_4_NV       0x8670
#define GL_MAP2_VERTEX_ATTRIB1_4_NV       0x8671
#define GL_MAP2_VERTEX_ATTRIB2_4_NV       0x8672
#define GL_MAP2_VERTEX_ATTRIB3_4_NV       0x8673
#define GL_MAP2_VERTEX_ATTRIB4_4_NV       0x8674
#define GL_MAP2_VERTEX_ATTRIB5_4_NV       0x8675
#define GL_MAP2_VERTEX_ATTRIB6_4_NV       0x8676
#define GL_MAP2_VERTEX_ATTRIB7_4_NV       0x8677
#define GL_MAP2_VERTEX_ATTRIB8_4_NV       0x8678
#define GL_MAP2_VERTEX_ATTRIB9_4_NV       0x8679
#define GL_MAP2_VERTEX_ATTRIB10_4_NV      0x867A
#define GL_MAP2_VERTEX_ATTRIB11_4_NV      0x867B
#define GL_MAP2_VERTEX_ATTRIB12_4_NV      0x867C
#define GL_MAP2_VERTEX_ATTRIB13_4_NV      0x867D
#define GL_MAP2_VERTEX_ATTRIB14_4_NV      0x867E
#define GL_MAP2_VERTEX_ATTRIB15_4_NV      0x867F
#endif

#ifndef GL_NV_vertex_program1_1
#endif

#ifndef GL_NV_vertex_program2
#endif

#ifndef GL_PGI_misc_hints
#define GL_PREFER_DOUBLEBUFFER_HINT_PGI   0x1A1F8
#define GL_CONSERVE_MEMORY_HINT_PGI       0x1A1FD
#define GL_RECLAIM_MEMORY_HINT_PGI        0x1A1FE
#define GL_NATIVE_GRAPHICS_HANDLE_PGI     0x1A202
#define GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI 0x1A203
#define GL_NATIVE_GRAPHICS_END_HINT_PGI   0x1A204
#define GL_ALWAYS_FAST_HINT_PGI           0x1A20C
#define GL_ALWAYS_SOFT_HINT_PGI           0x1A20D
#define GL_ALLOW_DRAW_OBJ_HINT_PGI        0x1A20E
#define GL_ALLOW_DRAW_WIN_HINT_PGI        0x1A20F
#define GL_ALLOW_DRAW_FRG_HINT_PGI        0x1A210
#define GL_ALLOW_DRAW_MEM_HINT_PGI        0x1A211
#define GL_STRICT_DEPTHFUNC_HINT_PGI      0x1A216
#define GL_STRICT_LIGHTING_HINT_PGI       0x1A217
#define GL_STRICT_SCISSOR_HINT_PGI        0x1A218
#define GL_FULL_STIPPLE_HINT_PGI          0x1A219
#define GL_CLIP_NEAR_HINT_PGI             0x1A220
#define GL_CLIP_FAR_HINT_PGI              0x1A221
#define GL_WIDE_LINE_HINT_PGI             0x1A222
#define GL_BACK_NORMALS_HINT_PGI          0x1A223
#endif

#ifndef GL_PGI_vertex_hints
#define GL_VERTEX_DATA_HINT_PGI           0x1A22A
#define GL_VERTEX_CONSISTENT_HINT_PGI     0x1A22B
#define GL_MATERIAL_SIDE_HINT_PGI         0x1A22C
#define GL_MAX_VERTEX_HINT_PGI            0x1A22D
#define GL_COLOR3_BIT_PGI                 0x00010000
#define GL_COLOR4_BIT_PGI                 0x00020000
#define GL_EDGEFLAG_BIT_PGI               0x00040000
#define GL_INDEX_BIT_PGI                  0x00080000
#define GL_MAT_AMBIENT_BIT_PGI            0x00100000
#define GL_MAT_AMBIENT_AND_DIFFUSE_BIT_PGI 0x00200000
#define GL_MAT_DIFFUSE_BIT_PGI            0x00400000
#define GL_MAT_EMISSION_BIT_PGI           0x00800000
#define GL_MAT_COLOR_INDEXES_BIT_PGI      0x01000000
#define GL_MAT_SHININESS_BIT_PGI          0x02000000
#define GL_MAT_SPECULAR_BIT_PGI           0x04000000
#define GL_NORMAL_BIT_PGI                 0x08000000
#define GL_TEXCOORD1_BIT_PGI              0x10000000
#define GL_TEXCOORD2_BIT_PGI              0x20000000
#define GL_TEXCOORD3_BIT_PGI              0x40000000
#define GL_TEXCOORD4_BIT_PGI              0x80000000
#define GL_VERTEX23_BIT_PGI               0x00000004
#define GL_VERTEX4_BIT_PGI                0x00000008
#endif

#ifndef GL_REND_screen_coordinates
#define GL_SCREEN_COORDINATES_REND        0x8490
#define GL_INVERTED_SCREEN_W_REND         0x8491
#endif

#ifndef GL_SGI_color_matrix
#define GL_COLOR_MATRIX_SGI               0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI   0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI 0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI 0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI 0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI 0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI 0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI 0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI 0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI 0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI 0x80BB
#endif

#ifndef GL_SGI_color_table
#define GL_COLOR_TABLE_SGI                0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D2
#define GL_PROXY_COLOR_TABLE_SGI          0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D5
#define GL_COLOR_TABLE_SCALE_SGI          0x80D6
#define GL_COLOR_TABLE_BIAS_SGI           0x80D7
#define GL_COLOR_TABLE_FORMAT_SGI         0x80D8
#define GL_COLOR_TABLE_WIDTH_SGI          0x80D9
#define GL_COLOR_TABLE_RED_SIZE_SGI       0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_SGI     0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_SGI      0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI     0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI 0x80DF
#endif

#ifndef GL_SGI_texture_color_table
#define GL_TEXTURE_COLOR_TABLE_SGI        0x80BC
#define GL_PROXY_TEXTURE_COLOR_TABLE_SGI  0x80BD
#endif

#ifndef GL_SGIS_detail_texture
#define GL_DETAIL_TEXTURE_2D_SGIS         0x8095
#define GL_DETAIL_TEXTURE_2D_BINDING_SGIS 0x8096
#define GL_LINEAR_DETAIL_SGIS             0x8097
#define GL_LINEAR_DETAIL_ALPHA_SGIS       0x8098
#define GL_LINEAR_DETAIL_COLOR_SGIS       0x8099
#define GL_DETAIL_TEXTURE_LEVEL_SGIS      0x809A
#define GL_DETAIL_TEXTURE_MODE_SGIS       0x809B
#define GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS 0x809C
#endif

#ifndef GL_SGIS_fog_function
#define GL_FOG_FUNC_SGIS                  0x812A
#define GL_FOG_FUNC_POINTS_SGIS           0x812B
#define GL_MAX_FOG_FUNC_POINTS_SGIS       0x812C
#endif

#ifndef GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif

#ifndef GL_SGIS_multisample
#define GL_MULTISAMPLE_SGIS               0x809D
#define GL_SAMPLE_ALPHA_TO_MASK_SGIS      0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_SGIS       0x809F
#define GL_SAMPLE_MASK_SGIS               0x80A0
#define GL_1PASS_SGIS                     0x80A1
#define GL_2PASS_0_SGIS                   0x80A2
#define GL_2PASS_1_SGIS                   0x80A3
#define GL_4PASS_0_SGIS                   0x80A4
#define GL_4PASS_1_SGIS                   0x80A5
#define GL_4PASS_2_SGIS                   0x80A6
#define GL_4PASS_3_SGIS                   0x80A7
#define GL_SAMPLE_BUFFERS_SGIS            0x80A8
#define GL_SAMPLES_SGIS                   0x80A9
#define GL_SAMPLE_MASK_VALUE_SGIS         0x80AA
#define GL_SAMPLE_MASK_INVERT_SGIS        0x80AB
#define GL_SAMPLE_PATTERN_SGIS            0x80AC
#endif

#ifndef GL_SGIS_pixel_texture
#define GL_PIXEL_TEXTURE_SGIS             0x8353
#define GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS 0x8354
#define GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS 0x8355
#define GL_PIXEL_GROUP_COLOR_SGIS         0x8356
#endif

#ifndef GL_SGIS_sharpen_texture
#define GL_LINEAR_SHARPEN_SGIS            0x80AD
#define GL_LINEAR_SHARPEN_ALPHA_SGIS      0x80AE
#define GL_LINEAR_SHARPEN_COLOR_SGIS      0x80AF
#define GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS 0x80B0
#endif

#ifndef GL_SGIS_texture_border_clamp
#define GL_CLAMP_TO_BORDER_SGIS           0x812D
#endif

#ifndef GL_SGIS_texture_color_mask
#define GL_TEXTURE_COLOR_WRITEMASK_SGIS   0x81EF
#endif

#ifndef GL_SGIS_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_SGIS             0x812F
#endif

#ifndef GL_SGIS_texture_filter4
#define GL_FILTER4_SGIS                   0x8146
#define GL_TEXTURE_FILTER4_SIZE_SGIS      0x8147
#endif

#ifndef GL_SGIS_texture_lod
#define GL_TEXTURE_MIN_LOD_SGIS           0x813A
#define GL_TEXTURE_MAX_LOD_SGIS           0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS        0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS         0x813D
#endif

#ifndef GL_SGIS_texture_select
#define GL_DUAL_ALPHA4_SGIS               0x8110
#define GL_DUAL_ALPHA8_SGIS               0x8111
#define GL_DUAL_ALPHA12_SGIS              0x8112
#define GL_DUAL_ALPHA16_SGIS              0x8113
#define GL_DUAL_LUMINANCE4_SGIS           0x8114
#define GL_DUAL_LUMINANCE8_SGIS           0x8115
#define GL_DUAL_LUMINANCE12_SGIS          0x8116
#define GL_DUAL_LUMINANCE16_SGIS          0x8117
#define GL_DUAL_INTENSITY4_SGIS           0x8118
#define GL_DUAL_INTENSITY8_SGIS           0x8119
#define GL_DUAL_INTENSITY12_SGIS          0x811A
#define GL_DUAL_INTENSITY16_SGIS          0x811B
#define GL_DUAL_LUMINANCE_ALPHA4_SGIS     0x811C
#define GL_DUAL_LUMINANCE_ALPHA8_SGIS     0x811D
#define GL_QUAD_ALPHA4_SGIS               0x811E
#define GL_QUAD_ALPHA8_SGIS               0x811F
#define GL_QUAD_LUMINANCE4_SGIS           0x8120
#define GL_QUAD_LUMINANCE8_SGIS           0x8121
#define GL_QUAD_INTENSITY4_SGIS           0x8122
#define GL_QUAD_INTENSITY8_SGIS           0x8123
#define GL_DUAL_TEXTURE_SELECT_SGIS       0x8124
#define GL_QUAD_TEXTURE_SELECT_SGIS       0x8125
#endif

#ifndef GL_SGIS_texture4D
#define GL_PACK_SKIP_VOLUMES_SGIS         0x8130
#define GL_PACK_IMAGE_DEPTH_SGIS          0x8131
#define GL_UNPACK_SKIP_VOLUMES_SGIS       0x8132
#define GL_UNPACK_IMAGE_DEPTH_SGIS        0x8133
#define GL_TEXTURE_4D_SGIS                0x8134
#define GL_PROXY_TEXTURE_4D_SGIS          0x8135
#define GL_TEXTURE_4DSIZE_SGIS            0x8136
#define GL_TEXTURE_WRAP_Q_SGIS            0x8137
#define GL_MAX_4D_TEXTURE_SIZE_SGIS       0x8138
#define GL_TEXTURE_4D_BINDING_SGIS        0x814F
#endif

#ifndef GL_SGIX_blend_alpha_minmax
#define GL_ALPHA_MIN_SGIX                 0x8320
#define GL_ALPHA_MAX_SGIX                 0x8321
#endif

#ifndef GL_SGIX_clipmap
#define GL_LINEAR_CLIPMAP_LINEAR_SGIX     0x8170
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX    0x8171
#define GL_TEXTURE_CLIPMAP_FRAME_SGIX     0x8172
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX    0x8173
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8174
#define GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX 0x8175
#define GL_TEXTURE_CLIPMAP_DEPTH_SGIX     0x8176
#define GL_MAX_CLIPMAP_DEPTH_SGIX         0x8177
#define GL_MAX_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8178
#define GL_NEAREST_CLIPMAP_NEAREST_SGIX   0x844D
#define GL_NEAREST_CLIPMAP_LINEAR_SGIX    0x844E
#define GL_LINEAR_CLIPMAP_NEAREST_SGIX    0x844F
#endif

#ifndef GL_SGIX_depth_texture
#define GL_DEPTH_COMPONENT16_SGIX         0x81A5
#define GL_DEPTH_COMPONENT24_SGIX         0x81A6
#define GL_DEPTH_COMPONENT32_SGIX         0x81A7
#endif

#ifndef GL_SGIX_flush_raster
#endif

#ifndef GL_SGIX_fog_offset
#define GL_FOG_OFFSET_SGIX                0x8198
#define GL_FOG_OFFSET_VALUE_SGIX          0x8199
#endif

#ifndef GL_SGIX_framezoom
#define GL_FRAMEZOOM_SGIX                 0x818B
#define GL_FRAMEZOOM_FACTOR_SGIX          0x818C
#define GL_MAX_FRAMEZOOM_FACTOR_SGIX      0x818D
#endif

#ifndef GL_SGIX_instruments
#define GL_INSTRUMENT_BUFFER_POINTER_SGIX 0x8180
#define GL_INSTRUMENT_MEASUREMENTS_SGIX   0x8181
#endif

#ifndef GL_SGIX_interlace
#define GL_INTERLACE_SGIX                 0x8094
#endif

#ifndef GL_SGIX_ir_instrument1
#define GL_IR_INSTRUMENT1_SGIX            0x817F
#endif

#ifndef GL_SGIX_list_priority
#define GL_LIST_PRIORITY_SGIX             0x8182
#endif

#ifndef GL_SGIX_pixel_texture
#define GL_PIXEL_TEX_GEN_SGIX             0x8139
#define GL_PIXEL_TEX_GEN_MODE_SGIX        0x832B
#endif

#ifndef GL_SGIX_reference_plane
#define GL_REFERENCE_PLANE_SGIX           0x817D
#define GL_REFERENCE_PLANE_EQUATION_SGIX  0x817E
#endif

#ifndef GL_SGIX_resample
#define GL_PACK_RESAMPLE_SGIX             0x842C
#define GL_UNPACK_RESAMPLE_SGIX           0x842D
#define GL_RESAMPLE_REPLICATE_SGIX        0x842E
#define GL_RESAMPLE_ZERO_FILL_SGIX        0x842F
#define GL_RESAMPLE_DECIMATE_SGIX         0x8430
#endif

#ifndef GL_SGIX_shadow
#define GL_TEXTURE_COMPARE_SGIX           0x819A
#define GL_TEXTURE_COMPARE_OPERATOR_SGIX  0x819B
#define GL_TEXTURE_LEQUAL_R_SGIX          0x819C
#define GL_TEXTURE_GEQUAL_R_SGIX          0x819D
#endif

#ifndef GL_SGIX_shadow_ambient
#define GL_SHADOW_AMBIENT_SGIX            0x80BF
#endif

#ifndef GL_SGIX_sprite
#define GL_SPRITE_SGIX                    0x8148
#define GL_SPRITE_MODE_SGIX               0x8149
#define GL_SPRITE_AXIS_SGIX               0x814A
#define GL_SPRITE_TRANSLATION_SGIX        0x814B
#define GL_SPRITE_AXIAL_SGIX              0x814C
#define GL_SPRITE_OBJECT_ALIGNED_SGIX     0x814D
#define GL_SPRITE_EYE_ALIGNED_SGIX        0x814E
#endif

#ifndef GL_SGIX_tag_sample_buffer
#endif

#ifndef GL_SGIX_texture_add_env
#define GL_TEXTURE_ENV_BIAS_SGIX          0x80BE
#endif

#ifndef GL_SGIX_texture_lod_bias
#define GL_TEXTURE_LOD_BIAS_S_SGIX        0x818E
#define GL_TEXTURE_LOD_BIAS_T_SGIX        0x818F
#define GL_TEXTURE_LOD_BIAS_R_SGIX        0x8190
#endif

#ifndef GL_SGIX_texture_multi_buffer
#define GL_TEXTURE_MULTI_BUFFER_HINT_SGIX 0x812E
#endif

#ifndef GL_SGIX_texture_scale_bias
#define GL_POST_TEXTURE_FILTER_BIAS_SGIX  0x8179
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX 0x817A
#define GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX 0x817B
#define GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX 0x817C
#endif

#ifndef GL_SGIX_vertex_preclip
#define GL_VERTEX_PRECLIP_SGIX            0x83EE
#define GL_VERTEX_PRECLIP_HINT_SGIX       0x83EF
#endif

#ifndef GL_SGIX_ycrcb
#define GL_YCRCB_422_SGIX                 0x81BB
#define GL_YCRCB_444_SGIX                 0x81BC
#endif

#ifndef GL_SUN_convolution_border_modes
#define GL_WRAP_BORDER_SUN                0x81D4
#endif

#ifndef GL_SUN_global_alpha
#define GL_GLOBAL_ALPHA_SUN               0x81D9
#define GL_GLOBAL_ALPHA_FACTOR_SUN        0x81DA
#endif

#ifndef GL_SUN_triangle_list
#define GL_RESTART_SUN                    0x01
#define GL_REPLACE_MIDDLE_SUN             0x02
#define GL_REPLACE_OLDEST_SUN             0x03
#define GL_TRIANGLE_LIST_SUN              0x81D7
#define GL_REPLACEMENT_CODE_SUN           0x81D8
#define GL_REPLACEMENT_CODE_ARRAY_SUN     0x85C0
#define GL_REPLACEMENT_CODE_ARRAY_TYPE_SUN 0x85C1
#define GL_REPLACEMENT_CODE_ARRAY_STRIDE_SUN 0x85C2
#define GL_REPLACEMENT_CODE_ARRAY_POINTER_SUN 0x85C3
#define GL_R1UI_V3F_SUN                   0x85C4
#define GL_R1UI_C4UB_V3F_SUN              0x85C5
#define GL_R1UI_C3F_V3F_SUN               0x85C6
#define GL_R1UI_N3F_V3F_SUN               0x85C7
#define GL_R1UI_C4F_N3F_V3F_SUN           0x85C8
#define GL_R1UI_T2F_V3F_SUN               0x85C9
#define GL_R1UI_T2F_N3F_V3F_SUN           0x85CA
#define GL_R1UI_T2F_C4F_N3F_V3F_SUN       0x85CB
#endif

#ifndef GL_SUN_vertex
#endif

#ifndef GL_SUNX_constant_data
#define GL_UNPACK_CONSTANT_DATA_SUNX      0x81D5
#define GL_TEXTURE_CONSTANT_DATA_SUNX     0x81D6
#endif

#ifndef GL_WIN_phong_shading
#define GL_PHONG_WIN                      0x80EA
#define GL_PHONG_HINT_WIN                 0x80EB
#endif

#ifndef GL_WIN_specular_fog
#define GL_FOG_SPECULAR_TEXTURE_WIN       0x80EC
#endif

	/*************************************************************/

#ifndef GL_VERSION_1_2
#define GL_VERSION_1_2 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf);
	extern void APIENTRY glBlendEquation(GLenum);
	extern void APIENTRY glDrawRangeElements(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid*);
	extern void APIENTRY glColorTable(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glColorTableParameterfv(GLenum, GLenum, const GLfloat*);
	extern void APIENTRY glColorTableParameteriv(GLenum, GLenum, const GLint*);
	extern void APIENTRY glCopyColorTable(GLenum, GLenum, GLint, GLint, GLsizei);
	extern void APIENTRY glGetColorTable(GLenum, GLenum, GLenum, GLvoid*);
	extern void APIENTRY glGetColorTableParameterfv(GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetColorTableParameteriv(GLenum, GLenum, GLint*);
	extern void APIENTRY glColorSubTable(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glCopyColorSubTable(GLenum, GLsizei, GLint, GLint, GLsizei);
	extern void APIENTRY glConvolutionFilter1D(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glConvolutionFilter2D(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glConvolutionParameterf(GLenum, GLenum, GLfloat);
	extern void APIENTRY glConvolutionParameterfv(GLenum, GLenum, const GLfloat*);
	extern void APIENTRY glConvolutionParameteri(GLenum, GLenum, GLint);
	extern void APIENTRY glConvolutionParameteriv(GLenum, GLenum, const GLint*);
	extern void APIENTRY glCopyConvolutionFilter1D(GLenum, GLenum, GLint, GLint, GLsizei);
	extern void APIENTRY glCopyConvolutionFilter2D(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
	extern void APIENTRY glGetConvolutionFilter(GLenum, GLenum, GLenum, GLvoid*);
	extern void APIENTRY glGetConvolutionParameterfv(GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetConvolutionParameteriv(GLenum, GLenum, GLint*);
	extern void APIENTRY glGetSeparableFilter(GLenum, GLenum, GLenum, GLvoid*, GLvoid*, GLvoid*);
	extern void APIENTRY glSeparableFilter2D(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*, const GLvoid*);
	extern void APIENTRY glGetHistogram(GLenum, GLboolean, GLenum, GLenum, GLvoid*);
	extern void APIENTRY glGetHistogramParameterfv(GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetHistogramParameteriv(GLenum, GLenum, GLint*);
	extern void APIENTRY glGetMinmax(GLenum, GLboolean, GLenum, GLenum, GLvoid*);
	extern void APIENTRY glGetMinmaxParameterfv(GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetMinmaxParameteriv(GLenum, GLenum, GLint*);
	extern void APIENTRY glHistogram(GLenum, GLsizei, GLenum, GLboolean);
	extern void APIENTRY glMinmax(GLenum, GLenum, GLboolean);
	extern void APIENTRY glResetHistogram(GLenum);
	extern void APIENTRY glResetMinmax(GLenum);
	extern void APIENTRY glTexImage3D(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glCopyTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
	extern void APIENTRY glActiveTexture(GLenum);
	extern void APIENTRY glClientActiveTexture(GLenum);
	extern void APIENTRY glMultiTexCoord1d(GLenum, GLdouble);
	extern void APIENTRY glMultiTexCoord1dv(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord1f(GLenum, GLfloat);
	extern void APIENTRY glMultiTexCoord1fv(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord1i(GLenum, GLint);
	extern void APIENTRY glMultiTexCoord1iv(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord1s(GLenum, GLshort);
	extern void APIENTRY glMultiTexCoord1sv(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord2d(GLenum, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord2dv(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord2f(GLenum, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord2fv(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord2i(GLenum, GLint, GLint);
	extern void APIENTRY glMultiTexCoord2iv(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord2s(GLenum, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord2sv(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord3d(GLenum, GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord3dv(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord3f(GLenum, GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord3fv(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord3i(GLenum, GLint, GLint, GLint);
	extern void APIENTRY glMultiTexCoord3iv(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord3s(GLenum, GLshort, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord3sv(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord4d(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord4dv(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord4f(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord4fv(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord4i(GLenum, GLint, GLint, GLint, GLint);
	extern void APIENTRY glMultiTexCoord4iv(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord4s(GLenum, GLshort, GLshort, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord4sv(GLenum, const GLshort*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLBLENDCOLORPROC)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	typedef void (APIENTRY* PFNGLBLENDEQUATIONPROC)(GLenum mode);
	typedef void (APIENTRY* PFNGLDRAWRANGEELEMENTSPROC)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
	typedef void (APIENTRY* PFNGLCOLORTABLEPROC)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* table);
	typedef void (APIENTRY* PFNGLCOLORTABLEPARAMETERFVPROC)(GLenum target, GLenum pname, const GLfloat* params);
	typedef void (APIENTRY* PFNGLCOLORTABLEPARAMETERIVPROC)(GLenum target, GLenum pname, const GLint* params);
	typedef void (APIENTRY* PFNGLCOPYCOLORTABLEPROC)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEPROC)(GLenum target, GLenum format, GLenum type, GLvoid* table);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEPARAMETERFVPROC)(GLenum target, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEPARAMETERIVPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLCOLORSUBTABLEPROC)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOPYCOLORSUBTABLEPROC)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
	typedef void (APIENTRY* PFNGLCONVOLUTIONFILTER1DPROC)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid* image);
	typedef void (APIENTRY* PFNGLCONVOLUTIONFILTER2DPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* image);
	typedef void (APIENTRY* PFNGLCONVOLUTIONPARAMETERFPROC)(GLenum target, GLenum pname, GLfloat params);
	typedef void (APIENTRY* PFNGLCONVOLUTIONPARAMETERFVPROC)(GLenum target, GLenum pname, const GLfloat* params);
	typedef void (APIENTRY* PFNGLCONVOLUTIONPARAMETERIPROC)(GLenum target, GLenum pname, GLint params);
	typedef void (APIENTRY* PFNGLCONVOLUTIONPARAMETERIVPROC)(GLenum target, GLenum pname, const GLint* params);
	typedef void (APIENTRY* PFNGLCOPYCONVOLUTIONFILTER1DPROC)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
	typedef void (APIENTRY* PFNGLCOPYCONVOLUTIONFILTER2DPROC)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (APIENTRY* PFNGLGETCONVOLUTIONFILTERPROC)(GLenum target, GLenum format, GLenum type, GLvoid* image);
	typedef void (APIENTRY* PFNGLGETCONVOLUTIONPARAMETERFVPROC)(GLenum target, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETCONVOLUTIONPARAMETERIVPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETSEPARABLEFILTERPROC)(GLenum target, GLenum format, GLenum type, GLvoid* row, GLvoid* column, GLvoid* span);
	typedef void (APIENTRY* PFNGLSEPARABLEFILTER2DPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* row, const GLvoid* column);
	typedef void (APIENTRY* PFNGLGETHISTOGRAMPROC)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values);
	typedef void (APIENTRY* PFNGLGETHISTOGRAMPARAMETERFVPROC)(GLenum target, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETHISTOGRAMPARAMETERIVPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETMINMAXPROC)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid* values);
	typedef void (APIENTRY* PFNGLGETMINMAXPARAMETERFVPROC)(GLenum target, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETMINMAXPARAMETERIVPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLHISTOGRAMPROC)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
	typedef void (APIENTRY* PFNGLMINMAXPROC)(GLenum target, GLenum internalformat, GLboolean sink);
	typedef void (APIENTRY* PFNGLRESETHISTOGRAMPROC)(GLenum target);
	typedef void (APIENTRY* PFNGLRESETMINMAXPROC)(GLenum target);
	typedef void (APIENTRY* PFNGLTEXIMAGE3DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void (APIENTRY* PFNGLTEXSUBIMAGE3DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void (APIENTRY* PFNGLCOPYTEXSUBIMAGE3DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	typedef void (APIENTRY* PFNGLACTIVETEXTUREPROC)(GLenum texture);
	typedef void (APIENTRY* PFNGLCLIENTACTIVETEXTUREPROC)(GLenum texture);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1DPROC)(GLenum target, GLdouble s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1DVPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1FPROC)(GLenum target, GLfloat s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1FVPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1IPROC)(GLenum target, GLint s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1IVPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1SPROC)(GLenum target, GLshort s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1SVPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2DPROC)(GLenum target, GLdouble s, GLdouble t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2DVPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2FPROC)(GLenum target, GLfloat s, GLfloat t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2FVPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2IPROC)(GLenum target, GLint s, GLint t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2IVPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2SPROC)(GLenum target, GLshort s, GLshort t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2SVPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3DPROC)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3DVPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3FPROC)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3FVPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3IPROC)(GLenum target, GLint s, GLint t, GLint r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3IVPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3SPROC)(GLenum target, GLshort s, GLshort t, GLshort r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3SVPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4DPROC)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4DVPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4FPROC)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4FVPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4IPROC)(GLenum target, GLint s, GLint t, GLint r, GLint q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4IVPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4SPROC)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4SVPROC)(GLenum target, const GLshort* v);
#endif

#ifndef GL_VERSION_1_3
#define GL_VERSION_1_3 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glCompressedTexImage3D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexImage1D(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage1D(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glGetCompressedTexImage(GLenum, GLint, void*);
	extern void APIENTRY glSampleCoverage(GLclampf, GLboolean);
	extern void APIENTRY glLoadTransposeMatrixf(const GLfloat*);
	extern void APIENTRY glLoadTransposeMatrixd(const GLdouble*);
	extern void APIENTRY glMultTransposeMatrixf(const GLfloat*);
	extern void APIENTRY glMultTransposeMatrixd(const GLdouble*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE3DPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE2DPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE1DPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLGETCOMPRESSEDTEXIMAGEPROC)(GLenum target, GLint level, void* img);
	typedef void (APIENTRY* PFNGLSAMPLECOVERAGEPROC)(GLclampf value, GLboolean invert);
	typedef void (APIENTRY* PFNGLLOADTRANSPOSEMATRIXFPROC)(const GLfloat* m);
	typedef void (APIENTRY* PFNGLLOADTRANSPOSEMATRIXDPROC)(const GLdouble* m);
	typedef void (APIENTRY* PFNGLMULTTRANSPOSEMATRIXFPROC)(const GLfloat* m);
	typedef void (APIENTRY* PFNGLMULTTRANSPOSEMATRIXDPROC)(const GLdouble* m);
#endif

#ifndef GL_VERSION_1_4
#define GL_VERSION_1_4 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glMultiDrawArrays(GLenum, GLint*, GLsizei*, GLsizei);
	extern void APIENTRY glMultiDrawElements(GLenum, const GLsizei*, GLenum, const GLvoid**, GLsizei);
	extern void APIENTRY glPointParameterf(GLenum pname, GLfloat param);
	extern void APIENTRY glPointParameterfv(GLenum pname, GLfloat* params);
	extern void APIENTRY glSecondaryColor3b(GLbyte, GLbyte, GLbyte);
	extern void APIENTRY glSecondaryColor3bv(const GLbyte*);
	extern void APIENTRY glSecondaryColor3d(GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glSecondaryColor3dv(const GLdouble*);
	extern void APIENTRY glSecondaryColor3f(GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glSecondaryColor3fv(const GLfloat*);
	extern void APIENTRY glSecondaryColor3i(GLint, GLint, GLint);
	extern void APIENTRY glSecondaryColor3iv(const GLint*);
	extern void APIENTRY glSecondaryColor3s(GLshort, GLshort, GLshort);
	extern void APIENTRY glSecondaryColor3sv(const GLshort*);
	extern void APIENTRY glSecondaryColor3ub(GLubyte, GLubyte, GLubyte);
	extern void APIENTRY glSecondaryColor3ubv(const GLubyte*);
	extern void APIENTRY glSecondaryColor3ui(GLuint, GLuint, GLuint);
	extern void APIENTRY glSecondaryColor3uiv(const GLuint*);
	extern void APIENTRY glSecondaryColor3us(GLushort, GLushort, GLushort);
	extern void APIENTRY glSecondaryColor3usv(const GLushort*);
	extern void APIENTRY glSecondaryColorPointer(GLint, GLenum, GLsizei, GLvoid*);
	extern void APIENTRY glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum);
	extern void APIENTRY glWindowPos2d(GLdouble x, GLdouble y);
	extern void APIENTRY glWindowPos2f(GLfloat x, GLfloat y);
	extern void APIENTRY glWindowPos2i(GLint x, GLint y);
	extern void APIENTRY glWindowPos2s(GLshort x, GLshort y);
	extern void APIENTRY glWindowPos2dv(const GLdouble* p);
	extern void APIENTRY glWindowPos2fv(const GLfloat* p);
	extern void APIENTRY glWindowPos2iv(const GLint* p);
	extern void APIENTRY glWindowPos2sv(const GLshort* p);
	extern void APIENTRY glWindowPos3d(GLdouble x, GLdouble y, GLdouble z);
	extern void APIENTRY glWindowPos3f(GLfloat x, GLfloat y, GLfloat z);
	extern void APIENTRY glWindowPos3i(GLint x, GLint y, GLint z);
	extern void APIENTRY glWindowPos3s(GLshort x, GLshort y, GLshort z);
	extern void APIENTRY glWindowPos3dv(const GLdouble* p);
	extern void APIENTRY glWindowPos3fv(const GLfloat* p);
	extern void APIENTRY glWindowPos3iv(const GLint* p);
	extern void APIENTRY glWindowPos3sv(const GLshort* p);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLMULTIDRAWARRAYSPROC)(GLenum mode, GLint* first, GLsizei* count, GLsizei primcount);
	typedef void (APIENTRY* PFNGLMULTIDRAWELEMENTSPROC)(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* *indices, GLsizei primcount);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFPROC)(GLenum pname, GLfloat param);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFVPROC)(GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3BPROC)(GLbyte red, GLbyte green, GLbyte blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3BVPROC)(const GLbyte* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3DPROC)(GLdouble red, GLdouble green, GLdouble blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3DVPROC)(const GLdouble* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3FPROC)(GLfloat red, GLfloat green, GLfloat blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3FVPROC)(const GLfloat* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3IPROC)(GLint red, GLint green, GLint blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3IVPROC)(const GLint* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3SPROC)(GLshort red, GLshort green, GLshort blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3SVPROC)(const GLshort* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UBPROC)(GLubyte red, GLubyte green, GLubyte blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UBVPROC)(const GLubyte* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UIPROC)(GLuint red, GLuint green, GLuint blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UIVPROC)(const GLuint* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3USPROC)(GLushort red, GLushort green, GLushort blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3USVPROC)(const GLushort* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLORPOINTERPROC)(GLint size, GLenum type, GLsizei stride, GLvoid* pointer);
	typedef void (APIENTRY* PFNGLBLENDFUNCSEPARATEPROC)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
	typedef void (APIENTRY* PFNGLWINDOWPOS2DPROC)(GLdouble x, GLdouble y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2FPROC)(GLfloat x, GLfloat y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2IPROC)(GLint x, GLint y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2SPROC)(GLshort x, GLshort y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2DVPROC)(const GLdouble* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2FVPROC)(const GLfloat* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2IVPROC)(const GLint* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2SVPROC)(const GLshort* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3DPROC)(GLdouble x, GLdouble y, GLdouble z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3FPROC)(GLfloat x, GLfloat y, GLfloat z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3IPROC)(GLint x, GLint y, GLint z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3SPROC)(GLshort x, GLshort y, GLshort z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3DVPROC)(const GLdouble* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3FVPROC)(const GLfloat* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3IVPROC)(const GLint* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3SVPROC)(const GLshort* p);
#endif


#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture 1
#endif

#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
#endif


#ifndef GL_ARB_multisample
#define GL_ARB_multisample 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glSampleCoverageARB(GLclampf, GLboolean);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLSAMPLECOVERAGEARBPROC)(GLclampf value, GLboolean invert);
#endif

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glActiveTextureARB(GLenum);
	extern void APIENTRY glClientActiveTextureARB(GLenum);
	extern void APIENTRY glMultiTexCoord1dARB(GLenum, GLdouble);
	extern void APIENTRY glMultiTexCoord1dvARB(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord1fARB(GLenum, GLfloat);
	extern void APIENTRY glMultiTexCoord1fvARB(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord1iARB(GLenum, GLint);
	extern void APIENTRY glMultiTexCoord1ivARB(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord1sARB(GLenum, GLshort);
	extern void APIENTRY glMultiTexCoord1svARB(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord2dARB(GLenum, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord2dvARB(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord2fARB(GLenum, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord2fvARB(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord2iARB(GLenum, GLint, GLint);
	extern void APIENTRY glMultiTexCoord2ivARB(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord2sARB(GLenum, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord2svARB(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord3dARB(GLenum, GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord3dvARB(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord3fARB(GLenum, GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord3fvARB(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord3iARB(GLenum, GLint, GLint, GLint);
	extern void APIENTRY glMultiTexCoord3ivARB(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord3sARB(GLenum, GLshort, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord3svARB(GLenum, const GLshort*);
	extern void APIENTRY glMultiTexCoord4dARB(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glMultiTexCoord4dvARB(GLenum, const GLdouble*);
	extern void APIENTRY glMultiTexCoord4fARB(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glMultiTexCoord4fvARB(GLenum, const GLfloat*);
	extern void APIENTRY glMultiTexCoord4iARB(GLenum, GLint, GLint, GLint, GLint);
	extern void APIENTRY glMultiTexCoord4ivARB(GLenum, const GLint*);
	extern void APIENTRY glMultiTexCoord4sARB(GLenum, GLshort, GLshort, GLshort, GLshort);
	extern void APIENTRY glMultiTexCoord4svARB(GLenum, const GLshort*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLACTIVETEXTUREARBPROC)(GLenum texture);
	typedef void (APIENTRY* PFNGLCLIENTACTIVETEXTUREARBPROC)(GLenum texture);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1DARBPROC)(GLenum target, GLdouble s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1DVARBPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1FARBPROC)(GLenum target, GLfloat s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1FVARBPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1IARBPROC)(GLenum target, GLint s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1IVARBPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1SARBPROC)(GLenum target, GLshort s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1SVARBPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2DARBPROC)(GLenum target, GLdouble s, GLdouble t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2DVARBPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2FARBPROC)(GLenum target, GLfloat s, GLfloat t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2FVARBPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2IARBPROC)(GLenum target, GLint s, GLint t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2IVARBPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2SARBPROC)(GLenum target, GLshort s, GLshort t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2SVARBPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3DARBPROC)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3DVARBPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3FARBPROC)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3FVARBPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3IARBPROC)(GLenum target, GLint s, GLint t, GLint r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3IVARBPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3SARBPROC)(GLenum target, GLshort s, GLshort t, GLshort r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3SVARBPROC)(GLenum target, const GLshort* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4DARBPROC)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4DVARBPROC)(GLenum target, const GLdouble* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4FARBPROC)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4FVARBPROC)(GLenum target, const GLfloat* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4IARBPROC)(GLenum target, GLint s, GLint t, GLint r, GLint q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4IVARBPROC)(GLenum target, const GLint* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4SARBPROC)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4SVARBPROC)(GLenum target, const GLshort* v);
#endif

#ifndef GL_ARB_point_parameters
#define GL_ARB_point_parameters 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glPointParameterfARB(GLenum pname, GLfloat param);
	extern void APIENTRY glPointParameterfvARB(GLenum pname, GLfloat* params);
#endif
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFARBPROC)(GLenum pname, GLfloat param);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFVARBPROC)(GLenum pname, GLfloat* params);
#endif

#ifndef GL_ARB_shadow
#define GL_ARB_shadow 1
#endif

#ifndef GL_ARB_texture_border_clamp
#define GL_ARB_texture_border_clamp 1
#endif

#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glCompressedTexImage3DARB(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexImage2DARB(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexImage1DARB(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage3DARB(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage2DARB(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glCompressedTexSubImage1DARB(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*);
	extern void APIENTRY glGetCompressedTexImageARB(GLenum, GLint, void*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data);
	typedef void (APIENTRY* PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)(GLenum target, GLint level, void* img);
#endif

#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1
#endif

#ifndef GL_ARB_texture_env_add
#define GL_ARB_texture_env_add 1
#endif

#ifndef GL_ARB_texture_env_combine
#define GL_ARB_texture_env_combine 1
#endif

#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3 1
#endif

#ifndef GL_ARB_texture_mirrored_repeat
#define GL_ARB_texture_mirrored_repeat 1
#endif

#ifndef GL_ARB_transpose_matrix
#define GL_ARB_transpose_matrix 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glLoadTransposeMatrixfARB(const GLfloat*);
	extern void APIENTRY glLoadTransposeMatrixdARB(const GLdouble*);
	extern void APIENTRY glMultTransposeMatrixfARB(const GLfloat*);
	extern void APIENTRY glMultTransposeMatrixdARB(const GLdouble*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLLOADTRANSPOSEMATRIXFARBPROC)(const GLfloat* m);
	typedef void (APIENTRY* PFNGLLOADTRANSPOSEMATRIXDARBPROC)(const GLdouble* m);
	typedef void (APIENTRY* PFNGLMULTTRANSPOSEMATRIXFARBPROC)(const GLfloat* m);
	typedef void (APIENTRY* PFNGLMULTTRANSPOSEMATRIXDARBPROC)(const GLdouble* m);
#endif

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1
#if defined(_WIN64)
	typedef __int64 GLintptrARB;
	typedef __int64 GLsizeiptrARB;
#elif defined(__ia64__) || defined(__x86_64__)
	typedef long int GLintptrARB;
	typedef long int GLsizeiptrARB;
#else
	typedef int GLintptrARB;
	typedef int GLsizeiptrARB;
#endif
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glBindBufferARB(GLenum target, GLuint buffer);
	extern void APIENTRY glDeleteBuffersARB(GLsizei n, const GLuint* buffers);
	extern void APIENTRY glGenBuffersARB(GLsizei n, GLuint* buffers);
	extern GLboolean APIENTRY glIsBufferARB(GLuint buffer);
	extern void APIENTRY glBufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage);
	extern void APIENTRY glBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid* data);
	extern void APIENTRY glGetBufferSubDataARB(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid* data);
	extern void* APIENTRY glMapBufferARB(GLenum target, GLenum access);
	extern GLboolean APIENTRY glUnmapBufferARB(GLenum target);
	extern void APIENTRY glGetBufferParameterivARB(GLenum target, GLenum pname, GLint* params);
	extern void APIENTRY glGetBufferPointervARB(GLenum target, GLenum pname, GLvoid** params);
#endif
	typedef void (APIENTRY* PFNGLBINDBUFFERARBPROC)(GLenum target, GLuint buffer);
	typedef void (APIENTRY* PFNGLDELETEBUFFERSARBPROC)(GLsizei n, const GLuint* buffers);
	typedef void (APIENTRY* PFNGLGENBUFFERSARBPROC)(GLsizei n, GLuint* buffers);
	typedef GLboolean(APIENTRY* PFNGLISBUFFERARBPROC)(GLuint buffer);
	typedef void (APIENTRY* PFNGLBUFFERDATAARBPROC)(GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage);
	typedef void (APIENTRY* PFNGLBUFFERSUBDATAARBPROC)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid* data);
	typedef void (APIENTRY* PFNGLGETBUFFERSUBDATAARBPROC)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid* data);
	typedef void* (APIENTRY* PFNGLMAPBUFFERARBPROC)(GLenum target, GLenum access);
	typedef GLboolean(APIENTRY* PFNGLUNMAPBUFFERARBPROC)(GLenum target);
	typedef void (APIENTRY* PFNGLGETBUFFERPARAMETERIVARBPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETBUFFERPOINTERVARBPROC)(GLenum target, GLenum pname, GLvoid** params);
#endif

#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glVertexAttrib1sARB(GLuint index, GLshort x);
	extern void APIENTRY glVertexAttrib1fARB(GLuint index, GLfloat x);
	extern void APIENTRY glVertexAttrib1dARB(GLuint index, GLdouble x);
	extern void APIENTRY glVertexAttrib2sARB(GLuint index, GLshort x, GLshort y);
	extern void APIENTRY glVertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y);
	extern void APIENTRY glVertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y);
	extern void APIENTRY glVertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z);
	extern void APIENTRY glVertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	extern void APIENTRY glVertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	extern void APIENTRY glVertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
	extern void APIENTRY glVertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void APIENTRY glVertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void APIENTRY glVertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
	extern void APIENTRY glVertexAttrib1svARB(GLuint index, const GLshort* v);
	extern void APIENTRY glVertexAttrib1fvARB(GLuint index, const GLfloat* v);
	extern void APIENTRY glVertexAttrib1dvARB(GLuint index, const GLdouble* v);
	extern void APIENTRY glVertexAttrib2svARB(GLuint index, const GLshort* v);
	extern void APIENTRY glVertexAttrib2fvARB(GLuint index, const GLfloat* v);
	extern void APIENTRY glVertexAttrib2dvARB(GLuint index, const GLdouble* v);
	extern void APIENTRY glVertexAttrib3svARB(GLuint index, const GLshort* v);
	extern void APIENTRY glVertexAttrib3fvARB(GLuint index, const GLfloat* v);
	extern void APIENTRY glVertexAttrib3dvARB(GLuint index, const GLdouble* v);
	extern void APIENTRY glVertexAttrib4bvARB(GLuint index, const GLbyte* v);
	extern void APIENTRY glVertexAttrib4svARB(GLuint index, const GLshort* v);
	extern void APIENTRY glVertexAttrib4ivARB(GLuint index, const GLint* v);
	extern void APIENTRY glVertexAttrib4ubvARB(GLuint index, const GLubyte* v);
	extern void APIENTRY glVertexAttrib4usvARB(GLuint index, const GLushort* v);
	extern void APIENTRY glVertexAttrib4uivARB(GLuint index, const GLuint* v);
	extern void APIENTRY glVertexAttrib4fvARB(GLuint index, const GLfloat* v);
	extern void APIENTRY glVertexAttrib4dvARB(GLuint index, const GLdouble* v);
	extern void APIENTRY glVertexAttrib4NbvARB(GLuint index, const GLbyte* v);
	extern void APIENTRY glVertexAttrib4NsvARB(GLuint index, const GLshort* v);
	extern void APIENTRY glVertexAttrib4NivARB(GLuint index, const GLint* v);
	extern void APIENTRY glVertexAttrib4NubvARB(GLuint index, const GLubyte* v);
	extern void APIENTRY glVertexAttrib4NusvARB(GLuint index, const GLushort* v);
	extern void APIENTRY glVertexAttrib4NuivARB(GLuint index, const GLuint* v);
	extern void APIENTRY glVertexAttribPointerARB(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
	extern void APIENTRY glEnableVertexAttribArrayARB(GLuint index);
	extern void APIENTRY glDisableVertexAttribArrayARB(GLuint index);
	extern void APIENTRY glProgramStringARB(GLenum target, GLenum format, GLsizei len, const void* string);
	extern void APIENTRY glBindProgramARB(GLenum target, GLuint program);
	extern void APIENTRY glDeleteProgramsARB(GLsizei n, const GLuint* programs);
	extern void APIENTRY glGenProgramsARB(GLsizei n, GLuint* programs);
	extern void APIENTRY glProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void APIENTRY glProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble* params);
	extern void APIENTRY glProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void APIENTRY glProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat* params);
	extern void APIENTRY glProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void APIENTRY glProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble* params);
	extern void APIENTRY glProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void APIENTRY glProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat* params);
	extern void APIENTRY glGetProgramEnvParameterdvARB(GLenum target, GLuint index, GLdouble* params);
	extern void APIENTRY glGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat* params);
	extern void APIENTRY glGetProgramLocalParameterdvARB(GLenum target, GLuint index, GLdouble* params);
	extern void APIENTRY glGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat* params);
	extern void APIENTRY glGetProgramivARB(GLenum target, GLenum pname, GLint* params);
	extern void APIENTRY glGetProgramStringARB(GLenum target, GLenum pname, void* string);
	extern void APIENTRY glGetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble* params);
	extern void APIENTRY glGetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat* params);
	extern void APIENTRY glGetVertexAttribivARB(GLuint index, GLenum pname, GLint* params);
	extern void APIENTRY glGetVertexAttribPointervARB(GLuint index, GLenum pname, void** pointer);
	extern GLboolean APIENTRY glIsProgramARB(GLuint program);
#endif
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1SARBPROC)(GLuint index, GLshort x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1FARBPROC)(GLuint index, GLfloat x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1DARBPROC)(GLuint index, GLdouble x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2SARBPROC)(GLuint index, GLshort x, GLshort y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2FARBPROC)(GLuint index, GLfloat x, GLfloat y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2DARBPROC)(GLuint index, GLdouble x, GLdouble y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3SARBPROC)(GLuint index, GLshort x, GLshort y, GLshort z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3FARBPROC)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3DARBPROC)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4SARBPROC)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4FARBPROC)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4DARBPROC)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NUBARBPROC)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1SVARBPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1FVARBPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1DVARBPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2SVARBPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2FVARBPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2DVARBPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3SVARBPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3FVARBPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3DVARBPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4BVARBPROC)(GLuint index, const GLbyte* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4SVARBPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4IVARBPROC)(GLuint index, const GLint* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4UBVARBPROC)(GLuint index, const GLubyte* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4USVARBPROC)(GLuint index, const GLushort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4UIVARBPROC)(GLuint index, const GLuint* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4FVARBPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4DVARBPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NBVARBPROC)(GLuint index, const GLbyte* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NSVARBPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NIVARBPROC)(GLuint index, const GLint* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NUBVARBPROC)(GLuint index, const GLubyte* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NUSVARBPROC)(GLuint index, const GLushort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4NUIVARBPROC)(GLuint index, const GLuint* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERARBPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
	typedef void (APIENTRY* PFNGLENABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
	typedef void (APIENTRY* PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)(GLuint index);
	typedef void (APIENTRY* PFNGLPROGRAMSTRINGARBPROC)(GLenum target, GLenum format, GLsizei len, const void* string);
	typedef void (APIENTRY* PFNGLBINDPROGRAMARBPROC)(GLenum target, GLuint program);
	typedef void (APIENTRY* PFNGLDELETEPROGRAMSARBPROC)(GLsizei n, const GLuint* programs);
	typedef void (APIENTRY* PFNGLGENPROGRAMSARBPROC)(GLsizei n, GLuint* programs);
	typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4DARBPROC)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4DVARBPROC)(GLenum target, GLuint index, const GLdouble* params);
	typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4FARBPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLPROGRAMENVPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
	typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4DARBPROC)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4DVARBPROC)(GLenum target, GLuint index, const GLdouble* params);
	typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4FARBPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLPROGRAMLOCALPARAMETER4FVARBPROC)(GLenum target, GLuint index, const GLfloat* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMENVPARAMETERDVARBPROC)(GLenum target, GLuint index, GLdouble* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMENVPARAMETERFVARBPROC)(GLenum target, GLuint index, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC)(GLenum target, GLuint index, GLdouble* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC)(GLenum target, GLuint index, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMIVARBPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMSTRINGARBPROC)(GLenum target, GLenum pname, void* string);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBDVARBPROC)(GLuint index, GLenum pname, GLdouble* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBFVARBPROC)(GLuint index, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBIVARBPROC)(GLuint index, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBPOINTERVARBPROC)(GLuint index, GLenum pname, void** pointer);
	typedef GLboolean(APIENTRY* PFNGLISPROGRAMARBPROC)(GLuint program);
#endif

#ifndef GL_ARB_window_pos
#define GL_ARB_window_pos 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glWindowPos2dARB(GLdouble x, GLdouble y);
	extern void APIENTRY glWindowPos2fARB(GLfloat x, GLfloat y);
	extern void APIENTRY glWindowPos2iARB(GLint x, GLint y);
	extern void APIENTRY glWindowPos2sARB(GLshort x, GLshort y);
	extern void APIENTRY glWindowPos2dvARB(const GLdouble* p);
	extern void APIENTRY glWindowPos2fvARB(const GLfloat* p);
	extern void APIENTRY glWindowPos2ivARB(const GLint* p);
	extern void APIENTRY glWindowPos2svARB(const GLshort* p);
	extern void APIENTRY glWindowPos3dARB(GLdouble x, GLdouble y, GLdouble z);
	extern void APIENTRY glWindowPos3fARB(GLfloat x, GLfloat y, GLfloat z);
	extern void APIENTRY glWindowPos3iARB(GLint x, GLint y, GLint z);
	extern void APIENTRY glWindowPos3sARB(GLshort x, GLshort y, GLshort z);
	extern void APIENTRY glWindowPos3dvARB(const GLdouble* p);
	extern void APIENTRY glWindowPos3fvARB(const GLfloat* p);
	extern void APIENTRY glWindowPos3ivARB(const GLint* p);
	extern void APIENTRY glWindowPos3svARB(const GLshort* p);
#endif
	typedef void (APIENTRY* PFNGLWINDOWPOS2DARBPROC)(GLdouble x, GLdouble y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2FARBPROC)(GLfloat x, GLfloat y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2IARBPROC)(GLint x, GLint y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2SARBPROC)(GLshort x, GLshort y);
	typedef void (APIENTRY* PFNGLWINDOWPOS2DVARBPROC)(const GLdouble* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2FVARBPROC)(const GLfloat* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2IVARBPROC)(const GLint* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS2SVARBPROC)(const GLshort* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3DARBPROC)(GLdouble x, GLdouble y, GLdouble z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3FARBPROC)(GLfloat x, GLfloat y, GLfloat z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3IARBPROC)(GLint x, GLint y, GLint z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3SARBPROC)(GLshort x, GLshort y, GLshort z);
	typedef void (APIENTRY* PFNGLWINDOWPOS3DVARBPROC)(const GLdouble* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3FVARBPROC)(const GLfloat* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3IVARBPROC)(const GLint* p);
	typedef void (APIENTRY* PFNGLWINDOWPOS3SVARBPROC)(const GLshort* p);
#endif

#ifndef GL_EXT_abgr
#define GL_EXT_abgr 1
#endif

#ifndef GL_EXT_bgra
#define GL_EXT_bgra 1
#endif

#ifndef GL_EXT_blend_color
#define GL_EXT_blend_color 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glBlendColorEXT(GLclampf, GLclampf, GLclampf, GLclampf);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLBLENDCOLOREXTPROC)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
#endif

#ifndef GL_EXT_blend_func_separate
#define GL_EXT_blend_func_separate 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glBlendFuncSeparateEXT(GLenum, GLenum, GLenum, GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLBLENDFUNCSEPARATEEXTPROC)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
#endif

#ifndef GL_EXT_blend_minmax
#define GL_EXT_blend_minmax 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glBlendEquationEXT(GLenum);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLBLENDEQUATIONEXTPROC)(GLenum mode);
#endif

#ifndef GL_EXT_blend_subtract
#define GL_EXT_blend_subtract 1
#endif

#ifndef GL_EXT_compiled_vertex_array
#define GL_EXT_compiled_vertex_array 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glLockArraysEXT(GLint, GLsizei);
	extern void APIENTRY glUnlockArraysEXT(void);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLLOCKARRAYSEXTPROC)(GLint first, GLsizei count);
	typedef void (APIENTRY* PFNGLUNLOCKARRAYSEXTPROC)(void);
#endif

#ifndef GL_EXT_draw_range_elements
#define GL_EXT_draw_range_elements 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glDrawRangeElementsEXT(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLDRAWRANGEELEMENTSEXTPROC)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
#endif

#ifndef GL_EXT_fog_coord
#define GL_EXT_fog_coord 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glFogCoordfEXT(GLfloat);
	extern void APIENTRY glFogCoordfvEXT(const GLfloat*);
	extern void APIENTRY glFogCoorddEXT(GLdouble);
	extern void APIENTRY glFogCoorddvEXT(const GLdouble*);
	extern void APIENTRY glFogCoordPointerEXT(GLenum, GLsizei, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLFOGCOORDFEXTPROC)(GLfloat coord);
	typedef void (APIENTRY* PFNGLFOGCOORDFVEXTPROC)(const GLfloat* coord);
	typedef void (APIENTRY* PFNGLFOGCOORDDEXTPROC)(GLdouble coord);
	typedef void (APIENTRY* PFNGLFOGCOORDDVEXTPROC)(const GLdouble* coord);
	typedef void (APIENTRY* PFNGLFOGCOORDPOINTEREXTPROC)(GLenum type, GLsizei stride, const GLvoid* pointer);
#endif

#ifndef GL_EXT_multi_draw_arrays
#define GL_EXT_multi_draw_arrays 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glMultiDrawArraysEXT(GLenum, GLint*, GLsizei*, GLsizei);
	extern void APIENTRY glMultiDrawElementsEXT(GLenum, const GLsizei*, GLenum, const GLvoid**, GLsizei);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLMULTIDRAWARRAYSEXTPROC)(GLenum mode, GLint* first, GLsizei* count, GLsizei primcount);
	typedef void (APIENTRY* PFNGLMULTIDRAWELEMENTSEXTPROC)(GLenum mode, const GLsizei* count, GLenum type, const GLvoid* *indices, GLsizei primcount);
#endif

#ifndef GL_EXT_packed_pixels
#define GL_EXT_packed_pixels 1
#endif

#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glColorTableEXT(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid* data);
	extern void APIENTRY glGetColorTableEXT(GLenum, GLenum, GLenum, GLvoid*);
	extern void APIENTRY glGetColorTableParameterivEXT(GLenum, GLenum, GLint*);
	extern void APIENTRY glGetColorTableParameterfvEXT(GLenum, GLenum, GLfloat*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLCOLORTABLEEXTPROC)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid* table);
	typedef void (APIENTRY* PFNGLCOLORSUBTABLEEXTPROC)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid* data);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEEXTPROC)(GLenum target, GLenum format, GLenum type, GLvoid* data);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)(GLenum target, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETCOLORTABLEPARAMETERFVEXTPROC)(GLenum target, GLenum pname, GLfloat* params);
#endif

#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glPointParameterfEXT(GLenum, GLfloat);
	extern void APIENTRY glPointParameterfvEXT(GLenum, const GLfloat*);
	extern void APIENTRY glPointParameterfSGIS(GLenum, GLfloat);
	extern void APIENTRY glPointParameterfvSGIS(GLenum, const GLfloat*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFEXTPROC)(GLenum pname, GLfloat param);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFVEXTPROC)(GLenum pname, const GLfloat* params);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFSGISPROC)(GLenum pname, GLfloat param);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERFVSGISPROC)(GLenum pname, const GLfloat* params);
#endif

#ifndef GL_EXT_rescale_normal
#define GL_EXT_rescale_normal 1
#endif

#ifndef GL_EXT_secondary_color
#define GL_EXT_secondary_color 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glSecondaryColor3bEXT(GLbyte, GLbyte, GLbyte);
	extern void APIENTRY glSecondaryColor3bvEXT(const GLbyte*);
	extern void APIENTRY glSecondaryColor3dEXT(GLdouble, GLdouble, GLdouble);
	extern void APIENTRY glSecondaryColor3dvEXT(const GLdouble*);
	extern void APIENTRY glSecondaryColor3fEXT(GLfloat, GLfloat, GLfloat);
	extern void APIENTRY glSecondaryColor3fvEXT(const GLfloat*);
	extern void APIENTRY glSecondaryColor3iEXT(GLint, GLint, GLint);
	extern void APIENTRY glSecondaryColor3ivEXT(const GLint*);
	extern void APIENTRY glSecondaryColor3sEXT(GLshort, GLshort, GLshort);
	extern void APIENTRY glSecondaryColor3svEXT(const GLshort*);
	extern void APIENTRY glSecondaryColor3ubEXT(GLubyte, GLubyte, GLubyte);
	extern void APIENTRY glSecondaryColor3ubvEXT(const GLubyte*);
	extern void APIENTRY glSecondaryColor3uiEXT(GLuint, GLuint, GLuint);
	extern void APIENTRY glSecondaryColor3uivEXT(const GLuint*);
	extern void APIENTRY glSecondaryColor3usEXT(GLushort, GLushort, GLushort);
	extern void APIENTRY glSecondaryColor3usvEXT(const GLushort*);
	extern void APIENTRY glSecondaryColorPointerEXT(GLint, GLenum, GLsizei, GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3BEXTPROC)(GLbyte red, GLbyte green, GLbyte blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3BVEXTPROC)(const GLbyte* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3DEXTPROC)(GLdouble red, GLdouble green, GLdouble blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3DVEXTPROC)(const GLdouble* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3FEXTPROC)(GLfloat red, GLfloat green, GLfloat blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3FVEXTPROC)(const GLfloat* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3IEXTPROC)(GLint red, GLint green, GLint blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3IVEXTPROC)(const GLint* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3SEXTPROC)(GLshort red, GLshort green, GLshort blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3SVEXTPROC)(const GLshort* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UBEXTPROC)(GLubyte red, GLubyte green, GLubyte blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UBVEXTPROC)(const GLubyte* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UIEXTPROC)(GLuint red, GLuint green, GLuint blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3UIVEXTPROC)(const GLuint* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3USEXTPROC)(GLushort red, GLushort green, GLushort blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3USVEXTPROC)(const GLushort* v);
	typedef void (APIENTRY* PFNGLSECONDARYCOLORPOINTEREXTPROC)(GLint size, GLenum type, GLsizei stride, GLvoid* pointer);
#endif

#ifndef GL_EXT_separate_specular_color
#define GL_EXT_separate_specular_color 1
#endif

#ifndef GL_EXT_shadow_funcs
#define GL_EXT_shadow_funcs 1
#endif

#ifndef GL_EXT_shared_texture_palette
#define GL_EXT_shared_texture_palette 1
#endif

#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void glActiveStencilFaceEXT(GLenum face);
#endif
	typedef void (APIENTRY* PFNGLACTIVESTENCILFACEEXTPROC)(GLenum face);
#endif

#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1
#endif

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
#endif

#ifndef GL_EXT_texture_env_add
#define GL_EXT_texture_env_add 1
#endif

#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine 1
#endif

#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#endif

#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias 1
#endif

#ifndef GL_EXT_texture_object
#define GL_EXT_texture_object 1
#ifdef GL_GLEXT_PROTOTYPES
	extern GLboolean APIENTRY glAreTexturesResidentEXT(GLsizei, const GLuint*, GLboolean*);
	extern void APIENTRY glBindTextureEXT(GLenum, GLuint);
	extern void APIENTRY glDeleteTexturesEXT(GLsizei, const GLuint*);
	extern void APIENTRY glGenTexturesEXT(GLsizei, GLuint*);
	extern GLboolean APIENTRY glIsTextureEXT(GLuint);
	extern void APIENTRY glPrioritizeTexturesEXT(GLsizei, const GLuint*, const GLclampf*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef GLboolean(APIENTRY* PFNGLARETEXTURESRESIDENTEXTPROC)(GLsizei n, const GLuint* textures, GLboolean* residences);
	typedef void (APIENTRY* PFNGLBINDTEXTUREEXTPROC)(GLenum target, GLuint texture);
	typedef void (APIENTRY* PFNGLDELETETEXTURESEXTPROC)(GLsizei n, const GLuint* textures);
	typedef void (APIENTRY* PFNGLGENTEXTURESEXTPROC)(GLsizei n, GLuint* textures);
	typedef GLboolean(APIENTRY* PFNGLISTEXTUREEXTPROC)(GLuint texture);
	typedef void (APIENTRY* PFNGLPRIORITIZETEXTURESEXTPROC)(GLsizei n, const GLuint* textures, const GLclampf* priorities);
#endif

#ifndef GL_EXT_texture3D
#define GL_EXT_texture3D 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glTexImage3DEXT(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
	extern void APIENTRY glTexSubImage3DEXT(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLTEXIMAGE3DEXTPROC)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
	typedef void (APIENTRY* PFNGLTEXSUBIMAGE3DEXTPROC)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
#endif

#ifndef GL_EXT_vertex_array
#define GL_EXT_vertex_array 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glArrayElementEXT(GLint);
	extern void APIENTRY glColorPointerEXT(GLint, GLenum, GLsizei, GLsizei, const GLvoid*);
	extern void APIENTRY glDrawArraysEXT(GLenum, GLint, GLsizei);
	extern void APIENTRY glEdgeFlagPointerEXT(GLsizei, GLsizei, const GLboolean*);
	extern void APIENTRY glGetPointervEXT(GLenum, GLvoid**);
	extern void APIENTRY glIndexPointerEXT(GLenum, GLsizei, GLsizei, const GLvoid*);
	extern void APIENTRY glNormalPointerEXT(GLenum, GLsizei, GLsizei, const GLvoid*);
	extern void APIENTRY glTexCoordPointerEXT(GLint, GLenum, GLsizei, GLsizei, const GLvoid*);
	extern void APIENTRY glVertexPointerEXT(GLint, GLenum, GLsizei, GLsizei, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLARRAYELEMENTEXTPROC)(GLint i);
	typedef void (APIENTRY* PFNGLCOLORPOINTEREXTPROC)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLDRAWARRAYSEXTPROC)(GLenum mode, GLint first, GLsizei count);
	typedef void (APIENTRY* PFNGLEDGEFLAGPOINTEREXTPROC)(GLsizei stride, GLsizei count, const GLboolean* pointer);
	typedef void (APIENTRY* PFNGLGETPOINTERVEXTPROC)(GLenum pname, GLvoid* *params);
	typedef void (APIENTRY* PFNGLINDEXPOINTEREXTPROC)(GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLNORMALPOINTEREXTPROC)(GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLTEXCOORDPOINTEREXTPROC)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLVERTEXPOINTEREXTPROC)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid* pointer);
#endif

#ifndef GL_EXT_vertex_weighting
#define GL_EXT_vertex_weighting 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glVertexWeightfEXT(GLfloat);
	extern void APIENTRY glVertexWeightfvEXT(const GLfloat*);
	extern void APIENTRY glVertexWeightPointerEXT(GLsizei, GLenum, GLsizei, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLVERTEXWEIGHTFEXTPROC)(GLfloat weight);
	typedef void (APIENTRY* PFNGLVERTEXWEIGHTFVEXTPROC)(const GLfloat* weight);
	typedef void (APIENTRY* PFNGLVERTEXWEIGHTPOINTEREXTPROC)(GLsizei size, GLenum type, GLsizei stride, const GLvoid* pointer);
#endif

#ifndef GL_HP_occlusion_test
#define GL_HP_occlusion_test 1
#endif

#ifndef GL_IBM_texture_mirrored_repeat
#define GL_IBM_texture_mirrored_repeat 1
#endif

#ifndef GL_NV_blend_square
#define GL_NV_blend_square 1
#endif

#ifndef GL_NV_copy_depth_to_color
#define GL_NV_copy_depth_to_color 1
#endif

#ifndef GL_NV_depth_clamp
#define GL_NV_depth_clamp 1
#endif

#ifndef GL_NV_element_array
#define GL_NV_element_array 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glElementPointerNV(GLenum type, const GLvoid* pointer);
	extern void APIENTRY glDrawElementArrayNV(GLenum mode, GLint first, GLsizei count);
	extern void APIENTRY glDrawRangeElementArrayNV(GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
	extern void APIENTRY glMultiDrawElementArrayNV(GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount);
	extern void APIENTRY glMultiDrawRangeElementArrayNV(GLenum mode, GLuint start, GLuint end, const GLint* first, const GLsizei* count, GLsizei primcount);
#endif
	typedef void (APIENTRY* PFNGLELEMENTPOINTERNVPROC)(GLenum type, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLDRAWELEMENTARRAYNVPROC)(GLenum mode, GLint first, GLsizei count);
	typedef void (APIENTRY* PFNGLDRAWRANGEELEMENTARRAYNVPROC)(GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
	typedef void (APIENTRY* PFNGLMULTIDRAWELEMENTARRAYNVPROC)(GLenum mode, const GLint* first, const GLsizei* count, GLsizei primcount);
	typedef void (APIENTRY* PFNGLMULTIDRAWRANGEELEMENTARRAYNVPROC)(GLenum mode, GLuint start, GLuint end, const GLint* first, const GLsizei* count, GLsizei primcount);
#endif

#ifndef GL_NV_fence
#define GL_NV_fence 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glDeleteFencesNV(GLsizei n, const GLuint* fences);
	extern void APIENTRY glGenFencesNV(GLsizei n, GLuint* fences);
	extern GLboolean APIENTRY glIsFenceNV(GLuint fence);
	extern GLboolean APIENTRY glTestFenceNV(GLuint fence);
	extern void APIENTRY glGetFenceivNV(GLuint fence, GLenum pname, GLint* params);
	extern void APIENTRY glFinishFenceNV(GLuint fence);
	extern void APIENTRY glSetFenceNV(GLuint fence, GLenum condition);
#endif
	typedef void (APIENTRY* PFNGLDELETEFENCESNVPROC)(GLsizei n, const GLuint* fences);
	typedef void (APIENTRY* PFNGLGENFENCESNVPROC)(GLsizei n, GLuint* fences);
	typedef GLboolean(APIENTRY* PFNGLISFENCENVPROC)(GLuint fence);
	typedef GLboolean(APIENTRY* PFNGLTESTFENCENVPROC)(GLuint fence);
	typedef void (APIENTRY* PFNGLGETFENCEIVNVPROC)(GLuint fence, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLFINISHFENCENVPROC)(GLuint fence);
	typedef void (APIENTRY* PFNGLSETFENCENVPROC)(GLuint fence, GLenum condition);
#endif

#ifndef GL_NV_float_buffer
#define GL_NV_float_buffer 1
#endif

#ifndef GL_NV_fog_distance
#define GL_NV_fog_distance 1
#endif

#ifndef GL_NV_fragment_program
#define GL_NV_fragment_program 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void APIENTRY glProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void APIENTRY glProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte* name, const GLfloat v[]);
	extern void APIENTRY glProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte* name, const GLdouble v[]);
	extern void APIENTRY glGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte* name, GLfloat* params);
	extern void APIENTRY glGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte* name, GLdouble* params);
#endif
	typedef void (APIENTRY* PFNGLPROGRAMNAMEDPARAMETER4FNVPROC)(GLuint id, GLsizei len, const GLubyte* name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLPROGRAMNAMEDPARAMETER4DNVPROC)(GLuint id, GLsizei len, const GLubyte* name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLPROGRAMNAMEDPARAMETER4FVNVPROC)(GLuint id, GLsizei len, const GLubyte* name, const GLfloat v[]);
	typedef void (APIENTRY* PFNGLPROGRAMNAMEDPARAMETER4DVNVPROC)(GLuint id, GLsizei len, const GLubyte* name, const GLdouble v[]);
	typedef void (APIENTRY* PFNGLGETPROGRAMNAMEDPARAMETERFVNVPROC)(GLuint id, GLsizei len, const GLubyte* name, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMNAMEDPARAMETERDVNVPROC)(GLuint id, GLsizei len, const GLubyte* name, GLdouble* params);
#endif

#ifndef GL_NV_half_float
#define GL_NV_half_float 1
#ifndef GLhalf
	typedef unsigned short GLhalf;
#endif
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glVertex2hNV(GLhalf x, GLhalf y);
	extern void APIENTRY glVertex2hvNV(const GLhalf* v);
	extern void APIENTRY glVertex3hNV(GLhalf x, GLhalf y, GLhalf z);
	extern void APIENTRY glVertex3hvNV(const GLhalf* v);
	extern void APIENTRY glVertex4hNV(GLhalf x, GLhalf y, GLhalf z, GLhalf w);
	extern void APIENTRY glVertex4hvNV(const GLhalf* v);
	extern void APIENTRY glNormal3hNV(GLhalf nx, GLhalf ny, GLhalf nz);
	extern void APIENTRY glNormal3hvNV(const GLhalf* v);
	extern void APIENTRY glColor3hNV(GLhalf red, GLhalf green, GLhalf blue);
	extern void APIENTRY glColor3hvNV(const GLhalf* v);
	extern void APIENTRY glColor4hNV(GLhalf red, GLhalf green, GLhalf blue, GLhalf alpha);
	extern void APIENTRY glColor4hvNV(const GLhalf* v);
	extern void APIENTRY glTexCoord1hNV(GLhalf s);
	extern void APIENTRY glTexCoord1hvNV(const GLhalf* v);
	extern void APIENTRY glTexCoord2hNV(GLhalf s, GLhalf t);
	extern void APIENTRY glTexCoord2hvNV(const GLhalf* v);
	extern void APIENTRY glTexCoord3hNV(GLhalf s, GLhalf t, GLhalf r);
	extern void APIENTRY glTexCoord3hvNV(const GLhalf* v);
	extern void APIENTRY glTexCoord4hNV(GLhalf s, GLhalf t, GLhalf r, GLhalf q);
	extern void APIENTRY glTexCoord4hvNV(const GLhalf* v);
	extern void APIENTRY glMultiTexCoord1hNV(GLenum target, GLhalf s);
	extern void APIENTRY glMultiTexCoord1hvNV(GLenum target, const GLhalf* v);
	extern void APIENTRY glMultiTexCoord2hNV(GLenum target, GLhalf s, GLhalf t);
	extern void APIENTRY glMultiTexCoord2hvNV(GLenum target, const GLhalf* v);
	extern void APIENTRY glMultiTexCoord3hNV(GLenum target, GLhalf s, GLhalf t, GLhalf r);
	extern void APIENTRY glMultiTexCoord3hvNV(GLenum target, const GLhalf* v);
	extern void APIENTRY glMultiTexCoord4hNV(GLenum target, GLhalf s, GLhalf t, GLhalf r, GLhalf q);
	extern void APIENTRY glMultiTexCoord4hvNV(GLenum target, const GLhalf* v);
	extern void APIENTRY glFogCoordhNV(GLhalf fog);
	extern void APIENTRY glFogCoordhvNV(const GLhalf* fog);
	extern void APIENTRY glSecondaryColor3hNV(GLhalf red, GLhalf green, GLhalf blue);
	extern void APIENTRY glSecondaryColor3hvNV(const GLhalf* v);
	extern void APIENTRY glVertexWeighthNV(GLhalf weight);
	extern void APIENTRY glVertexWeighthvNV(const GLhalf* weight);
	extern void APIENTRY glVertexAttrib1hNV(GLuint index, GLhalf x);
	extern void APIENTRY glVertexAttrib1hvNV(GLuint index, const GLhalf* v);
	extern void APIENTRY glVertexAttrib2hNV(GLuint index, GLhalf x, GLhalf y);
	extern void APIENTRY glVertexAttrib2hvNV(GLuint index, const GLhalf* v);
	extern void APIENTRY glVertexAttrib3hNV(GLuint index, GLhalf x, GLhalf y, GLhalf z);
	extern void APIENTRY glVertexAttrib3hvNV(GLuint index, const GLhalf* v);
	extern void APIENTRY glVertexAttrib4hNV(GLuint index, GLhalf x, GLhalf y, GLhalf z, GLhalf w);
	extern void APIENTRY glVertexAttrib4hvNV(GLuint index, const GLhalf* v);
	extern void APIENTRY glVertexAttribs1hvNV(GLuint index, GLsizei n, const GLhalf* v);
	extern void APIENTRY glVertexAttribs2hvNV(GLuint index, GLsizei n, const GLhalf* v);
	extern void APIENTRY glVertexAttribs3hvNV(GLuint index, GLsizei n, const GLhalf* v);
	extern void APIENTRY glVertexAttribs4hvNV(GLuint index, GLsizei n, const GLhalf* v);
#endif
	typedef void (APIENTRY* PFNGLVERTEX2HNVPROC)(GLhalf x, GLhalf y);
	typedef void (APIENTRY* PFNGLVERTEX2HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEX3HNVPROC)(GLhalf x, GLhalf y, GLhalf z);
	typedef void (APIENTRY* PFNGLVERTEX3HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEX4HNVPROC)(GLhalf x, GLhalf y, GLhalf z, GLhalf w);
	typedef void (APIENTRY* PFNGLVERTEX4HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLNORMAL3HNVPROC)(GLhalf nx, GLhalf ny, GLhalf nz);
	typedef void (APIENTRY* PFNGLNORMAL3HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLCOLOR3HNVPROC)(GLhalf red, GLhalf green, GLhalf blue);
	typedef void (APIENTRY* PFNGLCOLOR3HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLCOLOR4HNVPROC)(GLhalf red, GLhalf green, GLhalf blue, GLhalf alpha);
	typedef void (APIENTRY* PFNGLCOLOR4HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLTEXCOORD1HNVPROC)(GLhalf s);
	typedef void (APIENTRY* PFNGLTEXCOORD1HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLTEXCOORD2HNVPROC)(GLhalf s, GLhalf t);
	typedef void (APIENTRY* PFNGLTEXCOORD2HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLTEXCOORD3HNVPROC)(GLhalf s, GLhalf t, GLhalf r);
	typedef void (APIENTRY* PFNGLTEXCOORD3HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLTEXCOORD4HNVPROC)(GLhalf s, GLhalf t, GLhalf r, GLhalf q);
	typedef void (APIENTRY* PFNGLTEXCOORD4HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1HNVPROC)(GLenum target, GLhalf s);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD1HVNVPROC)(GLenum target, const GLhalf* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2HNVPROC)(GLenum target, GLhalf s, GLhalf t);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD2HVNVPROC)(GLenum target, const GLhalf* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3HNVPROC)(GLenum target, GLhalf s, GLhalf t, GLhalf r);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD3HVNVPROC)(GLenum target, const GLhalf* v);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4HNVPROC)(GLenum target, GLhalf s, GLhalf t, GLhalf r, GLhalf q);
	typedef void (APIENTRY* PFNGLMULTITEXCOORD4HVNVPROC)(GLenum target, const GLhalf* v);
	typedef void (APIENTRY* PFNGLFOGCOORDHNVPROC)(GLhalf fog);
	typedef void (APIENTRY* PFNGLFOGCOORDHVNVPROC)(const GLhalf* fog);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3HNVPROC)(GLhalf red, GLhalf green, GLhalf blue);
	typedef void (APIENTRY* PFNGLSECONDARYCOLOR3HVNVPROC)(const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXWEIGHTHNVPROC)(GLhalf weight);
	typedef void (APIENTRY* PFNGLVERTEXWEIGHTHVNVPROC)(const GLhalf* weight);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1HNVPROC)(GLuint index, GLhalf x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1HVNVPROC)(GLuint index, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2HNVPROC)(GLuint index, GLhalf x, GLhalf y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2HVNVPROC)(GLuint index, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3HNVPROC)(GLuint index, GLhalf x, GLhalf y, GLhalf z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3HVNVPROC)(GLuint index, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4HNVPROC)(GLuint index, GLhalf x, GLhalf y, GLhalf z, GLhalf w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4HVNVPROC)(GLuint index, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS1HVNVPROC)(GLuint index, GLsizei n, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS2HVNVPROC)(GLuint index, GLsizei n, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS3HVNVPROC)(GLuint index, GLsizei n, const GLhalf* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS4HVNVPROC)(GLuint index, GLsizei n, const GLhalf* v);
#endif

#ifndef GL_NV_light_max_exponent
#define GL_NV_light_max_exponent 1
#endif

#ifndef GL_NV_multisample_filter_hint
#define GL_NV_multisample_filter_hint 1
#endif

#ifndef GL_NV_occlusion_query
#define GL_NV_occlusion_query 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glGenOcclusionQueriesNV(GLsizei n, GLuint* ids);
	extern void APIENTRY glDeleteOcclusionQueriesNV(GLsizei n, const GLuint* ids);
	extern void APIENTRY glIsOcclusionQueryNV(GLuint id);
	extern void APIENTRY glBeginOcclusionQueryNV(GLuint id);
	extern void APIENTRY glEndOcclusionQueryNV(GLvoid);
	extern void APIENTRY glGetOcclusionQueryivNV(GLuint id, GLenum pname, GLint* params);
	extern void APIENTRY glGetOcclusionQueryuivNV(GLuint id, GLenum pname, GLuint* params);
#endif
	typedef void (APIENTRY* PFNGLGENOCCLUSIONQUERIESNVPROC)(GLsizei n, GLuint* ids);
	typedef void (APIENTRY* PFNGLDELETEOCCLUSIONQUERIESNVPROC)(GLsizei n, const GLuint* ids);
	typedef void (APIENTRY* PFNGLISOCCLUSIONQUERYNVPROC)(GLuint id);
	typedef void (APIENTRY* PFNGLBEGINOCCLUSIONQUERYNVPROC)(GLuint id);
	typedef void (APIENTRY* PFNGLENDOCCLUSIONQUERYNVPROC)(GLvoid);
	typedef void (APIENTRY* PFNGLGETOCCLUSIONQUERYIVNVPROC)(GLuint id, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETOCCLUSIONQUERYUIVNVPROC)(GLuint id, GLenum pname, GLuint* params);
#endif

#ifndef GL_NV_packed_depth_stencil
#define GL_NV_packed_depth_stencil 1
#endif

#ifndef GL_NV_pixel_data_range
#define GL_NV_pixel_data_range 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glPixelDataRangeNV(GLenum target, GLsizei length, GLvoid* pointer);
	extern void APIENTRY glFlushPixelDataRangeNV(GLenum target);
#endif
	typedef void (APIENTRY* PFNGLPIXELDATARANGENVPROC)(GLenum target, GLsizei length, GLvoid* pointer);
	typedef void (APIENTRY* PFNGLFLUSHPIXELDATARANGENVPROC)(GLenum target);
#endif

#ifndef GL_NV_point_sprite
#define GL_NV_point_sprite 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glPointParameteriNV(GLenum pname, int param);
	extern void APIENTRY glPointParameterivNV(GLenum pname, const int* params);
#endif
	typedef void (APIENTRY* PFNGLPOINTPARAMETERINVPROC)(GLenum pname, int param);
	typedef void (APIENTRY* PFNGLPOINTPARAMETERIVNVPROC)(GLenum pname, const int* params);
#endif

#ifndef GL_NV_primitive_restart
#define GL_NV_primitive_restart 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glPrimitiveRestartNV(GLvoid);
	extern void APIENTRY glPrimitiveRestartIndexNV(GLuint index);
#endif
	typedef void (APIENTRY* PFNGLPRIMITIVERESTARTNVPROC)(GLvoid);
	typedef void (APIENTRY* PFNGLPRIMITIVERESTARTINDEXNVPROC)(GLuint index);
#endif

#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glCombinerParameterfvNV(GLenum, const GLfloat*);
	extern void APIENTRY glCombinerParameterfNV(GLenum, GLfloat);
	extern void APIENTRY glCombinerParameterivNV(GLenum, const GLint*);
	extern void APIENTRY glCombinerParameteriNV(GLenum, GLint);
	extern void APIENTRY glCombinerInputNV(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum);
	extern void APIENTRY glCombinerOutputNV(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean);
	extern void APIENTRY glFinalCombinerInputNV(GLenum, GLenum, GLenum, GLenum);
	extern void APIENTRY glGetCombinerInputParameterfvNV(GLenum, GLenum, GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetCombinerInputParameterivNV(GLenum, GLenum, GLenum, GLenum, GLint*);
	extern void APIENTRY glGetCombinerOutputParameterfvNV(GLenum, GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetCombinerOutputParameterivNV(GLenum, GLenum, GLenum, GLint*);
	extern void APIENTRY glGetFinalCombinerInputParameterfvNV(GLenum, GLenum, GLfloat*);
	extern void APIENTRY glGetFinalCombinerInputParameterivNV(GLenum, GLenum, GLint*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLCOMBINERPARAMETERFVNVPROC)(GLenum pname, const GLfloat* params);
	typedef void (APIENTRY* PFNGLCOMBINERPARAMETERFNVPROC)(GLenum pname, GLfloat param);
	typedef void (APIENTRY* PFNGLCOMBINERPARAMETERIVNVPROC)(GLenum pname, const GLint* params);
	typedef void (APIENTRY* PFNGLCOMBINERPARAMETERINVPROC)(GLenum pname, GLint param);
	typedef void (APIENTRY* PFNGLCOMBINERINPUTNVPROC)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
	typedef void (APIENTRY* PFNGLCOMBINEROUTPUTNVPROC)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
	typedef void (APIENTRY* PFNGLFINALCOMBINERINPUTNVPROC)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
	typedef void (APIENTRY* PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC)(GLenum stage, GLenum portion, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC)(GLenum stage, GLenum portion, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC)(GLenum variable, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC)(GLenum variable, GLenum pname, GLint* params);
#endif

#ifndef GL_NV_register_combiners2
#define GL_NV_register_combiners2 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void glCombinerStageParameterfvNV(GLenum stage, GLenum pname, const GLfloat* params);
	extern void glGetCombinerStageParameterfvNV(GLenum stage, GLenum pname, GLfloat* params);
#endif
	typedef void (APIENTRY* PFNGLCOMBINERSTAGEPARAMETERFVNVPROC)(GLenum stage, GLenum pname, const GLfloat* params);
	typedef void (APIENTRY* PFNGLGETCOMBINERSTAGEPARAMETERFVNVPROC)(GLenum stage, GLenum pname, GLfloat* params);
#endif

#ifndef GL_NV_stencil_two_side
#define GL_NV_stencil_two_side 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glActiveStencilFaceNV(GLenum face);
#endif
	typedef void (APIENTRY* PFNGLACTIVESTENCILFACENVPROC)(GLenum face);
#endif

#ifndef GL_NV_texgen_emboss
#define GL_NV_texgen_emboss 1
#endif

#ifndef GL_NV_texgen_reflection
#define GL_NV_texgen_reflection 1
#endif

#ifndef GL_NV_texture_compression_vtc
#define GL_NV_texture_compression_vtc 1
#endif

#ifndef GL_NV_texture_env_combine4
#define GL_NV_texture_env_combine4 1
#endif

#ifndef GL_NV_texture_rectangle
#define GL_NV_texture_rectangle 1
#endif

#ifndef GL_NV_texture_shader
#define GL_NV_texture_shader 1
#endif

#ifndef GL_NV_texture_shader2
#define GL_NV_texture_shader2 1
#endif

#ifndef GL_NV_texture_shader3
#define GL_NV_texture_shader3 1
#endif

#ifndef GL_NV_vertex_array_range
#define GL_NV_vertex_array_range 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void APIENTRY glFlushVertexArrayRangeNV(void);
	extern void APIENTRY glVertexArrayRangeNV(GLsizei, const GLvoid*);
#endif /* GL_GLEXT_PROTOTYPES */
	typedef void (APIENTRY* PFNGLFLUSHVERTEXARRAYRANGENVPROC)(void);
	typedef void (APIENTRY* PFNGLVERTEXARRAYRANGENVPROC)(GLsizei size, const GLvoid* pointer);
#endif

#ifndef GL_NV_vertex_array_range2
#define GL_NV_vertex_array_range2 1
#endif

#ifndef GL_NV_vertex_program
#define GL_NV_vertex_program 1
#ifdef GL_GLEXT_PROTOTYPES
	extern void glBindProgramNV(GLenum target, GLuint id);
	extern void glDeleteProgramsNV(GLsizei n, const GLuint* ids);
	extern void glExecuteProgramNV(GLenum target, GLuint id, const GLfloat* params);
	extern void glGenProgramsNV(GLsizei n, GLuint* ids);
	extern GLboolean glAreProgramsResidentNV(GLsizei n, const GLuint* ids, boolean* residences);
	extern void glRequestResidentProgramsNV(GLsizei n, GLuint* ids);
	extern void glGetProgramParameterfvNV(GLenum target, GLuint index, GLenum pname, GLfloat* params);
	extern void glGetProgramParameterdvNV(GLenum target, GLuint index, GLenum pname, GLdouble* params);
	extern void glGetProgramivNV(GLuint id, GLenum pname, int* params);
	extern void glGetProgramStringNV(GLuint id, GLenum pname, GLubyte* program);
	extern void glGetTrackMatrixivNV(GLenum target, GLuint address, GLenum pname, int* params);
	extern void glGetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble* params);
	extern void glGetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat* params);
	extern void glGetVertexAttribivNV(GLuint index, GLenum pname, int* params);
	extern void glGetVertexAttribPointervNV(GLuint index, GLenum pname, void** pointer);
	extern GLboolean glIsProgramNV(GLuint id);
	extern void glLoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte* program);
	extern void glProgramParameter4fNV(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void glProgramParameter4dNV(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void glProgramParameter4dvNV(GLenum target, GLuint index, const GLdouble* params);
	extern void glProgramParameter4fvNV(GLenum target, GLuint index, const GLfloat* params);
	extern void glProgramParameters4dvNV(GLenum target, GLuint index, GLuint num, const GLdouble* params);
	extern void glProgramParameters4fvNV(GLenum target, GLuint index, GLuint num, const GLfloat* params);
	extern void glTrackMatrixNV(GLenum target, GLuint address, GLenum matrix, GLenum transform);
	extern void glVertexAttribPointerNV(GLuint index, int size, GLenum type, GLsizei stride, const void* pointer);
	extern void glVertexAttrib1sNV(GLuint index, GLshort x);
	extern void glVertexAttrib1fNV(GLuint index, GLfloat x);
	extern void glVertexAttrib1dNV(GLuint index, GLdouble x);
	extern void glVertexAttrib2sNV(GLuint index, GLshort x, GLshort y);
	extern void glVertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y);
	extern void glVertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y);
	extern void glVertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z);
	extern void glVertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	extern void glVertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	extern void glVertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
	extern void glVertexAttrib4fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	extern void glVertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	extern void glVertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
	extern void glVertexAttrib1svNV(GLuint index, const GLshort* v);
	extern void glVertexAttrib1fvNV(GLuint index, const GLfloat* v);
	extern void glVertexAttrib1dvNV(GLuint index, const GLdouble* v);
	extern void glVertexAttrib2svNV(GLuint index, const GLshort* v);
	extern void glVertexAttrib2fvNV(GLuint index, const GLfloat* v);
	extern void glVertexAttrib2dvNV(GLuint index, const GLdouble* v);
	extern void glVertexAttrib3svNV(GLuint index, const GLshort* v);
	extern void glVertexAttrib3fvNV(GLuint index, const GLfloat* v);
	extern void glVertexAttrib3dvNV(GLuint index, const GLdouble* v);
	extern void glVertexAttrib4svNV(GLuint index, const GLshort* v);
	extern void glVertexAttrib4fvNV(GLuint index, const GLfloat* v);
	extern void glVertexAttrib4dvNV(GLuint index, const GLdouble* v);
	extern void glVertexAttrib4ubvNV(GLuint index, const GLubyte* v);
	extern void glVertexAttribs1svNV(GLuint index, GLsizei n, const GLshort* v);
	extern void glVertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat* v);
	extern void glVertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble* v);
	extern void glVertexAttribs2svNV(GLuint index, GLsizei n, const GLshort* v);
	extern void glVertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat* v);
	extern void glVertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble* v);
	extern void glVertexAttribs3svNV(GLuint index, GLsizei n, const GLshort* v);
	extern void glVertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat* v);
	extern void glVertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble* v);
	extern void glVertexAttribs4svNV(GLuint index, GLsizei n, const GLshort* v);
	extern void glVertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat* v);
	extern void glVertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble* v);
	extern void glVertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte* v);
#endif
	typedef GLboolean(APIENTRY* PFNGLAREPROGRAMSRESIDENTNVPROC)(GLsizei n, const GLuint* programs, GLboolean* residences);
	typedef void (APIENTRY* PFNGLBINDPROGRAMNVPROC)(GLenum target, GLuint id);
	typedef void (APIENTRY* PFNGLDELETEPROGRAMSNVPROC)(GLsizei n, const GLuint* programs);
	typedef void (APIENTRY* PFNGLEXECUTEPROGRAMNVPROC)(GLenum target, GLuint id, const GLfloat* params);
	typedef void (APIENTRY* PFNGLGENPROGRAMSNVPROC)(GLsizei n, GLuint* programs);
	typedef void (APIENTRY* PFNGLGETPROGRAMPARAMETERDVNVPROC)(GLenum target, GLuint index, GLenum pname, GLdouble* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMPARAMETERFVNVPROC)(GLenum target, GLuint index, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMIVNVPROC)(GLuint id, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETPROGRAMSTRINGNVPROC)(GLuint id, GLenum pname, GLubyte* program);
	typedef void (APIENTRY* PFNGLGETTRACKMATRIXIVNVPROC)(GLenum target, GLuint address, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBDVNVPROC)(GLuint index, GLenum pname, GLdouble* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBFVNVPROC)(GLuint index, GLenum pname, GLfloat* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBIVNVPROC)(GLuint index, GLenum pname, GLint* params);
	typedef void (APIENTRY* PFNGLGETVERTEXATTRIBPOINTERVNVPROC)(GLuint index, GLenum pname, GLvoid* *pointer);
	typedef GLboolean(APIENTRY* PFNGLISPROGRAMNVPROC)(GLuint id);
	typedef void (APIENTRY* PFNGLLOADPROGRAMNVPROC)(GLenum target, GLuint id, GLsizei len, const GLubyte* program);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETER4DNVPROC)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETER4DVNVPROC)(GLenum target, GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETER4FNVPROC)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETER4FVNVPROC)(GLenum target, GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETERS4DVNVPROC)(GLenum target, GLuint index, GLsizei count, const GLdouble* v);
	typedef void (APIENTRY* PFNGLPROGRAMPARAMETERS4FVNVPROC)(GLenum target, GLuint index, GLsizei count, const GLfloat* v);
	typedef void (APIENTRY* PFNGLREQUESTRESIDENTPROGRAMSNVPROC)(GLsizei n, const GLuint* programs);
	typedef void (APIENTRY* PFNGLTRACKMATRIXNVPROC)(GLenum target, GLuint address, GLenum matrix, GLenum transform);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBPOINTERNVPROC)(GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid* pointer);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1DNVPROC)(GLuint index, GLdouble x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1DVNVPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1FNVPROC)(GLuint index, GLfloat x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1FVNVPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1SNVPROC)(GLuint index, GLshort x);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB1SVNVPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2DNVPROC)(GLuint index, GLdouble x, GLdouble y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2DVNVPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2FNVPROC)(GLuint index, GLfloat x, GLfloat y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2FVNVPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2SNVPROC)(GLuint index, GLshort x, GLshort y);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB2SVNVPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3DNVPROC)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3DVNVPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3FNVPROC)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3FVNVPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3SNVPROC)(GLuint index, GLshort x, GLshort y, GLshort z);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB3SVNVPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4DNVPROC)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4DVNVPROC)(GLuint index, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4FNVPROC)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4FVNVPROC)(GLuint index, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4SNVPROC)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4SVNVPROC)(GLuint index, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4UBNVPROC)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
	typedef void (APIENTRY* PFNGLVERTEXATTRIB4UBVNVPROC)(GLuint index, const GLubyte* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS1DVNVPROC)(GLuint index, GLsizei count, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS1FVNVPROC)(GLuint index, GLsizei count, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS1SVNVPROC)(GLuint index, GLsizei count, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS2DVNVPROC)(GLuint index, GLsizei count, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS2FVNVPROC)(GLuint index, GLsizei count, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS2SVNVPROC)(GLuint index, GLsizei count, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS3DVNVPROC)(GLuint index, GLsizei count, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS3FVNVPROC)(GLuint index, GLsizei count, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS3SVNVPROC)(GLuint index, GLsizei count, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS4DVNVPROC)(GLuint index, GLsizei count, const GLdouble* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS4FVNVPROC)(GLuint index, GLsizei count, const GLfloat* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS4SVNVPROC)(GLuint index, GLsizei count, const GLshort* v);
	typedef void (APIENTRY* PFNGLVERTEXATTRIBS4UBVNVPROC)(GLuint index, GLsizei count, const GLubyte* v);
#endif

#ifndef GL_NV_vertex_program1_1
#define GL_NV_vertex_program1_1 1
#endif

#ifndef GL_NV_vertex_program2
#define GL_NV_vertex_program2 1
#endif

#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap 1
#endif

#ifndef GL_SGIS_texture_lod
#define GL_SGIS_texture_lod 1
#endif

#ifndef GL_SGIX_depth_texture
#define GL_SGIX_depth_texture 1
#endif

#ifndef GL_SGIX_shadow
#define GL_SGIX_shadow 1
#endif

#ifdef __cplusplus
}
#endif

#endif

