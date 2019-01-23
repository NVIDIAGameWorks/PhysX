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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "wavefront.h"
#include "FrameworkFoundation.h"
#include "PxTkFile.h"

namespace WAVEFRONT
{

	/*******************************************************************/
	/******************** InParser.h  ********************************/
	/*******************************************************************/
	class InPlaceParserInterface
	{
	public:
		virtual int ParseLine(int lineno,int argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
		virtual ~InPlaceParserInterface() {}
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
				mHardString[i*2] = i;
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
		}

		int  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

		int ProcessLine(int lineno,char *line,InPlaceParserInterface *callback);

		const char ** GetArglist(char *source,int &count); // convert source string into an arg list, this is a destructive parse.

		void SetHardSeparator(char c) // add a hard separator
		{
			mHard[(unsigned int)c] = ST_HARD;
		}

		void SetHard(char c) // add a hard separator
		{
			mHard[(unsigned int)c] = ST_HARD;
		}


		void SetCommentSymbol(char c) // comment character, treated as 'end of string'
		{
			mHard[(unsigned int)c] = ST_EOS;
		}

		void ClearHardSeparator(char c)
		{
			mHard[(unsigned int)c] = ST_DATA;
		}


		void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

		bool EOS(char c)
		{
			if ( mHard[(unsigned int)c] == ST_EOS )
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
	void InPlaceParser::SetFile(const char* fname)
	{
		if ( mMyAlloc )
			free(mData);

		mData = 0;
		mLen  = 0;
		mMyAlloc = false;

		SampleFramework::File* fph = NULL;
		PxToolkit::fopen_s(&fph, fname, "rb");
		if ( fph )
		{
			fseek(fph,0L,SEEK_END);
			mLen = (int)ftell(fph);
			fseek(fph,0L,SEEK_SET);
			if ( mLen )
			{
				mData = (char *) malloc(sizeof(char)*(mLen+1));
				int ok = int(fread(mData, 1, mLen, fph));
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
			free(mData);
	}

#define MAXARGS 512

	bool InPlaceParser::IsHard(char c)
	{
		return mHard[(unsigned int)c] == ST_HARD;
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
		return mHard[(unsigned int)c] == ST_SOFT;
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

		lineno++; // last line.

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

		virtual void NodeTriangle(const GeometryVertex *v1,const GeometryVertex *v2,const GeometryVertex *v3, bool textured)
		{
		}

		virtual ~GeometryInterface() {}

	};


	/*******************************************************************/
	/******************** Obj.h  ********************************/
	/*******************************************************************/


	class OBJ : public InPlaceParserInterface
	{
	public:
		int LoadMesh(const char *fname,GeometryInterface *callback, bool textured);
		int ParseLine(int lineno,int argc,const char **argv);  // return TRUE to continue parsing, return FALSE to abort parsing process
	
		OBJ() : mVerts(NULL), mTexels(NULL), mNormals(NULL), mTextured(false), mCallback(NULL) { Clear(); }
		~OBJ() { Clear(); }

	private:

		void Clear();
		void GetVertex(GeometryVertex &v,const char *face) const;

		float* mVerts;
		unsigned mNumVerts; // this is the tripled number of verts
		unsigned mMaxVerts;

		float* mTexels; // doubled number of texcoords
		unsigned mNumTexels;
		unsigned mMaxTexels;

		float* mNormals; // tripled number of normals
		unsigned mNumNormals;
		unsigned mMaxNormals;

		bool mTextured;
		GeometryInterface* mCallback;
	};


	/*******************************************************************/
	/******************** Obj.cpp  ********************************/
	/*******************************************************************/

	int OBJ::LoadMesh(const char *fname, GeometryInterface *iface, bool textured)
	{
		Clear();
		
		mTextured = textured;
		int ret = 0;	
		mCallback = iface;

		InPlaceParser ipp(fname);

		ipp.Parse(this);

		return ret;
	}

	void OBJ::Clear()
	{
		if (mVerts)
			delete[] mVerts;
		mMaxVerts = 0;
		mNumVerts = 0;

		if (mTexels)
			delete[] mTexels;
		mMaxTexels = 0;
		mNumTexels = 0;

		if (mNormals)
			delete[] mNormals;
		mMaxNormals = 0;
		mNumNormals = 0;
	}

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

			if ( tindex >=0 && tindex < (int)(mNumTexels/2) )
			{
				const float *t = &mTexels[tindex*2];

				v.mTexel[0] = t[0];
				v.mTexel[1] = t[1];

			}

			const char *normal = strstr(texel+1,"/");
			if ( normal )
			{
				int nindex = atoi( normal+1 ) - 1;

				if (nindex >= 0 && nindex < (int)(mNumNormals/3) )
				{
					const float *n = &mNormals[nindex*3];

					v.mNormal[0] = n[0];
					v.mNormal[1] = n[1];
					v.mNormal[2] = n[2];
				}
			}
		}

		if ( index >= 0 && index < (int)(mNumVerts/3) )
		{

			const float *p = &mVerts[index*3];

			v.mPos[0] = p[0];
			v.mPos[1] = p[1];
			v.mPos[2] = p[2];
		} else
			assert(0); // "Negative face vertex indices are not supported in wavefront loader.");
	}

	template<typename T>
	void Resize(T*& data, unsigned& capacity, unsigned count, unsigned new_count)
	{
		if (new_count >= capacity)
		{
			capacity = new_count*2;
			T* tmp = new T[capacity];
			memcpy(tmp, data, count*sizeof(T));
			delete[] data;
			data = tmp;
		}
	}

	int OBJ::ParseLine(int lineno,int argc,const char **argv)  // return TRUE to continue parsing, return FALSE to abort parsing process
	{
		int ret = 0;

		if ( argc >= 1 )
		{
			const char *foo = argv[0];
			if ( *foo != '#' )
			{
				if (strcmp(argv[0], "v") == 0 && argc == 4 )
				{
					float vx = (float) atof( argv[1] );
					float vy = (float) atof( argv[2] );
					float vz = (float) atof( argv[3] );
					Resize(mVerts, mMaxVerts, mNumVerts, mNumVerts + 3);
					mVerts[mNumVerts++] = vx;
					mVerts[mNumVerts++] = vy;
					mVerts[mNumVerts++] = vz;
				}
				else if (strcmp(argv[0],"vt") == 0 && (argc == 3 || argc == 4))
				{
					// ignore 4rd component if present
					float tx = (float) atof( argv[1] );
					float ty = (float) atof( argv[2] );
					Resize(mTexels, mMaxTexels, mNumTexels, mNumTexels + 2);
					mTexels[mNumTexels++] = tx;
					mTexels[mNumTexels++] = ty;
				}
				else if (strcmp(argv[0],"vn") == 0 && argc == 4 )
				{
					float normalx = (float) atof(argv[1]);
					float normaly = (float) atof(argv[2]);
					float normalz = (float) atof(argv[3]);
					Resize(mNormals, mMaxNormals, mNumNormals, mNumNormals + 3);
					mNormals[mNumNormals++] = normalx;
					mNormals[mNumNormals++] = normaly;
					mNormals[mNumNormals++] = normalz;

				}
				else if (strcmp(argv[0],"f") == 0 && argc >= 4 )
				{
					GeometryVertex v[32];

					int vcount = argc-1;

					for (int i=1; i<argc; i++)
					{
						GetVertex(v[i-1],argv[i] );
					}

					// need to generate a normal!
#if 0 // not currently implemented
					if ( mNormals.empty() )
					{
						Vector3d<float> p1( v[0].mPos );
						Vector3d<float> p2( v[1].mPos );
						Vector3d<float> p3( v[2].mPos );

						Vector3d<float> n;
						n.ComputeNormal(p3,p2,p1);

						for (int i=0; i<vcount; i++)
						{
							v[i].mNormal[0] = n.x;
							v[i].mNormal[1] = n.y;
							v[i].mNormal[2] = n.z;
						}

					}
#endif

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
		BuildMesh() : mVertices(NULL), mTexCoords(NULL), mIndices(NULL)	{ Clear(); }
		~BuildMesh() { Clear(); }

		void Clear()
		{
			if (mVertices)
				delete[] mVertices;
			mMaxVertices = 0;
			mNumVertices = 0;

			if (mTexCoords)
				delete[] mTexCoords;
			mMaxTexCoords = 0;
			mNumTexCoords = 0;

			if (mIndices)
				delete[] mIndices;
			mMaxIndices = 0;
			mNumIndices = 0;
		}

		int GetIndex(const float *p, const float *texCoord)
		{

			int vcount = mNumVertices/3;

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

			Resize(mVertices, mMaxVertices, mNumVertices, mNumVertices + 3);
			mVertices[mNumVertices++] = p[0];
			mVertices[mNumVertices++] = p[1];
			mVertices[mNumVertices++] = p[2];

			if (texCoord != NULL)
			{
				Resize(mTexCoords, mMaxTexCoords, mNumTexCoords, mNumTexCoords + 2);
				mTexCoords[mNumTexCoords++] = texCoord[0];
				mTexCoords[mNumTexCoords++] = texCoord[1];
			}

			return vcount;
		}

		virtual void NodeTriangle(const GeometryVertex *v1,const GeometryVertex *v2,const GeometryVertex *v3, bool textured)
		{
			Resize(mIndices, mMaxIndices, mNumIndices, mNumIndices + 3);

			mIndices[mNumIndices++] = GetIndex(v1->mPos, textured ? v1->mTexel : NULL);
			mIndices[mNumIndices++] = GetIndex(v2->mPos, textured ? v2->mTexel : NULL);
			mIndices[mNumIndices++] = GetIndex(v3->mPos, textured ? v3->mTexel : NULL);
		}


		const float* GetVertices() const { return mVertices; }
		unsigned GetNumVertices() const { return mNumVertices; }

		const float* GetTexCoords(void) const { return mTexCoords; }
		unsigned GetNumTexCoords(void) const { return mNumTexCoords; }

		const int* GetIndices() const { return mIndices; }
		unsigned GetNumIndices() const {return mNumIndices; }


	private:
		float* mVertices;
		unsigned mMaxVertices;
		unsigned mNumVertices;

		float* mTexCoords;
		unsigned mMaxTexCoords;
		unsigned mNumTexCoords;

		int* mIndices;
		unsigned mMaxIndices;
		unsigned mNumIndices;
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
	delete []mIndices;
	delete []mVertices;
	if (mTexCoords)
		delete[] mTexCoords;
}

unsigned int WavefrontObj::loadObj(const char *fname, bool textured) // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
{
	unsigned int ret = 0;
	delete []mVertices;
	mVertices = 0;
	delete []mIndices;
	mIndices = 0;
	mVertexCount = 0;
	mTriCount = 0;

	BuildMesh bm;
	OBJ obj;
	obj.LoadMesh(fname, &bm, textured);

	if (bm.GetNumVertices())
	{
		mVertexCount = bm.GetNumVertices()/3;
		mVertices = new float[mVertexCount*3];
		memcpy(mVertices, bm.GetVertices(), sizeof(float)*mVertexCount*3);

		if (textured)
		{
			mTexCoords = new float[mVertexCount * 2];
			memcpy(mTexCoords, bm.GetTexCoords(), sizeof(float) * mVertexCount * 2);
		}

		mTriCount = bm.GetNumIndices()/3;
		mIndices = new int[mTriCount*3];
		memcpy(mIndices, bm.GetIndices(), sizeof(int)*mTriCount*3);
		ret = mTriCount;
	}

	return ret;
}



bool LoadWavefrontBinary(const char* filename, WavefrontObj& wfo)
{
	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, filename, "rb");
	if(!fp)	return false;

	size_t numRead = fread(&wfo.mVertexCount, 1, sizeof(int), fp);
	if(numRead != sizeof(int)) { fclose(fp); return false; }
	
	wfo.mVertices = new float[wfo.mVertexCount*3];
	numRead = fread(wfo.mVertices, 1, sizeof(float)*wfo.mVertexCount*3, fp);
	if(numRead != sizeof(float)*wfo.mVertexCount*3) { fclose(fp); return false; }

	numRead = fread(&wfo.mTriCount, 1, sizeof(int), fp);
	if(numRead != sizeof(int)) { fclose(fp); return false; }
	
	wfo.mIndices = new int[wfo.mTriCount*3];
	numRead = fread(wfo.mIndices, 1, sizeof(int)*wfo.mTriCount*3, fp);
	if(numRead != sizeof(int)*wfo.mTriCount*3) { fclose(fp); return false; }

	// NB: mTexCoords not supported

	fclose(fp);
	return true;
}

bool SaveWavefrontBinary(const char* filename, const WavefrontObj& wfo)
{
	SampleFramework::File* fp = NULL;
	PxToolkit::fopen_s(&fp, filename, "wb");
	if(!fp)	return false;

	fwrite(&wfo.mVertexCount, 1, sizeof(int), fp);
	fwrite(wfo.mVertices, 1, wfo.mVertexCount*3*sizeof(float), fp);

	fwrite(&wfo.mTriCount, 1, sizeof(int), fp);
	fwrite(wfo.mIndices, 1, wfo.mTriCount*3*sizeof(int), fp);

	// NB: mTexCoords not supported

	fclose(fp);
	return true;
}
