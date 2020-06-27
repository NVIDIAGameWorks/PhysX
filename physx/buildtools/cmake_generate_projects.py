import sys
import os
import glob
import os.path
import shutil
import subprocess
import xml.etree.ElementTree


def packmanExt():
    if sys.platform == 'win32':
        return 'cmd'
    return 'sh'


def cmakeExt():
    if sys.platform == 'win32':
        return '.exe'
    return ''


def filterPreset(presetName):
    winPresetFilter = ['win','uwp','ps4','switch','xboxone','android','crosscompile', 'emscripten']
    if sys.platform == 'win32':        
        if any(presetName.find(elem) != -1 for elem in winPresetFilter):
            return True
    else:        
        if all(presetName.find(elem) == -1 for elem in winPresetFilter):
            return True
    return False

def noPresetProvided():
    global input
    print('Preset parameter required, available presets:')
    presetfiles = []
    for file in glob.glob("buildtools/presets/*.xml"):
        presetfiles.append(file)

    if len(presetfiles) == 0:
        for file in glob.glob("buildtools/presets/public/*.xml"):
            presetfiles.append(file)

    counter = 0
    presetList = []
    for preset in presetfiles:
        if filterPreset(preset):
            presetXml = xml.etree.ElementTree.parse(preset).getroot()
            if(preset.find('user') == -1):
                print('(' + str(counter) + ') ' + presetXml.get('name') +
                    ' <--- ' + presetXml.get('comment'))
                presetList.append(presetXml.get('name'))
            else:
                print('(' + str(counter) + ') ' + presetXml.get('name') +
                    '.user <--- ' + presetXml.get('comment'))
                presetList.append(presetXml.get('name') + '.user')
            counter = counter + 1            
    # Fix Python 2.x.
    try: 
    	input = raw_input
    except NameError: 
    	pass    
    mode = int(input('Enter preset number: '))
    print('Running generate_projects.bat ' + presetList[mode])
    return presetList[mode]

class CMakePreset:
    presetName = ''
    targetPlatform = ''
    compiler = ''
    cmakeSwitches = []
    cmakeParams = []

    def __init__(self, presetName):
        xmlPath = "buildtools/presets/"+presetName+'.xml'
        if os.path.isfile(xmlPath):
            print('Using preset xml: '+xmlPath)
        else:
            xmlPath = "buildtools/presets/public/"+presetName+'.xml'
            if os.path.isfile(xmlPath):
                print('Using preset xml: '+xmlPath)
            else:
                print('Preset xml file: '+xmlPath+' not found')
                exit()

        # get the xml
        presetNode = xml.etree.ElementTree.parse(xmlPath).getroot()
        self.presetName = presetNode.attrib['name']
        for platform in presetNode.findall('platform'):
            self.targetPlatform = platform.attrib['targetPlatform']
            self.compiler = platform.attrib['compiler']
            print('Target platform: ' + self.targetPlatform +
                  ' using compiler: ' + self.compiler)

        for cmakeSwitch in presetNode.find('CMakeSwitches'):
            cmSwitch = '-D' + \
                cmakeSwitch.attrib['name'] + '=' + \
                cmakeSwitch.attrib['value'].upper()
            self.cmakeSwitches.append(cmSwitch)

        for cmakeParam in presetNode.find('CMakeParams'):
            if cmakeParam.attrib['name'] == 'CMAKE_INSTALL_PREFIX' or cmakeParam.attrib['name'] == 'PX_OUTPUT_LIB_DIR' or cmakeParam.attrib['name'] == 'PX_OUTPUT_EXE_DIR' or cmakeParam.attrib['name'] == 'PX_OUTPUT_DLL_DIR':
                cmParam = '-D' + cmakeParam.attrib['name'] + '=\"' + \
                    os.environ['PHYSX_ROOT_DIR'] + '/' + \
                    cmakeParam.attrib['value'] + '\"'
            elif cmakeParam.attrib['name'] == 'ANDROID_ABI':
                cmParam = '-D' + \
                    cmakeParam.attrib['name'] + '=\"' + \
                    cmakeParam.attrib['value'] + '\"'
            else:
                cmParam = '-D' + \
                    cmakeParam.attrib['name'] + '=' + \
                    cmakeParam.attrib['value']
            self.cmakeParams.append(cmParam)
    pass

    def isMultiConfigPlatform(self):
        if self.targetPlatform == 'linux':
            return False
        elif self.targetPlatform == 'linuxAarch64':
            return False
        elif self.targetPlatform == 'android':
            return False
        if self.targetPlatform == 'emscripten':
            return False
        return True

    def getCMakeSwitches(self):
        outString = ''
        for cmakeSwitch in self.cmakeSwitches:
            outString = outString + ' ' + cmakeSwitch
            if cmakeSwitch.find('PX_GENERATE_GPU_PROJECTS') != -1:
                if os.environ.get('PM_CUDA_PATH') is not None:
                    outString = outString + ' -DCUDA_TOOLKIT_ROOT_DIR=' + \
                        os.environ['PM_CUDA_PATH']
                if self.compiler == 'vc15':
                    print('VS15CL:' + os.environ['VS150CLPATH'])
                    outString = outString + ' -DCUDA_HOST_COMPILER=' + \
                        os.environ['VS150CLPATH']

        return outString

    def getCMakeParams(self):
        outString = ''
        for cmakeParam in self.cmakeParams:
            outString = outString + ' ' + cmakeParam
        return outString

    def getPlatformCMakeParams(self):
        outString = ' '
        if self.compiler == 'vc12':
            outString = outString + '-G \"Visual Studio 12 2013\"'
        elif self.compiler == 'vc14':
            outString = outString + '-G \"Visual Studio 14 2015\"'
        elif self.compiler == 'vc15':
            outString = outString + '-G \"Visual Studio 15 2017\"'
        elif self.compiler == 'vc16':
            outString = outString + '-G \"Visual Studio 16 2019\"'
        elif self.compiler == 'xcode':
            outString = outString + '-G Xcode'
        elif self.targetPlatform == 'android':
            outString = outString + '-G \"MinGW Makefiles\"'
        elif self.targetPlatform == 'linux':
            outString = outString + '-G \"Unix Makefiles\"'
        elif self.targetPlatform == 'linuxAarch64':
            outString = outString + '-G \"Unix Makefiles\"'

        if self.targetPlatform == 'win32':
            outString = outString + ' -AWin32'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=windows'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            return outString
        elif self.targetPlatform == 'win64':
            outString = outString + ' -Ax64'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=windows'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            return outString
        elif self.targetPlatform == 'uwp64':
            outString = outString + ' -Ax64'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=uwp'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            outString = outString + ' -DCMAKE_SYSTEM_NAME=WindowsStore'
            outString = outString + ' -DCMAKE_SYSTEM_VERSION=10.0'            
            return outString
        elif self.targetPlatform == 'uwp32':
            outString = outString + ' -AWin32'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=uwp'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            outString = outString + ' -DCMAKE_SYSTEM_NAME=WindowsStore'
            outString = outString + ' -DCMAKE_SYSTEM_VERSION=10.0'            
            return outString
        elif self.targetPlatform == 'uwparm32':
            outString = outString + ' -AARM'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=uwp'
            outString = outString + ' -DPX_OUTPUT_ARCH=arm'
            outString = outString + ' -DCMAKE_SYSTEM_NAME=WindowsStore'
            outString = outString + ' -DCMAKE_SYSTEM_VERSION=10.0'            
            return outString
        elif self.targetPlatform == 'uwparm64':
            outString = outString + ' -AARM64'
            outString = outString + ' -DTARGET_BUILD_PLATFORM=uwp'
            outString = outString + ' -DPX_OUTPUT_ARCH=arm'
            outString = outString + ' -DCMAKE_SYSTEM_NAME=WindowsStore'
            outString = outString + ' -DCMAKE_SYSTEM_VERSION=10.0'            
            return outString
        elif self.targetPlatform == 'ps4':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=ps4'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                os.environ['PM_CMakeModules_PATH'] + '/ps4/PS4Toolchain.txt'
            outString = outString + ' -DCMAKE_GENERATOR_PLATFORM=ORBIS'
            outString = outString + ' -DSUPPRESS_SUFFIX=ON'
            return outString
        elif self.targetPlatform == 'xboxone':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=xboxone'
            if self.compiler == 'vc14':
                outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                    os.environ['PM_CMakeModules_PATH'] + \
                    '/xboxone/XboxOneToolchain.txt'
            elif self.compiler == 'vc15':
                outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                    os.environ['PM_CMakeModules_PATH'] + \
                    '/xboxone/XboxOneToolchainVC15.txt'
                outString = outString + ' -T v141'
                outString = outString + ' -DCMAKE_VS150PATH=' + \
                    os.environ['VS150PATH']
            outString = outString + ' -DCMAKE_GENERATOR_PLATFORM=Durango'
            outString = outString + ' -DSUPPRESS_SUFFIX=ON'
            return outString
        elif self.targetPlatform == 'switch32':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=switch'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                os.environ['PM_CMakeModules_PATH'] + \
                '/switch/NX32Toolchain.txt'
            outString = outString + ' -DCMAKE_GENERATOR_PLATFORM=NX32'
            return outString
        elif self.targetPlatform == 'switch64':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=switch'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                os.environ['PM_CMakeModules_PATH'] + \
                '/switch/NX64Toolchain.txt'
            outString = outString + ' -DCMAKE_GENERATOR_PLATFORM=NX64'
            return outString
        elif self.targetPlatform == 'android':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=android'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                os.environ['PM_CMakeModules_PATH'] + \
                '/android/android.toolchain.cmake'
            outString = outString + ' -DANDROID_STL=\"gnustl_static\"'
            outString = outString + ' -DCM_ANDROID_FP=\"softfp\"'
            if os.environ.get('PM_AndroidNDK_PATH') is None:
                print('Please provide path to android NDK in variable PM_AndroidNDK_PATH.')
                exit(-1)
            else:
                outString = outString + ' -DANDROID_NDK=' + \
                    os.environ['PM_AndroidNDK_PATH']
                outString = outString + ' -DCMAKE_MAKE_PROGRAM=\"' + \
                    os.environ['PM_AndroidNDK_PATH'] + '\\prebuilt\\windows\\bin\\make.exe\"'
            return outString
        elif self.targetPlatform == 'linux':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=linux'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            if self.compiler == 'clang-crosscompile':
                outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                    os.environ['PM_CMakeModules_PATH'] + \
                    '/linux/LinuxCrossToolchain.x86_64-unknown-linux-gnu.cmake'
            elif self.compiler == 'clang':
                if os.environ.get('PM_clang_PATH') is not None:
                    outString = outString + ' -DCMAKE_C_COMPILER=' + \
                        os.environ['PM_clang_PATH'] + '/bin/clang'
                    outString = outString + ' -DCMAKE_CXX_COMPILER=' + \
                        os.environ['PM_clang_PATH'] + '/bin/clang++'
                else:
                    outString = outString + ' -DCMAKE_C_COMPILER=clang'
                    outString = outString + ' -DCMAKE_CXX_COMPILER=clang++'
            return outString
        elif self.targetPlatform == 'linuxAarch64':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=linux'
            outString = outString + ' -DPX_OUTPUT_ARCH=arm'
            if self.compiler == 'clang-crosscompile':
                outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=' + \
                    os.environ['PM_CMakeModules_PATH'] + \
                    '/linux/LinuxCrossToolchain.aarch64-unknown-linux-gnueabihf.cmake'
            elif self.compiler == 'gcc':
                outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=\"' + \
                    os.environ['PM_CMakeModules_PATH'] + \
                    '/linux/LinuxAarch64.cmake\"'
            return outString
        elif self.targetPlatform == 'mac64':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=mac'
            outString = outString + ' -DPX_OUTPUT_ARCH=x86'
            return outString
        elif self.targetPlatform == 'ios64':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=ios'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=\"' + \
                os.environ['PM_CMakeModules_PATH'] + '/ios/ios.toolchain.cmake\"'
            outString = outString + ' -DPX_OUTPUT_ARCH=arm'
            return outString
        elif self.targetPlatform == 'emscripten':
            outString = outString + ' -DTARGET_BUILD_PLATFORM=emscripten'
            outString = outString + ' -DCMAKE_TOOLCHAIN_FILE=\"' + \
                os.path.join(os.environ['EMSCRIPTEN'] + '/cmake/Modules/Platform/Emscripten.cmake\"')
            return outString
        return ''


def getCommonParams():
    outString = '--no-warn-unused-cli'
    outString = outString + ' -DCMAKE_PREFIX_PATH=\"' + os.environ['PM_PATHS'] + '\"'
    outString = outString + ' -DPHYSX_ROOT_DIR=\"' + \
        os.environ['PHYSX_ROOT_DIR'] + '\"'
    outString = outString + ' -DPX_OUTPUT_LIB_DIR=\"' + \
        os.environ['PHYSX_ROOT_DIR'] + '\"'
    outString = outString + ' -DPX_OUTPUT_BIN_DIR=\"' + \
        os.environ['PHYSX_ROOT_DIR'] + '\"'
    if os.environ.get('GENERATE_SOURCE_DISTRO') == '1':
        outString = outString + ' -DPX_GENERATE_SOURCE_DISTRO=1'
    return outString

def cleanupCompilerDir(compilerDirName):
    if os.path.exists(compilerDirName):
        if sys.platform == 'win32':
            os.system('rmdir /S /Q ' + compilerDirName)
        else:
            shutil.rmtree(compilerDirName, True)
    if os.path.exists(compilerDirName) == False:
        os.makedirs(compilerDirName)

def presetProvided(pName):
    parsedPreset = CMakePreset(pName)

    print('PM_CMakeModules_PATH: ' + os.environ['PM_CMakeModules_PATH'])
    print('PM_PATHS: ' + os.environ['PM_PATHS'])

    if os.environ.get('PM_cmake_PATH') is not None:
        cmakeExec = os.environ['PM_cmake_PATH'] + '/bin/cmake' + cmakeExt()
    elif pName == "emscripten":
        cmakeExec = 'emcmake cmake' + cmakeExt()
    else:
        cmakeExec = 'cmake' + cmakeExt()
    print('Cmake: ' + cmakeExec)

    # gather cmake parameters
    cmakeParams = parsedPreset.getPlatformCMakeParams()
    cmakeParams = cmakeParams + ' ' + getCommonParams()
    cmakeParams = cmakeParams + ' ' + parsedPreset.getCMakeSwitches()
    cmakeParams = cmakeParams + ' ' + parsedPreset.getCMakeParams()
    # print(cmakeParams)

    if os.path.isfile(os.environ['PHYSX_ROOT_DIR'] + '/compiler/internal/CMakeLists.txt'):
        cmakeMasterDir = 'internal'
    else:
        cmakeMasterDir = 'public'
    if parsedPreset.isMultiConfigPlatform():
        # cleanup and create output directory
        outputDir = os.path.join('compiler', parsedPreset.presetName)
        cleanupCompilerDir(outputDir)

        # run the cmake script
        # print('Cmake params:' + cmakeParams)
        os.chdir(os.path.join(os.environ['PHYSX_ROOT_DIR'], outputDir))
        os.system(cmakeExec + ' \"' +
                  os.environ['PHYSX_ROOT_DIR'] + '/compiler/' + cmakeMasterDir + '\"' + cmakeParams)
        os.chdir(os.environ['PHYSX_ROOT_DIR'])
    else:
        configs = ['debug', 'checked', 'profile', 'release']
        for config in configs:
            # cleanup and create output directory
            outputDir = os.path.join('compiler', parsedPreset.presetName + '-' + config)
            cleanupCompilerDir(outputDir)

            # run the cmake script
            # print('Cmake params:' + cmakeParams)
            os.chdir(os.path.join(os.environ['PHYSX_ROOT_DIR'], outputDir))
            # print(cmakeExec + ' \"' + os.environ['PHYSX_ROOT_DIR'] + '/compiler/' + cmakeMasterDir + '\"' + cmakeParams + ' -DCMAKE_BUILD_TYPE=' + config)
            os.system(cmakeExec + ' \"' + os.environ['PHYSX_ROOT_DIR'] + '/compiler/' +
                      cmakeMasterDir + '\"' + cmakeParams + ' -DCMAKE_BUILD_TYPE=' + config)
            os.chdir(os.environ['PHYSX_ROOT_DIR'])
    pass


def main():
    if len(sys.argv) != 2:
        presetName = noPresetProvided()
        os.chdir(os.environ['PHYSX_ROOT_DIR'])
        if sys.platform == 'win32':
            os.system('generate_projects.bat ' + presetName)
        else:
            os.system('./generate_projects.sh ' + presetName)
    else:
        presetName = sys.argv[1]
        if filterPreset(presetName):
            presetProvided(presetName)
        else:
            print('Preset not supported on this build platform.')

main()
