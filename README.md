# Viewer



## How to build (Ubuntu):

###Bash
```
cd <vulkan directory>
cd source/shaderc
python update_shaderc_sources.py
cd src
cmake -Bbuild_linux_x86-64 -H. -DSHADERC_SKIP_TESTS:BOOL=TRUE
make -C build_linux_x86-64/libshaderc

cd <dev directory>
git clone git@github.com:VladSerhiienko/Viewer.git
cd Viewer
git submodule init
git submodule update --recursive

cd third_party
unzip SDL2-2.0.7.zip

cd SDL2-2.0.7
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64

cd ../flatbuffers
git checkout master
git pull origin master
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64
cd ..

cd ../EASTL
git checkout master
git pull origin master
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H. -DEASTL_BUILD_TESTS:BOOL=TRUE

cd ..
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64

```

## How to build (Windows): 

###Bash:
```
cd <vulkan directory>
cd shaderc
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H. -DSHADERC_SKIP_TESTS:BOOL=TRUE -DSHADERC_ENABLE_SHARED_CRT:BOOL=TRUE

cd <dev directory>
git clone git@github.com:VladSerhiienko/Viewer.git
cd Viewer
git submodule init
git submodule update --recursive

cd third_party
unzip SDL2-2.0.7.zip

cd SDL2-2.0.7.zip
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H.

cd ../flatbuffers
git checkout master
git pull origin master
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H.

cd ../EASTL
git checkout master
git pull origin master
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H. -DEASTL_BUILD_TESTS:BOOL=TRUE

```

###PowerShell:
```
cd <vulkan directory>
cd shaderc/build_windows_x86-64

& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' libshaderc\libshaderc.sln /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' libshaderc\libshaderc.sln /p:Configuration=Release

cd <project directory>
cd third_party

& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' zlib.sln /target:ALL_BUILD /p:Configuration=Release

cd flatbuffers/build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' FlatBuffers.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' FlatBuffers.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

cd SDL2-2.0.7/build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' SDL2.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' SDL2.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

cd EASTL/build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' EASTL.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' EASTL.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

```

###Bash:
```
cd ..
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64

```