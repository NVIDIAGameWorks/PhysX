// #include "foundation/PxMath.h"
#include <iostream>
#include "pxshared/include/foundation/PxMath.h"
#include "physx/source/common/src/CmQueue.h"
#include "physx/source/common/src/CmSpatialVector.h"
#include "physx/source/foundation/include/PsTime.h"
#include "physx/include/geometry/PxTriangle.h"
int main() {

  std::cout << "DEBUG " << physx::PxPi << std::endl;
  physx::shdfnd::Time time;
  std::cout << "DEBUG time " << time.getElapsedSeconds() << std::endl;
  physx::Cm::SpatialVector v(physx::PxVec3(0, 1, 1), physx::PxVec3(0, 1, 2));
  std::cout << "DEBUG sv " << v.magnitude() << std::endl;
  physx::PxTriangle triangle(physx::PxVec3(0, 0, 0),
      physx::PxVec3(0, 10, 0),
      physx::PxVec3(1, 0, 0));
  std::cout << "DEBUG triangle area " << triangle.area() << std::endl;
  // TODO: Queue causes segfault.
  physx::Cm::Queue<int> q(3);
  std::cout << "DEBUG " << q.size() << std::endl;
  q.pushBack(8);
  std::cout << "DEBUG " << q.size() << std::endl;
  return 0;
}
