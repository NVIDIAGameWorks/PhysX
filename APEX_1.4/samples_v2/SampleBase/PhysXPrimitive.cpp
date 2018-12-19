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



#include "PhysXPrimitive.h"
#include <DirectXMath.h>
#include "ApexRenderMaterial.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											Base Mesh internal class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class Mesh
{
public:
	Mesh(const float v[], UINT numVertices)
	{
		ID3D11Device* device = GetDeviceManager()->GetDevice();

		ID3D11DeviceContext* context;
		device->GetImmediateContext(&context);

		mNumVertices = numVertices;

		D3D11_SUBRESOURCE_DATA vertexBufferData;

		ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
		vertexBufferData.pSysMem = v;

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(float) * 6 * mNumVertices;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

		V(device->CreateBuffer(&bufferDesc, &vertexBufferData, &mVertexBuffer));
	}

	~Mesh()
	{
		SAFE_RELEASE(mVertexBuffer);
	}

	void render(ID3D11DeviceContext& context)
	{
		ID3D11Buffer* pBuffers[1] = { mVertexBuffer };
		UINT strides[1] = { 6 * sizeof(float) };
		UINT offsets[1] = { 0 };
		context.IASetVertexBuffers(0, 1, pBuffers, strides, offsets);

		context.Draw(mNumVertices, 0);
	}
private:
	UINT mNumVertices;
	ID3D11Buffer* mVertexBuffer;

};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Box Mesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const float boxVertices[] =
{
		-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 

		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,

		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f,
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f,

		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f,

		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
		 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f,
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f,

		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f
};

class BoxMesh : public Mesh
{
public:
	BoxMesh() : Mesh(boxVertices, sizeof(boxVertices) / (6 * sizeof(boxVertices[0]))) {}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Plane Mesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


const float planeSize = 0.5f;

const float planeVertices[] =
{
	0,	planeSize,	planeSize,	1.0f,  0.0f,  0.0f,
	0, planeSize, -planeSize, 1.0f, 0.0f, 0.0f,
	0, -planeSize, -planeSize, 1.0f, 0.0f, 0.0f,
	0, -planeSize, -planeSize, 1.0f, 0.0f, 0.0f,
	0, -planeSize, planeSize, 1.0f, 0.0f, 0.0f,
	0, planeSize, planeSize, 1.0f, 0.0f, 0.0f,
};

class PlaneMesh : public Mesh
{
public:
	PlaneMesh() : Mesh(planeVertices, sizeof(planeVertices) / (6 * sizeof(planeVertices[0]))) {}
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											  Mesh Factory (Singleton)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class MeshFactory
{
public:
	~MeshFactory()
	{
		for (int i = 0; i < PxGeometryType::eGEOMETRY_COUNT; i++)
		{
			if (_meshes[i])
			{
				delete _meshes[i];
			}
		}
	}

	static MeshFactory& GetInstance()
	{
		static MeshFactory instance;
		return instance;
	}

	Mesh* GetMesh(PxGeometryType::Enum type)
	{
		if (_meshes[type] == NULL)
		{
			switch (type)
			{
			case PxGeometryType::eBOX:
				_meshes[type] = new BoxMesh();
				break;
			case PxGeometryType::ePLANE:
				_meshes[type] = new PlaneMesh();
				break;
			default:
				PX_ALWAYS_ASSERT_MESSAGE("Unsupported PxGeometryType");
				return NULL;
			}
		}

		return _meshes[type];
	}

private:
	MeshFactory() {}
	MeshFactory(const MeshFactory&);
	MeshFactory& operator=(MeshFactory&);

	Mesh* _meshes[PxGeometryType::eGEOMETRY_COUNT];
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													PhysXPrimitive
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


PhysXPrimitive::PhysXPrimitive(PxRigidActor* actor, PxVec3 scale) :
	mColor(1.0f, 1.0f, 1.0f)
{
	mActor = actor;
	mScale = scale;

	PxShape* buffer[1];
	actor->getShapes(buffer, 1);
	mMesh = MeshFactory::GetInstance().GetMesh(buffer[0]->getGeometryType());

}


PhysXPrimitive::~PhysXPrimitive()
{
}


PxMat44 PhysXPrimitive::getModelMatrix()
{
	return PxMat44(mActor->getGlobalPose()) * PxMat44(PxVec4(mScale, 1));
}


void PhysXPrimitive::render(ID3D11DeviceContext& context)
{
	context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mMesh->render(context);
}