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



#ifndef MESH_IMPORT_BUILDER_H

#define MESH_IMPORT_BUILDER_H

#include "MeshImport.h"

namespace mimp
{

class MeshBuilder : public MeshSystem, public MeshImportInterface
{
public:
  virtual void gather(void) = 0;
  virtual void scale(MiF32 s) = 0;
  virtual void rotate(MiF32 rotX,MiF32 rotY,MiF32 rotZ) = 0;
  virtual void setMeshImportBinary(bool state) = 0;


};

MeshBuilder * createMeshBuilder(const char *meshName,
                                const void *data,
                                MiU32 dlen,
                                MeshImporter *mi,
                                const char *options,
                                MeshImportApplicationResource *appResource);


MeshBuilder * createMeshBuilder(MeshImportApplicationResource *appResource);
void          releaseMeshBuilder(MeshBuilder *m);

}; // end of namespace

#endif
