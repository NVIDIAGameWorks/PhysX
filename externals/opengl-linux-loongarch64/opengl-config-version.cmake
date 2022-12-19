set(PACKAGE_VERSION "2017.5.19")
 
if("2017.5.19" MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)\\.") # strip the tweak version
  set(CVF_VERSION_NO_TWEAK "${CMAKE_MATCH_1}")
else()
  set(CVF_VERSION_NO_TWEAK "2017.5.19")
endif()

if(PACKAGE_FIND_VERSION MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)\\.") # strip the tweak version
  set(REQUESTED_VERSION_NO_TWEAK "${CMAKE_MATCH_1}")
else()
  set(REQUESTED_VERSION_NO_TWEAK "${PACKAGE_FIND_VERSION}")
endif()

if(REQUESTED_VERSION_NO_TWEAK STREQUAL CVF_VERSION_NO_TWEAK)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
else()
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
endif()

if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_EXACT TRUE)
endif()

