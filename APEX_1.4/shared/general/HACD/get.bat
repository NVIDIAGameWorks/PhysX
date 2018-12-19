p4 edit .../*.h
p4 edit .../*.cpp
cd include
copy \GoogleCode\hacd\hacd\include\*.h
p4 add *.h
cd ..
cd public
copy \GoogleCode\hacd\hacd\public\*.h
p4 revert PlatformConfigHACD.h
p4 add *.h
cd ..
cd src
copy \GoogleCode\hacd\hacd\src\*.cpp
p4 add *.cpp
cd ..

