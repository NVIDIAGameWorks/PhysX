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

#ifndef __PPCGEKKO__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector>

#include "wavefront.h"

#ifdef _WIN32
#	define strcasecmp _stricmp 
#endif

typedef std::vector< int > IntVector;
typedef std::vector< float > FloatVector;

#pragma warning(disable:4996)

namespace WAVEFRONT
{

/*******************************************************************/
/******************** InParser.h  ********************************/
/*******************************************************************/
class InPlaceParserInterface
{
public:
	virtual int ParseLine(int lineno,int argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
};

enum SeparatorType
{
	ST_DATA,        // is data
	ST_HARD,        // is a hard separator
	ST_SOFT,        // is a soft separator
	ST_EOS          // is a comment symbol, and everything past this character should be ignored
};

class InPlaceParser
{
public:
	InPlaceParser(void)
	{
		Init();
	}

	InPlaceParser(char *data,int len)
	{
		Init();
		SetSourceData(data,len);
	}

	InPlaceParser(const char *fname)
	{
		Init();
		SetFile(fname);
	}

	~InPlaceParser(void);

	void Init(void)
	{
		mQuoteChar = 34;
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;
		for (int i=0; i<256; i++)
		{
			mHard[i] = ST_DATA;
			mHardString[i*2] = (char)i;
			mHardString[i*2+1] = 0;
		}
		mHard[0]  = ST_EOS;
		mHard[32] = ST_SOFT;
		mHard[9]  = ST_SOFT;
		mHard[13] = ST_SOFT;
		mHard[10] = ST_SOFT;
	}

	void SetFile(const char *fname); // use this file as source data to parse.

	void SetSourceData(char *data,int len)
	{
		mData = data;
		mLen  = len;
		mMyAlloc = false;
	};

	int  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

	int ProcessLine(int lineno,char *line,InPlaceParserInterface *callback);

	const char ** GetArglist(char *source,int &count); // convert source string into an arg list, this is a destructive parse.

	void SetHardSeparator(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}

	void SetHard(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}


	void SetCommentSymbol(char c) // comment character, treated as 'end of string'
	{
		mHard[c] = ST_EOS;
	}

	void ClearHardSeparator(char c)
	{
		mHard[c] = ST_DATA;
	}


	void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

	bool EOS(char c)
	{
		if ( mHard[c] == ST_EOS )
		{
			return true;
		}
		return false;
	}

	void SetQuoteChar(char c)
	{
		mQuoteChar = c;
	}

private:


	inline char * AddHard(int &argc,const char **argv,char *foo);
	inline bool   IsHard(char c);
	inline char * SkipSpaces(char *foo);
	inline bool   IsWhiteSpace(char c);
	inline bool   IsNonSeparator(char c); // non seperator,neither hard nor soft

	bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
	char  *mData;  // ascii data to parse.
	int    mLen;   // length of data
	SeparatorType  mHard[256];
	char   mHardString[256*2];
	char           mQuoteChar;
};

/*******************************************************************/
/******************** InParser.cpp  ********************************/
/*******************************************************************/
void InPlaceParser::SetFile(const char *fname)
{
	if ( mMyAlloc )
	{
		free(mData);
	}
	mData = 0;
	mLen  = 0;
	mMyAlloc = false;

	FILE *fph = fopen(fname,"rb");
	if ( fph )
	{
		fseek(fph,0L,SEEK_END);
		mLen = ftell(fph);
		fseek(fph,0L,SEEK_SET);
		if ( mLen )
		{
			mData = (char *) malloc(sizeof(char)*(mLen+1));
			size_t ok = fread(mData, mLen, 1, fph);
			if ( !ok )
			{
				free(mData);
				mData = 0;
			}
			else
			{
				mData[mLen] = 0; // zero byte terminate end of file marker.
				mMyAlloc = true;
			}
		}
		fclose(fph);
	}

}

InPlaceParser::~InPlaceParser(void)
{
	if ( mMyAlloc )
	{
		free(mData);
	}
}

#define MAXARGS 512

bool InPlaceParser::IsHard(char c)
{
	return mHard[c] == ST_HARD;
}

char * InPlaceParser::AddHard(int &argc,const char **argv,char *foo)
{
	while ( IsHard(*foo) )
	{
		const char *hard = &mHardString[*foo*2];
		if ( argc < MAXARGS )
		{
			argv[argc++] = hard;
		}
		foo++;
	}
	return foo;
}

bool   InPlaceParser::IsWhiteSpace(char c)
{
	return mHard[c] == ST_SOFT;
}

char * InPlaceParser::SkipSpaces(char *foo)
{
	while ( !EOS(*foo) && IsWhiteSpace(*foo) ) foo++;
	return foo;
}

bool InPlaceParser::IsNonSeparator(char c)
{
	if ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 ) return true;
	return false;
}


int InPlaceParser::ProcessLine(int lineno,char *line,InPlaceParserInterface *callback)
{
	int ret = 0;

	const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{

		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) ) break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			foo++;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				foo++;
			}
		}
		else
		{

			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					foo++;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar ) foo++;
					if ( *foo ) *foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						foo++;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						foo++;
						break;
					}
					foo++;
				} // end of while loop...
			}
		}
	}

	if ( argc )
	{
		ret = callback->ParseLine(lineno, argc, argv );
	}

	return ret;
}

int  InPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
{
	assert( callback );
	if ( !mData ) return 0;

	int ret = 0;

	int lineno = 0;

	char *foo   = mData;
	char *begin = foo;


	while ( *foo )
	{
		if ( *foo == 10 || *foo == 13 )
		{
			lineno++;
			*foo = 0;

			if ( *begin ) // if there is any data to parse at all...
			{
				int v = ProcessLine(lineno,begin,callback);
				if ( v ) ret = v;
			}

			foo++;
			if ( *foo == 10 ) foo++; // skip line feed, if it is in the carraige-return line-feed format...
			begin = foo;
		}
		else
		{
			foo++;
		}
	}

	lineno++; // lasst line.

	int v = ProcessLine(lineno,begin,callback);
	if ( v ) ret = v;
	return ret;
}


void InPlaceParser::DefaultSymbols(void)
{
	SetHardSeparator(',');
	SetHardSeparator('(');
	SetHardSeparator(')');
	SetHardSeparator('=');
	SetHardSeparator('[');
	SetHardSeparator(']');
	SetHardSeparator('{');
	SetHardSeparator('}');
	SetCommentSymbol('#');
}


const char ** InPlaceParser::GetArglist(char *line,int &count) // convert source string into an arg list, this is a destructive parse.
{
	const char **ret = 0;

	static const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{

		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) ) break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			foo++;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				foo++;
			}
		}
		else
		{

			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					foo++;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar ) foo++;
					if ( *foo ) *foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						foo++;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						foo++;
						break;
					}
					foo++;
				} // end of while loop...
			}
		}
	}

	count = argc;
	if ( argc )
	{
		ret = argv;
	}

	return ret;
}

/*******************************************************************/
/******************** Geometry.h  ********************************/
/*******************************************************************/

class GeometryVertex
{
public:
	float        mPos[3];
	float        mNormal[3];
	float        mTexel[2];
};


class GeometryInterface
{
public:

	virtual void NodeTriangle(const GeometryVertex * /*v1*/,const GeometryVertex * /*v2*/,const GeometryVertex * /*v3*/, bool /*textured*/)
	{
	}

};


/*******************************************************************/
/******************** Obj.h  ********************************/
/*******************************************************************/


class OBJ : public InPlaceParserInterface
{
public:
  int LoadMesh(const char *fname,GeometryInterface *callback, bool textured);
  int ParseLine(int lineno,int argc,const char **argv);  // return TRUE to continue parsing, return FALSE to abort parsing process
private:

  void GetVertex(GeometryVertex &v,const char *face) const;

  FloatVector     mVerts;
  FloatVector     mTexels;
  FloatVector     mNormals;

  bool            mTextured;

  GeometryInterface *mCallback;
};


/*******************************************************************/
/******************** Obj.cpp  ********************************/
/*******************************************************************/

int OBJ::LoadMesh(const char *fname,GeometryInterface *iface, bool textured)
{
  mTextured = textured;
  int ret = 0;

  mVerts.clear();
  mTexels.clear();
  mNormals.clear();

  mCallback = iface;

  InPlaceParser ipp(fname);

  ipp.Parse(this);

return ret;
}

/***
static const char * GetArg(const char **argv,int i,int argc)
{
  const char * ret = 0;
  if ( i < argc ) ret = argv[i];
  return ret;
}
****/

void OBJ::GetVertex(GeometryVertex &v,const char *face) const
{
  v.mPos[0] = 0;
  v.mPos[1] = 0;
  v.mPos[2] = 0;

  v.mTexel[0] = 0;
  v.mTexel[1] = 0;

  v.mNormal[0] = 0;
  v.mNormal[1] = 1;
  v.mNormal[2] = 0;

  int index = atoi( face )-1;

  const char *texel = strstr(face,"/");

  if ( texel )
  {
    int tindex = atoi( texel+1) - 1;

    if ( tindex >=0 && tindex < (int)(mTexels.size()/2) )
    {
    	const float *t = &mTexels[tindex*2];

      v.mTexel[0] = t[0];
      v.mTexel[1] = t[1];

    }

    const char *normal = strstr(texel+1,"/");
    if ( normal )
    {
      int nindex = atoi( normal+1 ) - 1;

      if (nindex >= 0 && nindex < (int)(mNormals.size()/3) )
      {
      	const float *n = &mNormals[nindex*3];

        v.mNormal[0] = n[0];
        v.mNormal[1] = n[1];
        v.mNormal[2] = n[2];
      }
    }
  }

  if ( index >= 0 && index < (int)(mVerts.size()/3) )
  {

    const float *p = &mVerts[index*3];

    v.mPos[0] = p[0];
    v.mPos[1] = p[1];
    v.mPos[2] = p[2];
  }

}

int OBJ::ParseLine(int /*lineno*/,int argc,const char **argv)  // return TRUE to continue parsing, return FALSE to abort parsing process
{
  int ret = 0;

  if ( argc >= 1 )
  {
    const char *foo = argv[0];
    if ( *foo != '#' )
    {
      if ( strcasecmp(argv[0],"v") == 0 && argc == 4 )
      {
        float vx = (float) atof( argv[1] );
        float vy = (float) atof( argv[2] );
        float vz = (float) atof( argv[3] );
        mVerts.push_back(vx);
        mVerts.push_back(vy);
        mVerts.push_back(vz);
      }
      else if ( strcasecmp(argv[0],"vt") == 0 && (argc == 3 || argc == 4))
      {
        // ignore 4rd component if present
        float tx = (float) atof( argv[1] );
        float ty = (float) atof( argv[2] );
        mTexels.push_back(tx);
        mTexels.push_back(ty);
      }
      else if ( strcasecmp(argv[0],"vn") == 0 && argc == 4 )
      {
        float normalx = (float) atof(argv[1]);
        float normaly = (float) atof(argv[2]);
        float normalz = (float) atof(argv[3]);
        mNormals.push_back(normalx);
        mNormals.push_back(normaly);
        mNormals.push_back(normalz);
      }
      else if ( strcasecmp(argv[0],"f") == 0 && argc >= 4 )
      {
        GeometryVertex v[32];

        int vcount = argc-1;

        for (int i=1; i<argc; i++)
        {
          GetVertex(v[i-1],argv[i] );
        }


		mCallback->NodeTriangle(&v[0],&v[1],&v[2], mTextured);

        if ( vcount >=3 ) // do the fan
        {
          for (int i=2; i<(vcount-1); i++)
          {
            mCallback->NodeTriangle(&v[0],&v[i],&v[i+1], mTextured);
          }
        }

      }
    }
  }

  return ret;
}




class BuildMesh : public GeometryInterface
{
public:

	int GetIndex(const float *p, const float *texCoord)
	{

		int vcount = (int)mVertices.size()/3;

		if(vcount>0)
		{
			//New MS STL library checks indices in debug build, so zero causes an assert if it is empty.
			const float *v = &mVertices[0];
			const float *t = texCoord != NULL ? &mTexCoords[0] : NULL;

			for (int i=0; i<vcount; i++)
			{
				if ( v[0] == p[0] && v[1] == p[1] && v[2] == p[2] )
				{
					if (texCoord == NULL || (t[0] == texCoord[0] && t[1] == texCoord[1]))
					{
						return i;
					}
				}
				v+=3;
				if (t != NULL)
					t += 2;
			}
		}

		mVertices.push_back( p[0] );
		mVertices.push_back( p[1] );
		mVertices.push_back( p[2] );

		if (texCoord != NULL)
		{
			mTexCoords.push_back( texCoord[0] );
			mTexCoords.push_back( texCoord[1] );
		}

		return vcount;
	}

	virtual void NodeTriangle(const GeometryVertex *v1,const GeometryVertex *v2,const GeometryVertex *v3, bool textured)
	{
		mIndices.push_back( GetIndex(v1->mPos, textured ? v1->mTexel : NULL) );
		mIndices.push_back( GetIndex(v2->mPos, textured ? v2->mTexel : NULL) );
		mIndices.push_back( GetIndex(v3->mPos, textured ? v3->mTexel : NULL) );
	}

  const FloatVector& GetVertices(void) const { return mVertices; };
  const FloatVector& GetTexCoords(void) const { return mTexCoords; };
  const IntVector& GetIndices(void) const { return mIndices; };

private:
  FloatVector     mVertices;
  FloatVector     mTexCoords;
  IntVector       mIndices;
};

};

using namespace WAVEFRONT;

WavefrontObj::WavefrontObj(void)
{
	mVertexCount = 0;
	mTriCount    = 0;
	mIndices     = 0;
	mVertices    = NULL;
	mTexCoords   = NULL;
}

WavefrontObj::~WavefrontObj(void)
{
	delete mIndices;
	delete mVertices;
}

unsigned int WavefrontObj::loadObj(const char *fname, bool textured) // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
{

	unsigned int ret = 0;

	delete mVertices;
	mVertices = 0;
	delete mIndices;
	mIndices = 0;
	mVertexCount = 0;
	mTriCount = 0;


  BuildMesh bm;

  OBJ obj;

  obj.LoadMesh(fname,&bm, textured);


	const FloatVector &vlist = bm.GetVertices();
	const IntVector &indices = bm.GetIndices();
	if ( vlist.size() )
	{
		mVertexCount = (int)vlist.size()/3;
		mVertices = new float[mVertexCount*3];
		memcpy( mVertices, &vlist[0], sizeof(float)*mVertexCount*3 );

		if (textured)
		{
			mTexCoords = new float[mVertexCount * 2];
			const FloatVector& tList = bm.GetTexCoords();
			memcpy( mTexCoords, &tList[0], sizeof(float) * mVertexCount * 2);
		}

		mTriCount = (int)indices.size()/3;
		mIndices = new int[mTriCount*3*sizeof(int)];
		memcpy(mIndices, &indices[0], sizeof(int)*mTriCount*3);
		ret = mTriCount;
	}


	return ret;
}


bool WavefrontObj::saveObj(const char *fname,int vcount,const float *vertices,int tcount,const int *indices)
{
  bool ret = false;

  FILE *fph = fopen(fname,"wb");
  if ( fph )
  {
    for (int i=0; i<vcount; i++)
    {
      fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", vertices[0], vertices[1], vertices[2] );
      vertices+=3;
    }
    for (int i=0; i<tcount; i++)
    {
      fprintf(fph,"f %d %d %d\r\n", indices[0]+1, indices[1]+1, indices[2]+1 );
      indices+=3;
    }
    fclose(fph);
    ret = true;
  }
  return ret;
}

#endif
