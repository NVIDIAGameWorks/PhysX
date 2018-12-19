/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgSMALLDETERMINANT__
#define __dgSMALLDETERMINANT__

#include "dgTypes.h"

class dgGoogol;
double Determinant2x2 (const double matrix[2][2], double* const error);
double Determinant3x3 (const double matrix[3][3], double* const error);
double Determinant4x4 (const double matrix[4][4], double* const error);


dgGoogol Determinant2x2 (const dgGoogol matrix[2][2]);
dgGoogol Determinant3x3 (const dgGoogol matrix[3][3]);
dgGoogol Determinant4x4 (const dgGoogol matrix[4][4]);

#endif
