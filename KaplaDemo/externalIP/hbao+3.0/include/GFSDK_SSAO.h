/* 
* Copyright (c) 2008-2016, NVIDIA CORPORATION. All rights reserved. 
* 
* NVIDIA CORPORATION and its licensors retain all intellectual property 
* and proprietary rights in and to this software, related documentation 
* and any modifications thereto. Any use, reproduction, disclosure or 
* distribution of this software and related documentation without an express 
* license agreement from NVIDIA CORPORATION is strictly prohibited. 
*/

/*====================================================================================================
-------------------------------------------------------------------------------------------
                                           HBAO+                                           
-------------------------------------------------------------------------------------------

HBAO+ is a SSAO algorithm designed to achieve high efficiency on DX11 GPUs.
The algorithm is based on HBAO [Bavoil and Sainz 2008], with the following differences:

(1.) To minimize cache trashing, HBAO+ does not use any randomization texture.
Instead, the algorithm uses an Interleaved Rendering approach, generating the AO
in multiple passes with a unique jitter value per pass [Bavoil and Jansen 2013].

(2.) To avoid over-occlusion artifacts, HBAO+ uses a simpler AO approximation than HBAO,
similar to "Scalable Ambient Obscurance" [McGuire et al. 2012] [Bukowski et al. 2012].

(3.) To minimize flickering, the HBAO+ is always rendered in full resolution,
from full-resolution depths.

[Bavoil et al. 2008] "Image-Space Horizon-Based Ambient Occlusion"
http://www.nvidia.com/object/siggraph-2008-HBAO.html

[McGuire et al. 2012] "Scalable Ambient Obscurance"
http://graphics.cs.williams.edu/papers/SAOHPG12/

[Bukowski et al. 2012] "Scalable High-Quality Motion Blur and Ambient Occlusion"
http://graphics.cs.williams.edu/papers/VVSIGGRAPH12/

[Bavoil and Jansen 2013] "Particle Shadows & Cache-Efficient Post-Processing"
https://developer.nvidia.com/gdc-2013

-------------------------------------------------------------------------------------------
                                           MSAA                                            
-------------------------------------------------------------------------------------------

(1.) The input depth & normal textures are required to have matching dimensions and MSAA sample count.

(2.) The output render target can have arbitrary dimensions and MSAA sample count.

(3.) If the input textures are MSAA, only sample 0 is used to render the AO.

(4.) If the output render target is MSAA, a per-pixel AO value is written to all samples.

-------------------------------------------------------------------------------------------
                                       PERFORMANCE                                        
-------------------------------------------------------------------------------------------

In 1920x1080 1xAA, using HARDWARE_DEPTHS as input and BLUR_RADIUS_4, the RenderAO call takes 
up to 46 MB of video memory and 2.3 ms / frame on GeForce GTX 680.

-------------------------------------------------------------------------------------------
                                    INTEGRATION EXAMPLE                                    
-------------------------------------------------------------------------------------------

(1.) INITIALIZE THE LIBRARY

    GFSDK_SSAO_CustomHeap CustomHeap;
    CustomHeap.new_ = ::operator new;
    CustomHeap.delete_ = ::operator delete;

    GFSDK_SSAO_Status status;
    GFSDK_SSAO_Context_D3D11* pAOContext;
    status = GFSDK_SSAO_CreateContext_D3D11(pD3D11Device, &pAOContext, &CustomHeap);
    assert(status == GFSDK_SSAO_OK); // HBAO+ requires feature level 11_0 or above

(2.) SET INPUT DEPTHS

    GFSDK_SSAO_InputData_D3D11 Input;
    Input.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
    Input.DepthData.pFullResDepthTextureSRV = pDepthStencilTextureSRV;
    Input.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4(pProjectionMatrix);
    Input.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
    Input.DepthData.MetersToViewSpaceUnits = SceneScale;

(3.) SET AO PARAMETERS

    GFSDK_SSAO_Parameters_D3D11 Params;
    Params.Radius = 2.f;
    Params.Bias = 0.1f;
    Params.PowerExponent = 2.f;
    Params.Blur.Enable = true;
    Params.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
    Params.Blur.Sharpness = 16.f;

(4.) SET RENDER TARGET

    GFSDK_SSAO_Output_D3D11 Output;
    Output.pRenderTargetView = pOutputColorRTV;
    Output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;

(5.) RENDER AO

    status = pAOContext->RenderAO(pD3D11Context, Input, Params, Output);
    assert(status == GFSDK_SSAO_OK);

-------------------------------------------------------------------------------------------
                                 DOCUMENTATION AND SUPPORT                                 
-------------------------------------------------------------------------------------------

    See the documentation for more information on the input requirements and the parameters.
    For any feedback or questions about this library, please contact devsupport@nvidia.com.

====================================================================================================*/

#pragma once
#pragma pack(push,8) // Make sure we have consistent structure packings

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*====================================================================================================
   Entry-point declarations.
====================================================================================================*/

#if defined(ANDROID) || defined(LINUX) || defined(MACOSX)
#define GFSDK_SSAO_CDECL
#define GFSDK_SSAO_EXPORT
#define GFSDK_SSAO_STDCALL
#else
#define GFSDK_SSAO_CDECL __cdecl
#define GFSDK_SSAO_EXPORT __declspec(dllexport)
#define GFSDK_SSAO_STDCALL __stdcall
#endif

#if GFSDK_SSAO_DYNAMIC_LOAD_LIBRARY

#define GFSDK_SSAO_DECL(RETURN_TYPE, FUNCTION_NAME, ...) typedef RETURN_TYPE (*PFN_##FUNCTION_NAME)(__VA_ARGS__); extern PFN_##FUNCTION_NAME FUNCTION_NAME
#define GFSDK_SSAO_VERSION_ARGUMENT GFSDK_SSAO_Version HeaderVersion
#define GFSDK_SSAO_CUSTOM_HEAP_ARGUMENT const GFSDK_SSAO_CustomHeap* pCustomHeap

#else

#if !_WINDLL
#define GFSDK_SSAO_DECL(RETURN_TYPE, FUNCTION_NAME, ...) extern "C" RETURN_TYPE GFSDK_SSAO_CDECL FUNCTION_NAME(__VA_ARGS__)
#else
#define GFSDK_SSAO_DECL(RETURN_TYPE, FUNCTION_NAME, ...) extern "C" GFSDK_SSAO_EXPORT RETURN_TYPE GFSDK_SSAO_CDECL FUNCTION_NAME(__VA_ARGS__)
#endif

#define GFSDK_SSAO_VERSION_ARGUMENT GFSDK_SSAO_Version HeaderVersion = GFSDK_SSAO_Version()
#define GFSDK_SSAO_CUSTOM_HEAP_ARGUMENT const GFSDK_SSAO_CustomHeap* pCustomHeap = NULL

#endif

/*====================================================================================================
   Forward declarations.
====================================================================================================*/

typedef char GLchar;
typedef int GLint;
typedef int GLsizei;
typedef void GLvoid;
typedef float GLfloat;
typedef float GLclampf;
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef unsigned int GFSDK_SSAO_BOOL;
typedef unsigned int GFSDK_SSAO_UINT;
typedef float GFSDK_SSAO_FLOAT;
typedef size_t GFSDK_SSAO_SIZE_T;
typedef uint64_t GFSDK_SSAO_UINT64;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11DepthStencilView;
struct ID3D11RenderTargetView;

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct D3D12_BLEND_DESC;

/*====================================================================================================
   Build version.
====================================================================================================*/

struct GFSDK_SSAO_Version
{
    GFSDK_SSAO_Version()
        : Major(3)
        , Minor(0)
        , Branch(0)
        , Revision(0)
    {
    }

    GFSDK_SSAO_UINT Major;
    GFSDK_SSAO_UINT Minor;
    GFSDK_SSAO_UINT Branch;
    GFSDK_SSAO_UINT Revision;
};

/*====================================================================================================
   Enums.
====================================================================================================*/

enum GFSDK_SSAO_Status
{
    GFSDK_SSAO_OK,                                          // Success
    GFSDK_SSAO_VERSION_MISMATCH,                            // The header version number does not match the DLL version number
    GFSDK_SSAO_NULL_ARGUMENT,                               // One of the required argument pointers is NULL
    GFSDK_SSAO_INVALID_PROJECTION_MATRIX,                   // The projection matrix is not valid
    GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX,                // The world-to-view matrix is not valid (transposing it may help)
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION,           // The normal-texture resolution does not match the depth-texture resolution
    GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT,         // The normal-texture sample count does not match the depth-texture sample count
    GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS,                 // One of the viewport dimensions (width or height) is 0
    GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE,                // The viewport depth range is not a sub-range of [0.f,1.f]
    GFSDK_SSAO_MEMORY_ALLOCATION_FAILED,                    // Failed to allocate memory on the heap
    GFSDK_SSAO_INVALID_DEPTH_STENCIL_RESOLUTION,            // The depth-stencil resolution does not match the output render-target resolution
    GFSDK_SSAO_INVALID_DEPTH_STENCIL_SAMPLE_COUNT,          // The depth-stencil sample count does not match the output render-target sample count
    //
    // D3D-specific enums
    //
    GFSDK_SSAO_D3D_FEATURE_LEVEL_NOT_SUPPORTED,             // The current D3D11 feature level is lower than 11_0
    GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED,                // A resource-creation call has failed (running out of memory?)
    GFSDK_SSAO_D3D12_UNSUPPORTED_DEPTH_CLAMP_MODE,          // CLAMP_TO_BORDER is used (implemented on D3D11 & GL, but not on D3D12)
    GFSDK_SSAO_D3D12_INVALID_HEAP_TYPE,                     // One of the heaps provided to GFSDK_SSAO_CreateContext_D3D12 has an unexpected type
    GFSDK_SSAO_D3D12_INSUFFICIENT_DESCRIPTORS,              // One of the heaps provided to GFSDK_SSAO_CreateContext_D3D12 has insufficient descriptors
    GFSDK_SSAO_D3D12_INVALID_NODE_MASK,                     // NodeMask has more than one bit set. HBAO+ only supports operation on one D3D12 device node.
    //
    // GL-specific enums
    //
    GFSDK_SSAO_GL_INVALID_TEXTURE_TARGET,                   // One of the input textures is not GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
    GFSDK_SSAO_GL_INVALID_TEXTURE_OBJECT,                   // One of the input texture objects has index 0
    GFSDK_SSAO_GL_RESOURCE_CREATION_FAILED,                 // A GL resource-creation call has failed (running out of memory?)
    GFSDK_SSAO_GL_NULL_FUNCTION_POINTER,                    // One of the provided GL function pointers is NULL
    GFSDK_SSAO_GL_UNSUPPORTED_VIEWPORT,                     // A custom input viewport is enabled (not supported on GL)
};

enum GFSDK_SSAO_DepthTextureType
{
    GFSDK_SSAO_HARDWARE_DEPTHS,                             // Non-linear depths in the range [0.f,1.f]
    GFSDK_SSAO_HARDWARE_DEPTHS_SUB_RANGE,                   // Non-linear depths in the range [Viewport.MinDepth,Viewport.MaxDepth]
    GFSDK_SSAO_VIEW_DEPTHS,                                 // Linear depths in the range [ZNear,ZFar]
};

enum GFSDK_SSAO_BlendMode
{
    GFSDK_SSAO_OVERWRITE_RGB,                               // Overwrite the destination RGB with the AO, preserving alpha
    GFSDK_SSAO_MULTIPLY_RGB,                                // Multiply the AO over the destination RGB, preserving alpha
    GFSDK_SSAO_CUSTOM_BLEND,                                // Composite the AO using a custom blend state
};

enum GFSDK_SSAO_DepthStencilMode
{
    GFSDK_SSAO_DISABLED_DEPTH_STENCIL,                      // Composite the AO without any depth-stencil testing
    GFSDK_SSAO_CUSTOM_DEPTH_STENCIL,                        // Composite the AO with a custom depth-stencil state
};

enum GFSDK_SSAO_BlurRadius
{
    GFSDK_SSAO_BLUR_RADIUS_2,                               // Kernel radius = 2 pixels
    GFSDK_SSAO_BLUR_RADIUS_4,                               // Kernel radius = 4 pixels (recommended)
};

enum GFSDK_SSAO_DepthStorage
{
    GFSDK_SSAO_FP16_VIEW_DEPTHS,                            // Store the internal view depths in FP16 (recommended)
    GFSDK_SSAO_FP32_VIEW_DEPTHS,                            // Store the internal view depths in FP32 (slower)
};

enum GFSDK_SSAO_DepthClampMode
{
    GFSDK_SSAO_CLAMP_TO_EDGE,                               // Use clamp-to-edge when sampling depth (may cause false occlusion near screen borders)
    GFSDK_SSAO_CLAMP_TO_BORDER,                             // Use clamp-to-border when sampling depth (may cause halos near screen borders)
};

enum GFSDK_SSAO_MatrixLayout
{
    GFSDK_SSAO_ROW_MAJOR_ORDER,                             // The matrix is stored as Row[0],Row[1],Row[2],Row[3]
    GFSDK_SSAO_COLUMN_MAJOR_ORDER,                          // The matrix is stored as Col[0],Col[1],Col[2],Col[3]
};

enum GFSDK_SSAO_RenderMask
{
    GFSDK_SSAO_DRAW_Z                              = (1 << 0),  // Linearize the input depths
    GFSDK_SSAO_DRAW_AO                             = (1 << 1),  // Render AO based on pre-linearized depths
    GFSDK_SSAO_DRAW_DEBUG_N                        = (1 << 2),  // Render the internal view normals (for debugging)
    GFSDK_SSAO_DRAW_DEBUG_X                        = (1 << 3),  // Render the X component as grayscale
    GFSDK_SSAO_DRAW_DEBUG_Y                        = (1 << 4),  // Render the Y component as grayscale
    GFSDK_SSAO_DRAW_DEBUG_Z                        = (1 << 5),  // Render the Z component as grayscale
    GFSDK_SSAO_RENDER_AO                           = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_AO,
    GFSDK_SSAO_RENDER_DEBUG_NORMAL                 = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_DEBUG_N,
    GFSDK_SSAO_RENDER_DEBUG_NORMAL_X               = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_DEBUG_N | GFSDK_SSAO_DRAW_DEBUG_X,
    GFSDK_SSAO_RENDER_DEBUG_NORMAL_Y               = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_DEBUG_N | GFSDK_SSAO_DRAW_DEBUG_Y,
    GFSDK_SSAO_RENDER_DEBUG_NORMAL_Z               = GFSDK_SSAO_DRAW_Z | GFSDK_SSAO_DRAW_DEBUG_N | GFSDK_SSAO_DRAW_DEBUG_Z,
};

/*====================================================================================================
   Input/output textures.
====================================================================================================*/

struct GFSDK_SSAO_ShaderResourceView_D3D12
{
    GFSDK_SSAO_ShaderResourceView_D3D12()
        : pResource(NULL)
        , GpuHandle(0)
    {
    }
    ID3D12Resource*     pResource;
    GFSDK_SSAO_UINT64   GpuHandle;
};

struct GFSDK_SSAO_RenderTargetView_D3D12
{
    GFSDK_SSAO_RenderTargetView_D3D12()
        : pResource(NULL)
        , CpuHandle(0)
    {
    }
    ID3D12Resource*     pResource;
    GFSDK_SSAO_SIZE_T   CpuHandle;
};

struct GFSDK_SSAO_Texture_GL
{
    GLenum Target;                                              // Must be GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
    GLuint TextureId;                                           // OpenGL texture object index

    GFSDK_SSAO_Texture_GL()
        : Target(0)
        , TextureId(0)
    {
    }
    GFSDK_SSAO_Texture_GL(GLenum InTarget, GLuint InTextureId)
        : Target(InTarget)
        , TextureId(InTextureId)
    {
    }
};

/*====================================================================================================
   Input data.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// 4x4 Matrix.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_Float4x4
{
    GFSDK_SSAO_Float4x4()
    {
        memset(&Array, 0, sizeof(Array));
    }
    GFSDK_SSAO_Float4x4(const GFSDK_SSAO_FLOAT* pMatrix)
    {
        memcpy(&Array, pMatrix, sizeof(Array));
    }
    GFSDK_SSAO_FLOAT Array[16];
};

struct GFSDK_SSAO_Matrix
{
    GFSDK_SSAO_Float4x4         Data;                       // 4x4 matrix stored as an array of 16 floats
    GFSDK_SSAO_MatrixLayout     Layout;                     // Memory layout of the matrix

    GFSDK_SSAO_Matrix()
        : Layout(GFSDK_SSAO_ROW_MAJOR_ORDER)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// [Optional] Input viewport.
//
// Remarks:
//    * The Viewport defines a sub-area of the input & output full-resolution textures to be sourced and rendered to.
//      Only the depth pixels within the viewport sub-area contribute to the RenderAO output.
//    * The Viewport's MinDepth and MaxDepth values are ignored except if DepthTextureType is HARDWARE_DEPTHS_SUB_RANGE.
//    * If Enable is false, a default input viewport is used with:
//          TopLeftX = 0
//          TopLeftY = 0
//          Width    = InputDepthTexture.Width
//          Height   = InputDepthTexture.Height
//          MinDepth = 0.f
//          MaxDepth = 1.f
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_InputViewport
{
    GFSDK_SSAO_BOOL     Enable;                             // To use the provided viewport data (instead of the default viewport)
    GFSDK_SSAO_UINT     TopLeftX;                           // X coordinate of the top-left corner of the viewport rectangle, in pixels
    GFSDK_SSAO_UINT     TopLeftY;                           // Y coordinate of the top-left corner of the viewport rectangle, in pixels
    GFSDK_SSAO_UINT     Width;                              // The width of the viewport rectangle, in pixels
    GFSDK_SSAO_UINT     Height;                             // The height of the viewport rectangle, in pixels
    GFSDK_SSAO_FLOAT    MinDepth;                           // The minimum depth for GFSDK_SSAO_HARDWARE_DEPTHS_SUB_RANGE
    GFSDK_SSAO_FLOAT    MaxDepth;                           // The maximum depth for GFSDK_SSAO_HARDWARE_DEPTHS_SUB_RANGE

    GFSDK_SSAO_InputViewport()
        : Enable(false)
        , TopLeftX(0)
        , TopLeftY(0)
        , Width(0)
        , Height(0)
        , MinDepth(0.f)
        , MaxDepth(1.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// Input depth data.
//
// Requirements:
//    * View-space depths (linear) are required to be non-multisample.
//    * Hardware depths (non-linear) can be multisample or non-multisample.
//    * The projection matrix must have the following form, with |P23| == 1.f:
//       { P00, 0.f, 0.f, 0.f }
//       { 0.f, P11, 0.f, 0.f }
//       { P20, P21, P22, P23 }
//       { 0.f, 0.f, P32, 0.f }
//
// Remarks:
//    * MetersToViewSpaceUnits is used to convert the AO radius parameter from meters to view-space units,
//      as well as to convert the blur sharpness parameter from inverse meters to inverse view-space units.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_InputDepthData
{
    GFSDK_SSAO_DepthTextureType     DepthTextureType;           // HARDWARE_DEPTHS, HARDWARE_DEPTHS_SUB_RANGE or VIEW_DEPTHS
    GFSDK_SSAO_Matrix               ProjectionMatrix;           // 4x4 perspective matrix from the depth generation pass
    GFSDK_SSAO_FLOAT                MetersToViewSpaceUnits;     // DistanceInViewSpaceUnits = MetersToViewSpaceUnits * DistanceInMeters
    GFSDK_SSAO_InputViewport        Viewport;                   // Viewport from the depth generation pass

    GFSDK_SSAO_InputDepthData()
        : DepthTextureType(GFSDK_SSAO_HARDWARE_DEPTHS)
        , MetersToViewSpaceUnits(1.f)
    {
    }
};

struct GFSDK_SSAO_InputDepthData_D3D12 : GFSDK_SSAO_InputDepthData
{
    GFSDK_SSAO_ShaderResourceView_D3D12 FullResDepthTextureSRV; // Full-resolution depth texture
};

struct GFSDK_SSAO_InputDepthData_D3D11 : GFSDK_SSAO_InputDepthData
{
    ID3D11ShaderResourceView*       pFullResDepthTextureSRV;    // Full-resolution depth texture

    GFSDK_SSAO_InputDepthData_D3D11()
        : pFullResDepthTextureSRV(NULL)
     {
     }
};

struct GFSDK_SSAO_InputDepthData_GL : GFSDK_SSAO_InputDepthData
{
    GFSDK_SSAO_Texture_GL FullResDepthTexture;                  // Full-resolution depth texture
};

//---------------------------------------------------------------------------------------------------
// [Optional] Input normal data.
//
// Requirements:
//    * The normal texture is required to contain world-space normals in RGB.
//    * The normal texture must have the same resolution and MSAA sample count as the input depth texture.
//    * The view-space Y & Z axis are assumed to be pointing up & forward respectively (left-handed projection).
//    * The WorldToView matrix is assumed to not contain any non-uniform scaling.
//    * The WorldView matrix must have the following form:
//       { M00, M01, M02, 0.f }
//       { M10, M11, M12, 0.f }
//       { M20, M21, M22, 0.f }
//       { M30, M31, M32, M33 }
//
// Remarks:
//    * The actual view-space normal used for the AO rendering is:
//      N = normalize( mul( FetchedNormal.xyz * DecodeScale + DecodeBias, (float3x3)WorldToViewMatrix ) )
//    * Using bent normals as input may result in false-occlusion (overdarkening) artifacts.
//      Such artifacts may be alleviated by increasing the AO Bias parameter.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_InputNormalData
{
    GFSDK_SSAO_BOOL                 Enable;                         // To use the provided normals (instead of reconstructed ones)
    GFSDK_SSAO_Matrix               WorldToViewMatrix;              // 4x4 WorldToView matrix from the depth generation pass
    GFSDK_SSAO_FLOAT                DecodeScale;                    // Optional pre-matrix scale
    GFSDK_SSAO_FLOAT                DecodeBias;                     // Optional pre-matrix bias

    GFSDK_SSAO_InputNormalData()
        : Enable(false)
        , DecodeScale(1.f)
        , DecodeBias(0.f)
    {
    }
};

struct GFSDK_SSAO_InputNormalData_D3D12 : GFSDK_SSAO_InputNormalData
{
    GFSDK_SSAO_ShaderResourceView_D3D12 FullResNormalTextureSRV;    // Full-resolution world-space normal texture
};

struct GFSDK_SSAO_InputNormalData_D3D11 : GFSDK_SSAO_InputNormalData
{
    ID3D11ShaderResourceView*   pFullResNormalTextureSRV;           // Full-resolution world-space normal texture

    GFSDK_SSAO_InputNormalData_D3D11()
        : pFullResNormalTextureSRV(NULL)
    {
    }
};

struct GFSDK_SSAO_InputNormalData_GL : GFSDK_SSAO_InputNormalData
{
    GFSDK_SSAO_Texture_GL       FullResNormalTexture;               // Full-resolution world-space normal texture
};

//---------------------------------------------------------------------------------------------------
// Input data.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_InputData_D3D12
{
    GFSDK_SSAO_InputDepthData_D3D12         DepthData;          // Required
    GFSDK_SSAO_InputNormalData_D3D12        NormalData;         // Optional GBuffer normals
};

struct GFSDK_SSAO_InputData_D3D11
{
    GFSDK_SSAO_InputDepthData_D3D11         DepthData;          // Required
    GFSDK_SSAO_InputNormalData_D3D11        NormalData;         // Optional GBuffer normals
};

struct GFSDK_SSAO_InputData_GL
{
    GFSDK_SSAO_InputDepthData_GL            DepthData;          // Required
    GFSDK_SSAO_InputNormalData_GL           NormalData;         // Optional GBuffer normals
};

/*====================================================================================================
   Parameters.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// When enabled, the screen-space AO kernel radius is:
// - inversely proportional to ViewDepth for ViewDepth > ForegroundViewDepth
// - uniform in screen-space for ViewDepth <= ForegroundViewDepth
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_ForegroundAO
{
    GFSDK_SSAO_BOOL Enable;                                     // Enabling this may have a small performance impact
    GFSDK_SSAO_FLOAT ForegroundViewDepth;                       // View-space depth at which the AO footprint should get clamped

    GFSDK_SSAO_ForegroundAO()
        : Enable(false)
        , ForegroundViewDepth(0.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// When enabled, the screen-space AO kernel radius is:
// - inversely proportional to ViewDepth for ViewDepth < BackgroundViewDepth
// - uniform in screen-space for ViewDepth >= BackgroundViewDepth (instead of falling off to zero)
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_BackgroundAO
{
    GFSDK_SSAO_BOOL Enable;                                     // Enabling this may have a small performance impact
    GFSDK_SSAO_FLOAT BackgroundViewDepth;                       // View-space depth at which the AO footprint should stop falling off with depth

    GFSDK_SSAO_BackgroundAO()
        : Enable(false)
        , BackgroundViewDepth(0.f)
    {
    }
};

struct GFSDK_SSAO_DepthThreshold
{
    GFSDK_SSAO_BOOL                 Enable;                     // To return white AO for ViewDepths > MaxViewDepth
    GFSDK_SSAO_FLOAT                MaxViewDepth;               // Custom view-depth threshold
    GFSDK_SSAO_FLOAT                Sharpness;                  // The higher, the sharper are the AO-to-white transitions

    GFSDK_SSAO_DepthThreshold()
        : Enable(false)
        , MaxViewDepth(0.f)
        , Sharpness(100.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// When enabled, the actual per-pixel blur sharpness value depends on the per-pixel view depth with:
//     LerpFactor = (PixelViewDepth - ForegroundViewDepth) / (BackgroundViewDepth - ForegroundViewDepth)
//     Sharpness = lerp(Sharpness*ForegroundSharpnessScale, Sharpness, saturate(LerpFactor))
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_BlurSharpnessProfile
{
    GFSDK_SSAO_BOOL                 Enable;                         // To make the blur sharper in the foreground
    GFSDK_SSAO_FLOAT                ForegroundSharpnessScale;       // Sharpness scale factor for ViewDepths <= ForegroundViewDepth
    GFSDK_SSAO_FLOAT                ForegroundViewDepth;            // Maximum view depth of the foreground depth range
    GFSDK_SSAO_FLOAT                BackgroundViewDepth;            // Minimum view depth of the background depth range

    GFSDK_SSAO_BlurSharpnessProfile ()
        : Enable(false)
        , ForegroundSharpnessScale(4.f)
        , ForegroundViewDepth(0.f)
        , BackgroundViewDepth(1.f)
    {
    }
};

struct GFSDK_SSAO_BlurParameters
{
    GFSDK_SSAO_BOOL                 Enable;                     // To blur the AO with an edge-preserving blur
    GFSDK_SSAO_BlurRadius           Radius;                     // BLUR_RADIUS_2 or BLUR_RADIUS_4
    GFSDK_SSAO_FLOAT                Sharpness;                  // The higher, the more the blur preserves edges // 0.0~16.0
    GFSDK_SSAO_BlurSharpnessProfile SharpnessProfile;           // Optional depth-dependent sharpness function

    GFSDK_SSAO_BlurParameters()
        : Enable(true)
        , Radius(GFSDK_SSAO_BLUR_RADIUS_4)
        , Sharpness(16.f)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// Remarks:
//    * The final occlusion is a weighted sum of 2 occlusion contributions. The NearAO and FarAO parameters are the weights.
//    * Setting the DepthStorage parameter to FP16_VIEW_DEPTHS is fastest but may introduce minor false-occlusion artifacts for large depths.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_Parameters
{
    GFSDK_SSAO_FLOAT                Radius;                     // The AO radius in meters
    GFSDK_SSAO_FLOAT                Bias;                       // To hide low-tessellation artifacts // 0.0~0.5
    GFSDK_SSAO_FLOAT                NearAO;                     // Scale factor for the near-range AO, the greater the darker // 1.0~4.0
    GFSDK_SSAO_FLOAT                FarAO;                      // Scale factor for the far-range AO, the greater the darker // 1.0~4.0
    GFSDK_SSAO_FLOAT                PowerExponent;              // The final AO output is pow(AO, powerExponent) // 1.0~8.0
    GFSDK_SSAO_ForegroundAO         ForegroundAO;               // To limit the occlusion scale in the foreground
    GFSDK_SSAO_BackgroundAO         BackgroundAO;               // To add larger-scale occlusion in the distance
    GFSDK_SSAO_DepthStorage         DepthStorage;               // Quality / performance tradeoff
    GFSDK_SSAO_DepthClampMode       DepthClampMode;             // To hide possible false-occlusion artifacts near screen borders
    GFSDK_SSAO_DepthThreshold       DepthThreshold;             // Optional Z threshold, to hide possible depth-precision artifacts
    GFSDK_SSAO_BlurParameters       Blur;                       // Optional AO blur, to blur the AO before compositing it

    GFSDK_SSAO_Parameters()
        : Radius(1.f)
        , Bias(0.1f)
        , NearAO(2.f)
        , FarAO(1.f)
        , PowerExponent(2.f)
        , DepthStorage(GFSDK_SSAO_FP16_VIEW_DEPTHS)
        , DepthClampMode(GFSDK_SSAO_CLAMP_TO_EDGE)
    {
    }
};

/*====================================================================================================
   Output.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// [Optional] Custom blend state.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_CustomBlendState_D3D12
{
    struct D3D12_BLEND_DESC*        pBlendState;
    const GFSDK_SSAO_FLOAT*         pBlendFactor;

    GFSDK_SSAO_CustomBlendState_D3D12()
        : pBlendState(NULL)
        , pBlendFactor(NULL)
    {
    }
};

struct GFSDK_SSAO_CustomBlendState_D3D11
{
    ID3D11BlendState*               pBlendState;
    const GFSDK_SSAO_FLOAT*         pBlendFactor;

    GFSDK_SSAO_CustomBlendState_D3D11()
        : pBlendState(NULL)
        , pBlendFactor(NULL)
    {
    }
};

struct GFSDK_SSAO_CustomBlendState_GL
{
    struct
    {
        GLenum ModeRGB;
        GLenum ModeAlpha;
    } BlendEquationSeparate;

    struct
    {
        GLenum SrcRGB;
        GLenum DstRGB;
        GLenum SrcAlpha;
        GLenum DstAlpha;
    } BlendFuncSeparate;

    struct
    {
        GLclampf R;
        GLclampf G;
        GLclampf B;
        GLclampf A;
    } BlendColor;

    struct
    {
        GLboolean R;
        GLboolean G;
        GLboolean B;
        GLboolean A;
    } ColorMask;

    GFSDK_SSAO_CustomBlendState_GL()
    {
        memset(this, 0, sizeof(*this));
    }
};

//---------------------------------------------------------------------------------------------------
// Compositing blend state.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_BlendState_D3D12
{
    GFSDK_SSAO_BlendMode                Mode;                   // OVERWRITE_RGB, MULTIPLY_RGB or CUSTOM_BLEND
    GFSDK_SSAO_CustomBlendState_D3D12   CustomState;            // Relevant only if Mode is CUSTOM_BLEND

    GFSDK_SSAO_BlendState_D3D12()
        : Mode(GFSDK_SSAO_OVERWRITE_RGB)
    {
    }
};

struct GFSDK_SSAO_BlendState_D3D11
{
    GFSDK_SSAO_BlendMode                Mode;                   // OVERWRITE_RGB, MULTIPLY_RGB or CUSTOM_BLEND
    GFSDK_SSAO_CustomBlendState_D3D11   CustomState;            // Relevant only if Mode is CUSTOM_BLEND

    GFSDK_SSAO_BlendState_D3D11()
        : Mode(GFSDK_SSAO_OVERWRITE_RGB)
    {
    }
};

struct GFSDK_SSAO_BlendState_GL
{
    GFSDK_SSAO_BlendMode                Mode;                   // OVERWRITE_RGB, MULTIPLY_RGB or CUSTOM_BLEND
    GFSDK_SSAO_CustomBlendState_GL      CustomState;            // Relevant only if Mode is CUSTOM_BLEND

    GFSDK_SSAO_BlendState_GL()
        : Mode(GFSDK_SSAO_OVERWRITE_RGB)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// [Optional] Custom depth-stencil state.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_CustomDepthStencilState_D3D11
{
    ID3D11DepthStencilState*        pDepthStencilState;
    GFSDK_SSAO_UINT                 StencilRef;

    GFSDK_SSAO_CustomDepthStencilState_D3D11()
        : pDepthStencilState(NULL)
        , StencilRef(0)
    {
    }
};

struct GFSDK_SSAO_DepthStencilState_D3D11
{
    GFSDK_SSAO_DepthStencilMode                 Mode;               // DISABLED_DEPTH_STENCIL or CUSTOM_DEPTH_STENCIL
    GFSDK_SSAO_CustomDepthStencilState_D3D11    CustomState;        // Relevant only if Mode is CUSTOM_DEPTH_STENCIL

    GFSDK_SSAO_DepthStencilState_D3D11()
        : Mode(GFSDK_SSAO_DISABLED_DEPTH_STENCIL)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// [Optional] Two-pass AO compositing.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_BlendPass_D3D11
{
    GFSDK_SSAO_BlendState_D3D11         Blend;
    GFSDK_SSAO_DepthStencilState_D3D11  DepthStencil;
};

//---------------------------------------------------------------------------------------------------
// Remarks:
//    * This can be useful to use different blend states depending on the stencil value.
//    * For instance: 1) for character pixels (with Stencil==A), write AO with blending disabled,
//                    2) for other pixels (with Stencil!=A), write AO with MIN blending.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_TwoPassBlend_D3D11
{
    GFSDK_SSAO_BOOL                     Enable;                 // When enabled, overrides any other compositing state
    ID3D11DepthStencilView*             pDepthStencilView;      // Used to mask the pixels in each of the 2 passses
    GFSDK_SSAO_BlendPass_D3D11          FirstPass;              // Blend & depth-stencil state for the first compositing pass
    GFSDK_SSAO_BlendPass_D3D11          SecondPass;             // Blend & depth-stencil state for the second compositing pass

    GFSDK_SSAO_TwoPassBlend_D3D11()
        : Enable(false)
        , pDepthStencilView(NULL)
    {
    }
};

//---------------------------------------------------------------------------------------------------
// Output render target & compositing state.
//---------------------------------------------------------------------------------------------------

struct GFSDK_SSAO_Output_D3D12
{
    GFSDK_SSAO_RenderTargetView_D3D12*  pRenderTargetView;      // Output render target of RenderAO
    GFSDK_SSAO_BlendState_D3D12         Blend;                  // Blend state used when writing the AO to pRenderTargetView

    GFSDK_SSAO_Output_D3D12()
        : pRenderTargetView(NULL)
    {
    }
};

struct GFSDK_SSAO_Output_D3D11
{
    ID3D11RenderTargetView*             pRenderTargetView;      // Output render target of RenderAO
    GFSDK_SSAO_BlendState_D3D11         Blend;                  // Blend state used when writing the AO to pRenderTargetView
    GFSDK_SSAO_TwoPassBlend_D3D11       TwoPassBlend;           // Optional two-pass compositing using depth-stencil masking

    GFSDK_SSAO_Output_D3D11()
        : pRenderTargetView(NULL)
    {
    }
};

struct GFSDK_SSAO_Output_GL
{
    GLuint                              OutputFBO;                  // Output Frame Buffer Object of RenderAO
    GFSDK_SSAO_BlendState_GL            Blend;                      // Blend state used when writing the AO to OutputFBO

    GFSDK_SSAO_Output_GL()
        : OutputFBO(0)
    {
    }
};

/*====================================================================================================
  [Optional] Let the library allocate its memory on a custom heap.
====================================================================================================*/

struct GFSDK_SSAO_CustomHeap
{
    GFSDK_SSAO_CustomHeap()
        : new_(NULL)
        , delete_(NULL)
    {
    }
    void* (*new_)(size_t);
    void (*delete_)(void*);
};

/*====================================================================================================
  [Optional] For debugging any issues with the input projection matrix.
====================================================================================================*/

struct GFSDK_SSAO_ProjectionMatrixDepthRange
{
    GFSDK_SSAO_ProjectionMatrixDepthRange()
        : ZNear(-1.f)
        , ZFar(-1.f)
    {
    }
    GFSDK_SSAO_FLOAT ZNear;
    GFSDK_SSAO_FLOAT ZFar;
};

/*====================================================================================================
   Base interface.
====================================================================================================*/

class GFSDK_SSAO_Context
{
public:

//---------------------------------------------------------------------------------------------------
// [Optional] Returns the amount of video memory allocated by the library, in bytes.
//---------------------------------------------------------------------------------------------------
virtual GFSDK_SSAO_UINT GetAllocatedVideoMemoryBytes() = 0;

}; //class GFSDK_SSAO_Context

//---------------------------------------------------------------------------------------------------
// [Optional] Returns the DLL version information.
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status, GFSDK_SSAO_GetVersion, GFSDK_SSAO_Version* pVersion);

/*====================================================================================================
   D3D11 interface.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Note: The RenderAO, PreCreateRTs and Release entry points should not be called simultaneously from different threads.
//---------------------------------------------------------------------------------------------------
class GFSDK_SSAO_Context_D3D11 : public GFSDK_SSAO_Context
{
public:

    //---------------------------------------------------------------------------------------------------
    // Renders SSAO.
    //
    // Remarks:
    //    * Allocates internal D3D render targets on first use, and re-allocates them when the viewport dimensions change.
    //    * All the relevant device-context states are saved and restored internally when entering and exiting the call.
    //    * Setting RenderMask = GFSDK_SSAO_RENDER_DEBUG_NORMAL_Z can be useful to visualize the normals used for the AO rendering.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS          - One of the viewport dimensions (width or height) is 0
    //     GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE         - The viewport depth range is not a sub-range of [0.f,1.f]
    //     GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX         - The world-to-view matrix is not valid (transposing it may help)
    //     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION    - The normal-texture resolution does not match the depth-texture resolution
    //     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT  - The normal-texture sample count does not match the depth-texture sample count
    //     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status RenderAO(
        ID3D11DeviceContext* pDeviceContext,
        const GFSDK_SSAO_InputData_D3D11& InputData,
        const GFSDK_SSAO_Parameters& Parameters,
        const GFSDK_SSAO_Output_D3D11& Output,
        GFSDK_SSAO_RenderMask RenderMask = GFSDK_SSAO_RENDER_AO) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Pre-creates all internal render targets for RenderAO.
    //
    // Remarks:
    //    * This call may be safely skipped since RenderAO creates its render targets on demand if they were not pre-created.
    //    * This call releases and re-creates the internal render targets if the provided resolution changes.
    //    * This call performs CreateTexture calls for all the relevant render targets.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status PreCreateRTs(
        const GFSDK_SSAO_Parameters& Parameters,
        GFSDK_SSAO_UINT ViewportWidth,
        GFSDK_SSAO_UINT ViewportHeight) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Gets the library-internal ZNear and ZFar values derived from the input projection matrix.
    //
    // Remarks:
    //    * HBAO+ supports all perspective projection matrices, with arbitrary ZNear and ZFar.
    //    * For reverse infinite projections, GetProjectionMatrixDepthRange should return ZNear=+INF and ZFar=0.f.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status GetProjectionMatrixDepthRange(
        const GFSDK_SSAO_InputData_D3D11& InputData,
        GFSDK_SSAO_ProjectionMatrixDepthRange& OutputDepthRange) = 0;

    //---------------------------------------------------------------------------------------------------
    // Releases all D3D objects created by the library (to be called right before releasing the D3D device).
    //---------------------------------------------------------------------------------------------------
    virtual void Release() = 0;

}; //class GFSDK_SSAO_Context_D3D11

//---------------------------------------------------------------------------------------------------
// Creates a GFSDK_SSAO_Context associated with the D3D11 device.
//
// Remarks:
//    * Allocates D3D11 resources internally.
//    * Allocates memory using the default "::operator new", or "pCustomHeap->new_" if provided.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_VERSION_MISMATCH                     - Invalid HeaderVersion (have you set HeaderVersion = GFSDK_SSAO_Version()?)
//     GFSDK_SSAO_MEMORY_ALLOCATION_FAILED             - Failed to allocate memory on the heap
//     GFSDK_SSAO_D3D_FEATURE_LEVEL_NOT_SUPPORTED      - The D3D feature level of pD3DDevice is lower than 11_0
//     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status, GFSDK_SSAO_CreateContext_D3D11,
    ID3D11Device* pD3DDevice,
    GFSDK_SSAO_Context_D3D11** ppContext,
    GFSDK_SSAO_CUSTOM_HEAP_ARGUMENT,
    GFSDK_SSAO_VERSION_ARGUMENT);

/*====================================================================================================
   GL interface.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Note: The RenderAO, PreCreateFBOs and Release entry points should not be called simultaneously from different threads.
//---------------------------------------------------------------------------------------------------
class GFSDK_SSAO_Context_GL : public GFSDK_SSAO_Context
{
public:

    //---------------------------------------------------------------------------------------------------
    // Renders SSAO.
    //
    // Remarks:
    //    * Allocates internal GL framebuffer objects on first use, and re-allocates them when the depth-texture resolution changes.
    //    * All the relevant GL states are saved and restored internally when entering and exiting the call.
    //    * Setting RenderMask = GFSDK_SSAO_RENDER_DEBUG_NORMAL_Z can be useful to visualize the normals used for the AO rendering.
    //    * The current GL PolygonMode is assumed to be GL_FILL.
    //    * The OutputFBO cannot contain a texture bound in InputData.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE         - The viewport depth range is not a sub-range of [0.f,1.f]
    //     GFSDK_SSAO_GL_INVALID_TEXTURE_TARGET            - One of the input textures is not GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE
    //     GFSDK_SSAO_GL_INVALID_TEXTURE_OBJECT            - One of the input texture objects has index 0
    //     GFSDK_SSAO_GL_RESOURCE_CREATION_FAILED          - A GL resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_GL_UNSUPPORTED_VIEWPORT              - A custom input viewport is enabled (not supported on GL)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status RenderAO(
        const GFSDK_SSAO_InputData_GL& InputData,
        const GFSDK_SSAO_Parameters& Parameters,
        const GFSDK_SSAO_Output_GL& Output,
        GFSDK_SSAO_RenderMask RenderMask = GFSDK_SSAO_RENDER_AO) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Pre-creates all internal FBOs for RenderAO.
    //
    // Remarks:
    //    * This call may be safely skipped since RenderAO creates its framebuffer objects on demand if they were not pre-created.
    //    * This call releases and re-creates the internal framebuffer objects if the provided resolution changes.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_GL_RESOURCE_CREATION_FAILED          - A GL resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status PreCreateFBOs(
        const GFSDK_SSAO_Parameters& Parameters,
        GFSDK_SSAO_UINT ViewportWidth,
        GFSDK_SSAO_UINT ViewportHeight) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Gets the library-internal ZNear and ZFar values derived from the input projection matrix.
    //
    // Remarks:
    //    * HBAO+ supports all perspective projection matrices, with arbitrary ZNear and ZFar.
    //    * For reverse infinite projections, GetProjectionMatrixDepthRange should return ZNear=+INF and ZFar=0.f.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status GetProjectionMatrixDepthRange(
        const GFSDK_SSAO_InputData_GL& InputData,
        GFSDK_SSAO_ProjectionMatrixDepthRange& OutputDepthRange) = 0;

    //---------------------------------------------------------------------------------------------------
    // Releases all GL resources created by the library.
    //---------------------------------------------------------------------------------------------------
    virtual void Release() = 0;

}; // class GFSDK_SSAO_Context_GL

//---------------------------------------------------------------------------------------------------
// GL functions used by the GL context, in alphabetic order.
// Requires GL 3.2 (Core Profile) or above.
//---------------------------------------------------------------------------------------------------
struct GFSDK_SSAO_GLFunctions
{
    void (GFSDK_SSAO_STDCALL* glActiveTexture) (GLenum texture);
    void (GFSDK_SSAO_STDCALL* glAttachShader) (GLuint program, GLuint shader);
    void (GFSDK_SSAO_STDCALL* glBindBuffer) (GLenum target, GLuint buffer);
    void (GFSDK_SSAO_STDCALL* glBindBufferBase) (GLenum target, GLuint index, GLuint buffer);
    void (GFSDK_SSAO_STDCALL* glBindFramebuffer) (GLenum target, GLuint framebuffer);
    void (GFSDK_SSAO_STDCALL* glBindFragDataLocation) (GLuint, GLuint, const GLchar*);
    void (GFSDK_SSAO_STDCALL* glBindTexture) (GLenum target, GLuint texture);
    void (GFSDK_SSAO_STDCALL* glBindVertexArray) (GLuint array);
    void (GFSDK_SSAO_STDCALL* glBlendColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (GFSDK_SSAO_STDCALL* glBlendEquationSeparate) (GLenum modeRGB, GLenum modeAlpha);
    void (GFSDK_SSAO_STDCALL* glBlendFuncSeparate) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
    void (GFSDK_SSAO_STDCALL* glBufferData) (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
    void (GFSDK_SSAO_STDCALL* glBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
    void (GFSDK_SSAO_STDCALL* glColorMaski) (GLuint, GLboolean, GLboolean, GLboolean, GLboolean);
    void (GFSDK_SSAO_STDCALL* glCompileShader) (GLuint shader);
    GLuint (GFSDK_SSAO_STDCALL* glCreateShader) (GLenum type);
    GLuint (GFSDK_SSAO_STDCALL* glCreateProgram) (void);
    void (GFSDK_SSAO_STDCALL* glDeleteBuffers) (GLsizei n, const GLuint* buffers);
    void (GFSDK_SSAO_STDCALL* glDeleteFramebuffers) (GLsizei n, const GLuint* framebuffers);
    void (GFSDK_SSAO_STDCALL* glDeleteProgram) (GLuint program);
    void (GFSDK_SSAO_STDCALL* glDeleteShader) (GLuint shader);
    void (GFSDK_SSAO_STDCALL* glDeleteTextures) (GLsizei n, const GLuint *textures);
    void (GFSDK_SSAO_STDCALL* glDeleteVertexArrays) (GLsizei n, const GLuint* arrays);
    void (GFSDK_SSAO_STDCALL* glDisable) (GLenum cap);
    void (GFSDK_SSAO_STDCALL* glDrawBuffers) (GLsizei n, const GLenum* bufs);
    void (GFSDK_SSAO_STDCALL* glEnable) (GLenum cap);
    void (GFSDK_SSAO_STDCALL* glDrawArrays) (GLenum mode, GLint first, GLsizei count);
    void (GFSDK_SSAO_STDCALL* glFramebufferTexture) (GLenum, GLenum, GLuint, GLint);
    void (GFSDK_SSAO_STDCALL* glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (GFSDK_SSAO_STDCALL* glFramebufferTextureLayer) (GLenum target,GLenum attachment, GLuint texture,GLint level,GLint layer);
    void (GFSDK_SSAO_STDCALL* glGenBuffers) (GLsizei n, GLuint* buffers);
    void (GFSDK_SSAO_STDCALL* glGenFramebuffers) (GLsizei n, GLuint* framebuffers);
    void (GFSDK_SSAO_STDCALL* glGenTextures) (GLsizei n, GLuint *textures);
    void (GFSDK_SSAO_STDCALL* glGenVertexArrays) (GLsizei n, GLuint* arrays);
    GLenum (GFSDK_SSAO_STDCALL* glGetError) (void);
    void (GFSDK_SSAO_STDCALL* glGetBooleani_v) (GLenum, GLuint, GLboolean*);
    void (GFSDK_SSAO_STDCALL* glGetFloatv) (GLenum pname, GLfloat *params);
    void (GFSDK_SSAO_STDCALL* glGetIntegerv) (GLenum pname, GLint *params);
    void (GFSDK_SSAO_STDCALL* glGetIntegeri_v) (GLenum target, GLuint index, GLint* data);
    void (GFSDK_SSAO_STDCALL* glGetProgramiv) (GLuint program, GLenum pname, GLint* param);
    void (GFSDK_SSAO_STDCALL* glGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    void (GFSDK_SSAO_STDCALL* glGetShaderiv) (GLuint shader, GLenum pname, GLint* param);
    void (GFSDK_SSAO_STDCALL* glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
    const GLubyte* (GFSDK_SSAO_STDCALL* glGetString) (GLenum name);
    GLuint (GFSDK_SSAO_STDCALL* glGetUniformBlockIndex) (GLuint program, const GLchar* uniformBlockName);
    GLint (GFSDK_SSAO_STDCALL* glGetUniformLocation) (GLuint program, const GLchar* name);
    void (GFSDK_SSAO_STDCALL* glGetTexLevelParameteriv) (GLenum target, GLint level, GLenum pname, GLint *params);
    GLboolean (GFSDK_SSAO_STDCALL* glIsEnabled) (GLenum cap);
    GLboolean (GFSDK_SSAO_STDCALL* glIsEnabledi) (GLenum, GLuint);
    void (GFSDK_SSAO_STDCALL* glLinkProgram) (GLuint program);
    void (GFSDK_SSAO_STDCALL* glPolygonOffset) (GLfloat factor, GLfloat units);
    void (GFSDK_SSAO_STDCALL* glShaderSource) (GLuint shader, GLsizei count, const GLchar* const* strings, const GLint* lengths);
    void (GFSDK_SSAO_STDCALL* glTexImage2D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (GFSDK_SSAO_STDCALL* glTexImage3D) (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (GFSDK_SSAO_STDCALL* glTexParameteri) (GLenum target, GLenum pname, GLint param);
    void (GFSDK_SSAO_STDCALL* glTexParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
    void (GFSDK_SSAO_STDCALL* glUniform1i) (GLint location, GLint v0);
    void (GFSDK_SSAO_STDCALL* glUniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
    void (GFSDK_SSAO_STDCALL* glUseProgram) (GLuint program);
    void (GFSDK_SSAO_STDCALL* glViewport) (GLint x, GLint y, GLsizei width, GLsizei height);
    typedef void (GFSDK_SSAO_STDCALL* glShaderSourceGLESType) (GLuint shader, GLsizei count, const GLchar* const* strings, const GLint* lengths);

    GFSDK_SSAO_GLFunctions()
    {
        memset(this, 0, sizeof(*this));
    }
};

//---------------------------------------------------------------------------------------------------
// Helper macro to initialize all the GL function pointers.
//---------------------------------------------------------------------------------------------------
#define GFSDK_SSAO_INIT_GL_FUNCTIONS(GL)\
{\
    GL.glActiveTexture = glActiveTexture;\
    GL.glAttachShader = glAttachShader;\
    GL.glBindBuffer = glBindBuffer;\
    GL.glBindBufferBase = glBindBufferBase;\
    GL.glBindFramebuffer = glBindFramebuffer;\
    GL.glBindFragDataLocation = glBindFragDataLocation;\
    GL.glBindTexture = glBindTexture;\
    GL.glBindVertexArray = glBindVertexArray;\
    GL.glBlendColor = glBlendColor;\
    GL.glBlendEquationSeparate = glBlendEquationSeparate;\
    GL.glBlendFuncSeparate = glBlendFuncSeparate;\
    GL.glBufferData = glBufferData;\
    GL.glBufferSubData = glBufferSubData;\
    GL.glColorMaski = glColorMaski;\
    GL.glCompileShader = glCompileShader;\
    GL.glCreateShader = glCreateShader;\
    GL.glCreateProgram = glCreateProgram;\
    GL.glDeleteBuffers = glDeleteBuffers;\
    GL.glDeleteFramebuffers = glDeleteFramebuffers;\
    GL.glDeleteProgram = glDeleteProgram;\
    GL.glDeleteShader = glDeleteShader;\
    GL.glDeleteTextures = glDeleteTextures;\
    GL.glDeleteVertexArrays = glDeleteVertexArrays;\
    GL.glDisable = glDisable;\
    GL.glDrawBuffers = glDrawBuffers;\
    GL.glEnable = glEnable;\
    GL.glDrawArrays = glDrawArrays;\
    GL.glFramebufferTexture = glFramebufferTexture;\
    GL.glFramebufferTexture2D = glFramebufferTexture2D;\
    GL.glFramebufferTextureLayer = glFramebufferTextureLayer;\
    GL.glGenBuffers = glGenBuffers;\
    GL.glGenFramebuffers = glGenFramebuffers;\
    GL.glGenTextures = glGenTextures;\
    GL.glGenVertexArrays = glGenVertexArrays;\
    GL.glGetError = glGetError;\
    GL.glGetBooleani_v = glGetBooleani_v;\
    GL.glGetFloatv = glGetFloatv;\
    GL.glGetIntegerv = glGetIntegerv;\
    GL.glGetIntegeri_v = glGetIntegeri_v;\
    GL.glGetProgramiv = glGetProgramiv;\
    GL.glGetProgramInfoLog = glGetProgramInfoLog;\
    GL.glGetShaderiv = glGetShaderiv;\
    GL.glGetShaderInfoLog = glGetShaderInfoLog;\
    GL.glGetString = glGetString;\
    GL.glGetUniformBlockIndex = glGetUniformBlockIndex;\
    GL.glGetUniformLocation = glGetUniformLocation;\
    GL.glGetTexLevelParameteriv = glGetTexLevelParameteriv;\
    GL.glIsEnabled = glIsEnabled;\
    GL.glIsEnabledi = glIsEnabledi;\
    GL.glLinkProgram = glLinkProgram;\
    GL.glPolygonOffset = glPolygonOffset;\
    GL.glShaderSource = (GFSDK_SSAO_GLFunctions::glShaderSourceGLESType)glShaderSource;\
    GL.glTexImage2D = glTexImage2D;\
    GL.glTexImage3D = glTexImage3D;\
    GL.glTexParameteri = glTexParameteri;\
    GL.glTexParameterfv = glTexParameterfv;\
    GL.glUniform1i = glUniform1i;\
    GL.glUniformBlockBinding = glUniformBlockBinding;\
    GL.glUseProgram = glUseProgram;\
    GL.glViewport = glViewport;\
}

//---------------------------------------------------------------------------------------------------
// Creates a GFSDK_SSAO_Context_GL associated with the current GL context.
//
// Remarks:
//    * Requires GL Core 3.2 or above.
//    * Allocates GL resources internally.
//    * Allocates memory using the default "::operator new", or "pCustomHeap->new_" if provided.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_VERSION_MISMATCH                     - Invalid HeaderVersion (have you set HeaderVersion = GFSDK_SSAO_Version()?)
//     GFSDK_SSAO_GL_RESOURCE_CREATION_FAILED          - A GL resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_GL_NULL_FUNCTION_POINTER,            - One of the provided GL function pointers is NULL
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status, GFSDK_SSAO_CreateContext_GL,
    GFSDK_SSAO_Context_GL** ppContext,
    const GFSDK_SSAO_GLFunctions* pGLFunctions,
    GFSDK_SSAO_CUSTOM_HEAP_ARGUMENT,
    GFSDK_SSAO_VERSION_ARGUMENT);

/*====================================================================================================
   D3D12 interface.
====================================================================================================*/

//---------------------------------------------------------------------------------------------------
// Note: The RenderAO, PreCreateRTs and Release entry points should not be called simultaneously from different threads.
//---------------------------------------------------------------------------------------------------
class GFSDK_SSAO_Context_D3D12 : public GFSDK_SSAO_Context
{
public:

    //---------------------------------------------------------------------------------------------------
    // Renders SSAO.
    //
    // Remarks:
    //    * Allocates internal D3D render targets on first use, and re-allocates them when the viewport dimensions change.
    //    * Setting RenderMask = GFSDK_SSAO_RENDER_DEBUG_NORMAL_Z can be useful to visualize the normals used for the AO rendering.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_INVALID_VIEWPORT_DIMENSIONS          - One of the viewport dimensions (width or height) is 0
    //     GFSDK_SSAO_INVALID_VIEWPORT_DEPTH_RANGE         - The viewport depth range is not a sub-range of [0.f,1.f]
    //     GFSDK_SSAO_INVALID_WORLD_TO_VIEW_MATRIX         - The world-to-view matrix is not valid (transposing it may help)
    //     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_RESOLUTION    - The normal-texture resolution does not match the depth-texture resolution
    //     GFSDK_SSAO_INVALID_NORMAL_TEXTURE_SAMPLE_COUNT  - The normal-texture sample count does not match the depth-texture sample count
    //     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status RenderAO(
        ID3D12CommandQueue* pCmdQueue,
        ID3D12GraphicsCommandList* pCmdList,
        const GFSDK_SSAO_InputData_D3D12& InputData,
        const GFSDK_SSAO_Parameters& Parameters,
        const GFSDK_SSAO_Output_D3D12& Output,
        GFSDK_SSAO_RenderMask RenderMask = GFSDK_SSAO_RENDER_AO) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Pre-creates all internal render targets for RenderAO.
    //
    // Remarks:
    //    * This call may be safely skipped since RenderAO creates its render targets on demand if they were not pre-created.
    //    * This call releases and re-creates the internal render targets if the provided resolution changes.
    //    * This call performs CreateCommittedResource calls for all the relevant render targets.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A D3D resource-creation call has failed (running out of memory?)
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status PreCreateRTs(
        ID3D12CommandQueue* pCmdQueue,
        const GFSDK_SSAO_Parameters& Parameters,
        GFSDK_SSAO_UINT ViewportWidth,
        GFSDK_SSAO_UINT ViewportHeight) = 0;

    //---------------------------------------------------------------------------------------------------
    // [Optional] Gets the library-internal ZNear and ZFar values derived from the input projection matrix.
    //
    // Remarks:
    //    * HBAO+ supports all perspective projection matrices, with arbitrary ZNear and ZFar.
    //    * For reverse infinite projections, GetProjectionMatrixDepthRange should return ZNear=+INF and ZFar=0.f.
    //
    // Returns:
    //     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
    //     GFSDK_SSAO_INVALID_PROJECTION_MATRIX            - The projection matrix is not valid
    //     GFSDK_SSAO_OK                                   - Success
    //---------------------------------------------------------------------------------------------------
    virtual GFSDK_SSAO_Status GetProjectionMatrixDepthRange(
        const GFSDK_SSAO_InputData_D3D12& InputData,
        GFSDK_SSAO_ProjectionMatrixDepthRange& OutputDepthRange) = 0;

    //---------------------------------------------------------------------------------------------------
    // Releases all D3D objects created by the library (to be called right before releasing the D3D device).
    //---------------------------------------------------------------------------------------------------
    virtual void Release() = 0;

}; //class GFSDK_SSAO_Context_D3D12

//---------------------------------------------------------------------------------------------------
//
// Remark:
//    * Describes the D3D12 descriptor heap space that is passed to the library
//
//---------------------------------------------------------------------------------------------------
#define GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12           38  // Number of required descriptors for RTV heap type
#define GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12   57  // Number of required descriptors for CBV/SRV/UAV heap type

struct GFSDK_SSAO_DescriptorHeapRange_D3D12
{
    ID3D12DescriptorHeap* pDescHeap;           // Pointer to the descriptor heap
    GFSDK_SSAO_UINT       BaseIndex;           // The base index that can be used as a start index of the pDescHeap
    GFSDK_SSAO_UINT       NumDescriptors;      // The maximum number of descriptors that are allowed to be used in the library

    GFSDK_SSAO_DescriptorHeapRange_D3D12()
        : pDescHeap(NULL)
        , BaseIndex(0)
        , NumDescriptors(0)
    {
    }
};

struct GFSDK_SSAO_DescriptorHeaps_D3D12
{
    GFSDK_SSAO_DescriptorHeapRange_D3D12 CBV_SRV_UAV;
    GFSDK_SSAO_DescriptorHeapRange_D3D12 RTV;
};

//---------------------------------------------------------------------------------------------------
// Creates a GFSDK_SSAO_Context associated with the D3D12 device.
//
// Remarks:
//    * Allocates D3D12 resources internally.
//    * Allocates memory using the default "::operator new", or "pCustomHeap->new_" if provided.
//
// Returns:
//     GFSDK_SSAO_NULL_ARGUMENT                        - One of the required argument pointers is NULL
//     GFSDK_SSAO_VERSION_MISMATCH                     - Invalid HeaderVersion (have you set HeaderVersion = GFSDK_SSAO_Version()?)
//     GFSDK_SSAO_MEMORY_ALLOCATION_FAILED             - Failed to allocate memory on the heap
//     GFSDK_SSAO_D3D_FEATURE_LEVEL_NOT_SUPPORTED      - The D3D feature level of pD3DDevice is lower than 11_0
//     GFSDK_SSAO_D3D_RESOURCE_CREATION_FAILED         - A resource-creation call has failed (running out of memory?)
//     GFSDK_SSAO_D3D12_INVALID_HEAP_TYPE              - One of the heaps provided to GFSDK_SSAO_CreateContext_D3D12 has an unexpected type
//     GFSDK_SSAO_D3D12_INSUFFICIENT_DESCRIPTORS       - One of the heaps described in pHeapInfo has insufficient descriptors
//     GFSDK_SSAO_D3D12_INVALID_NODE_MASK              - NodeMask has more than one bit set, or is zero
//     GFSDK_SSAO_OK                                   - Success
//---------------------------------------------------------------------------------------------------
GFSDK_SSAO_DECL(GFSDK_SSAO_Status, GFSDK_SSAO_CreateContext_D3D12,
    ID3D12Device* pD3DDevice,
    GFSDK_SSAO_UINT NodeMask,
    const GFSDK_SSAO_DescriptorHeaps_D3D12& DescriptorHeaps,
    GFSDK_SSAO_Context_D3D12** ppContext,
    GFSDK_SSAO_CUSTOM_HEAP_ARGUMENT,
    GFSDK_SSAO_VERSION_ARGUMENT);

#pragma pack(pop)
