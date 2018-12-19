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

#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include "BmpFile.h"
#include <iostream>

using namespace std;
// standard cubemap face names
/*
const char *faceName[6] = {
    "negz",
    "negz",
    "posy",
    "negy",
    "negz",
    "negz",
};
*/
const char *faceName0[6] = {
    "posx",
    "negx",
    "posy",
    "negy",
    "posz",
    "negz",
};
const char *faceName1[6] = {
    "ft",
    "bk",
    "up",
    "dn",
    "rt",
    "lf",
};


void loadImg(GLenum target, const char *filename, char mode = 0, float sr = 0.0f, float sg = 0.0f, float sb = 0.0f, float startGrad = -1.0f, float endGrad = -1.0f, char dir = 1)
{
	int len = strlen(filename);

	unsigned char* ptr = 0, *ptrBegin;
	int w, h;

	BmpLoaderBuffer loader;
	
	if (strcmp(&filename[len-4], ".bmp") == 0)
	{
		// bmp
		if (!loader.loadFile(filename)) {
			fprintf(stderr, "Error loading bmp '%s'\n");
			return;  
		}
		ptrBegin = ptr = loader.mRGB;
		h = loader.mHeight;
		w = loader.mWidth;

	}
	else
	{
		fprintf(stderr, "Error loading '%s' jpg, tga and and other formats not supported\n");
		return;
	}
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	if (mode == 0) {
		// Nothing
	}
	float rf = sr*255.0f;
	float gf = sg*255.0f;
	float bf = sb*255.0f;
	unsigned char r = (unsigned char)(rf+0.5f);
	unsigned char g = (unsigned char)(gf+0.5f);
	unsigned char b = (unsigned char)(bf+0.5f);

	
	if (mode == 1) {
		// Fill all with sky color
		//printf("Mode is 1\n");
		
		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				*(ptr++) = r;
				*(ptr++) = g;
				*(ptr++) = b;
			}
		}
	}
	if (mode == 2) {
		// Fill with gradient
		int sh = startGrad*h + 0.5f;
		int eh = endGrad*h + 0.5f;
		if (dir == 1) {
			for (int i = 0; i < sh; i++) {
				for (int j = 0; j < w; j++) {
					*(ptr++) = r;
					*(ptr++) = g;
					*(ptr++) = b;
				}
			}
			float grad = 1.0f/(eh - sh);
			float frac = 0.0f;
			for (int i = sh; i < eh; i++) {
				for (int j = 0; j < w; j++) {
					ptr[0] = (unsigned char)(0.5f + ptr[0]*frac + (1.0f-frac)*rf);
					ptr[1] = (unsigned char)(0.5f + ptr[1]*frac + (1.0f-frac)*gf);
					ptr[2] = (unsigned char)(0.5f + ptr[2]*frac + (1.0f-frac)*bf);
					ptr+=3;
				}
				frac+=grad;
			}
		} else {
			ptr+=(w*3*sh);
			float grad = 1.0f/(eh - sh);
			float frac = 1.0f;
			for (int i = sh; i < eh; i++) {
				for (int j = 0; j < w; j++) {
					ptr[0] = (unsigned char)(0.5f + ptr[0]*frac + (1.0f-frac)*rf);
					ptr[1] = (unsigned char)(0.5f + ptr[1]*frac + (1.0f-frac)*gf);
					ptr[2] = (unsigned char)(0.5f + ptr[2]*frac + (1.0f-frac)*bf);
					ptr+=3;
				}
				frac-=grad;
			}
			for (int i = eh; i < h; i++) {
				for (int j = 0; j < w; j++) {
					*(ptr++) = r;
					*(ptr++) = g;
					*(ptr++) = b;
				}
			}
		}
	}
	/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
    glTexImage2D(target, 0, GL_RGBA8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, ptrBegin);
	
	/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	//glGenerateMipmapEXT(target);
	/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/

}

GLuint loadImgTexture(const char *filename)
{
    GLuint texID;
    glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    loadImg(GL_TEXTURE_2D, filename);

    return texID;
}


// specify filename in printf format, e.g. "cubemap_%s.png"
GLuint loadCubeMap(const char *string, char mode, char nameset, float sr, float sg, float sb, float startGrad, float endGrad, char dir)
{
    GLuint texID;
    glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texID);
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
    // load faces
	if (mode <= 1) {
		for(int i=0; i<6; i++) {
			char buff[1024];
			if (nameset == 0) {
				sprintf(buff, string, faceName0[i]);
			} else {
				sprintf(buff, string, faceName1[i]);
			}
			loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, mode, sr, sg, sb, startGrad, endGrad, dir);
			

		}
	} /*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	if (mode == 2) {
		for(int i=0; i<6; i++) {
			char buff[1024];
			if (nameset == 0) {
				sprintf(buff, string, faceName0[i]);
			} else {
				sprintf(buff, string, faceName1[i]);
			}
			if (i == 2) {
				if (dir == -1) {
					loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, 0);
				} else {
					loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, 1, sr, sg, sb);
				}
			} else
			if (i == 3) {
				if (dir == -1) {
					loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, 1, sr, sg, sb);
				} else {
					loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, 0);
				}
			} else {
				loadImg(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, buff, mode, sr, sg, sb, startGrad, endGrad, dir);
			}
		}
	}
/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
	glGenerateMipmapEXT(GL_TEXTURE_CUBE_MAP);
	/*{GLenum er = glGetError(); if (er != GL_NO_ERROR) {	 cout<<"Var name: "<<name<<" OGL Error "<<er<<endl; }}*/
    return texID;
}


GLuint createTexture(GLenum target, GLint internalformat, int w, int h, GLenum type, GLenum format, void *data)
{
    GLuint texid;
    glGenTextures(1, &texid);
    glBindTexture(target, texid);

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(target, 0, internalformat, w, h, 0, format, type, data);

    return texid;
}
