If the Physx API is changed, the API level serialization meta data files need to be updated.
This can be done by running the following cmd line:

python generateMetaData.py (osx, linux or windows with > python 2.7)

On windows generateMetaData.bat can be run to execute generateMetaData.py. 
It look for python in
	1. environment variable PYTHON
	2. p4sw location (NVidia only)
	3. PATH
	
Linux and osx distributions mostly come with Python pre-installed. On windows please download
and install the latest python version from here: 
	https://www.python.org/downloads 
and make sure to add python to your windows PATH (option in python installer)

The generateMetaData.py tests for meta data files being writable. If not, p4 commands are 
used to open the files for edit before the update. If p4 is not available (or not configured for cmd line usage, see https://www.perforce.com/perforce/r15.1/manuals/p4guide/chapter.configuration.html: "Using config files"), 
the files need to be made writable manually.

Requirements:
Windows: Visual Studio 2015

Testing:
generateMetaData.py -test runs the meta data generation in test mode. This mode checks that the current
set of meta data files is still in sync with the PhysX API (i.e. already generated meta data files). 

