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

#ifndef PHYSX_NO_RENDERER

#include "GLFontData.h"
#include "GLFontRenderer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <GL/gl.h>

using namespace physx;

bool GLFontRenderer::init()
{
	m_isInit=false;
	m_textureObject=0;
	m_screenWidth=640;
	m_screenHeight=480;
	m_color = 0xffffffff;
	
	glGenTextures(1, &m_textureObject);
	if(m_textureObject == 0)
		return false;

	glBindTexture(GL_TEXTURE_2D, m_textureObject);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// expand to rgba
	unsigned char* pNewSource = new unsigned char[OGL_FONT_TEXTURE_WIDTH*OGL_FONT_TEXTURE_HEIGHT*4];
	for(int i=0;i<OGL_FONT_TEXTURE_WIDTH*OGL_FONT_TEXTURE_HEIGHT;i++)
	{
		pNewSource[i*4+0]=255;
		pNewSource[i*4+1]=255;
		pNewSource[i*4+2]=255;
		pNewSource[i*4+3]=OGLFontData[i];
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, OGL_FONT_TEXTURE_WIDTH, OGL_FONT_TEXTURE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pNewSource);
	glBindTexture(GL_TEXTURE_2D, 0);

//	delete[] pNewSource;

	m_isInit=true;
	return true;
}

inline unsigned int safeStrlen( const char* pString )
{
	return ( pString && *pString) ? (unsigned int)strlen(pString) : 0;
}

template<typename TOperator>
void GLFontRenderer::print( float fontSize, const char* pString, bool forceMonoSpace, int monoSpaceWidth, bool doOrthoProj, TOperator inOperator )
{
	if ( !m_isInit )
		init();
	unsigned int num = safeStrlen( pString );
	if ( !m_isInit || !num )
		return;
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_textureObject);
	
	if(doOrthoProj)
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glOrtho(0, m_screenWidth, 0, m_screenHeight, -1, 1);
	}
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnable(GL_BLEND);

	glColor4f((m_color&0xff) / float(0xff), ((m_color>>8)&0xff) / float(0xff), 
		((m_color>>16)&0xff) / float(0xff), ((m_color>>24)&0xff) / float(0xff));

	mVertList.resize( PxMax( num * 6 * 3, mVertList.size() ) );
	mTextureCoordList.resize( PxMax( num * 6 * 2, mTextureCoordList.size() ) );
	PxF32* pVertList = reinterpret_cast<PxF32*>( mVertList.begin() );
	PxF32* pTextureCoordList = reinterpret_cast<PxF32*>( mTextureCoordList.begin() );

	int vertIndex = 0;
	int textureCoordIndex = 0;

	float translateDown = 0.0f;
	float translate = 0.0f;
	unsigned int count = 0;
	
	const float glyphHeightUV = ((float)OGL_FONT_CHARS_PER_COL)/OGL_FONT_TEXTURE_HEIGHT*2-0.01f;
	const float glyphWidthUV = ((float)OGL_FONT_CHARS_PER_ROW)/OGL_FONT_TEXTURE_WIDTH;
	
	for(unsigned int i=0;i<num; i++)
	{
		if (pString[i] == '\n') {
			translateDown-=0.005f*m_screenHeight+fontSize;
			translate = 0.0f;
			continue;
		}

		int c = pString[i]-OGL_FONT_CHAR_BASE;
		if (c < OGL_FONT_CHARS_PER_ROW*OGL_FONT_CHARS_PER_COL) {

			count++;

			float glyphWidth = (float)GLFontGlyphWidth[c];
			if(forceMonoSpace){
				glyphWidth = (float)monoSpaceWidth;
			}
			
			glyphWidth = glyphWidth*(fontSize/(((float)OGL_FONT_TEXTURE_WIDTH)/OGL_FONT_CHARS_PER_ROW));

			float cxUV = float((c)%OGL_FONT_CHARS_PER_ROW)/OGL_FONT_CHARS_PER_ROW+0.008f;
			float cyUV = float((c)/OGL_FONT_CHARS_PER_ROW)/OGL_FONT_CHARS_PER_COL+0.008f;
			inOperator( cxUV, cyUV, glyphWidthUV, glyphHeightUV
						, pVertList, pTextureCoordList, vertIndex, textureCoordIndex
						, translate, translateDown, glyphWidth );
			vertIndex += 6*3;
			textureCoordIndex += 6*2;
			
			translate+=1.0f * glyphWidth;
		}
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, pVertList);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, pTextureCoordList);
	glDrawArrays(GL_TRIANGLES, 0, count*6);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	if(doOrthoProj)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);	
	glDisable(GL_BLEND);
}

struct TextPrintRender
{
	float x;
	float y;
	float fontSize;
	int screenHeight;

	TextPrintRender( float _x, float _y, float _fontSize, int _screenWidth, int _screenHeight )
		: x( _x*_screenWidth )
		, y( _y*_screenHeight )
		, fontSize( _fontSize * _screenHeight )
		, screenHeight( _screenHeight )
	{
	}
	void operator()( float cxUV, float cyUV, float glyphWidthUV, float glyphHeightUV
					, float* pVertList, float* pTextureCoordList, int vertIndex, int textureCoordIndex
					, float translate, float translateDown, float glyphWidth )
	{ 
		translate *= screenHeight; //move translate to pixel space
		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;

		pVertList[vertIndex++] = x+0+translate;
		pVertList[vertIndex++] = y+0+translateDown;
		pVertList[vertIndex++] = 0;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;

		pVertList[vertIndex++] = x+fontSize+translate;
		pVertList[vertIndex++] = y+fontSize+translateDown;
		pVertList[vertIndex++] = 0;

		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;

		pVertList[vertIndex++] = x+0+translate;
		pVertList[vertIndex++] = y+fontSize+translateDown;
		pVertList[vertIndex++] = 0;

		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;

		pVertList[vertIndex++] = x+0+translate;
		pVertList[vertIndex++] = y+0+translateDown;
		pVertList[vertIndex++] = 0;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;
		
		pVertList[vertIndex++] = x+fontSize+translate;
		pVertList[vertIndex++] = y+0+translateDown;
		pVertList[vertIndex++] = 0;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;
		
		pVertList[vertIndex++] = x+fontSize+translate;
		pVertList[vertIndex++] = y+fontSize+translateDown;
		pVertList[vertIndex++] = 0;
	}
};


void GLFontRenderer::print(float x, float y, float fontSize, const char* pString, bool forceMonoSpace, int monoSpaceWidth, bool doOrthoProj)
{
	print( fontSize, pString, forceMonoSpace, monoSpaceWidth, doOrthoProj, TextPrintRender( x, y, fontSize, m_screenWidth, m_screenHeight ) );
}

GLFontMeasureResult GLFontRenderer::measure( float fontSize, const char* pString, bool forceMonoSpace, int monoSpaceWidth )
{
	GLFontMeasureResult retval( 0, fontSize );
	if ( !m_isInit )
		init();
	unsigned int num( safeStrlen( pString ) );
	
	for(unsigned int i=0;i<num; i++)
	{
		int c = pString[i]-OGL_FONT_CHAR_BASE;
		if (c < OGL_FONT_CHARS_PER_ROW*OGL_FONT_CHARS_PER_COL) {
			float glyphWidth = (float)GLFontGlyphWidth[c];
			if(forceMonoSpace){
				glyphWidth = (float)monoSpaceWidth;
			}
			
			glyphWidth = glyphWidth*(fontSize/(((float)OGL_FONT_TEXTURE_WIDTH)/OGL_FONT_CHARS_PER_ROW));
			retval.width += glyphWidth;
		}
	}
	return retval;
}

struct TextPrint3DRender
{
	PxVec3 pos;
	PxVec3 cameraDir;
	PxVec3 up;
	PxVec3 right;
	float fontSize;
	TextPrint3DRender( const PxVec3& _pos, const PxVec3& _cameraDir, const PxVec3& _up, float _fontSize )
		: pos( _pos )
		, cameraDir( _cameraDir )
		, up( _up )
		, fontSize( _fontSize )
	{
	}
	void operator()( float cxUV, float cyUV, float glyphWidthUV, float glyphHeightUV
					, float* pVertList, float* pTextureCoordList, int vertIndex, int textureCoordIndex
					, float translate, float translateDown, float glyphWidth )
	{
		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;

		PxVec3 v;
		v = pos + right * translate - up * (translateDown + fontSize);
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;

		v = pos + right * (glyphWidth + translate) - up * translateDown;
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;

		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;

		v = pos + right * translate - up * translateDown;
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;

		pTextureCoordList[textureCoordIndex++] = cxUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;

		v = pos + right * translate - up * (fontSize + translateDown);
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV+glyphHeightUV;
		
		v = pos + right * (glyphWidth + translate) - up * (fontSize + translateDown);
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;

		pTextureCoordList[textureCoordIndex++] = cxUV+glyphWidthUV;
		pTextureCoordList[textureCoordIndex++] = cyUV;
		
		v = pos + right * (glyphWidth + translate) - up * translateDown;
		pVertList[vertIndex++] = v.x;
		pVertList[vertIndex++] = v.y;
		pVertList[vertIndex++] = v.z;
	}
};

void GLFontRenderer::print3d(const PxVec3& pos, const PxVec3& cameraDir, const PxVec3& up, float fontSize, const char* pString, bool forceMonoSpace, int monoSpaceWidth)
{
	print( fontSize, pString, forceMonoSpace, monoSpaceWidth, false, TextPrint3DRender( pos, cameraDir, up, fontSize ) );
}


void GLFontRenderer::setScreenResolution(int screenWidth, int screenHeight)
{
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;
}

void GLFontRenderer::setColor(unsigned int abgr)
{
	m_color = abgr;
}

#endif //!PHYSX_NO_RENDERER