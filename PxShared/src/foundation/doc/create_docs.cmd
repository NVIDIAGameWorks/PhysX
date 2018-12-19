set DOXYGEN_DIR=..\..\..\..\..\..\..\devrel\GameWorks\BuildTools\doxygen-win\bin
set HTMLHELP_DIR=..\..\..\..\..\..\..\devrel\GameWorks\BuildTools\HTMLHelpWorkshop

%DOXYGEN_DIR%\doxygen.exe docs.doxyfile
cd html
..\%HTMLHELP_DIR%\hhc.exe index.hhp
cd ..
