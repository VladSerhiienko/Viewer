# Viewer
[FbxPipeline](https://github.com/VladSerhiienko/FbxPipeline) accompaning project.

## How to build (Linux, bash + cmake + make):

### Bash
```
cd <vulkan directory>/source/shaderc
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
cd ..

cd flatbuffers
git checkout master
git pull origin master
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64
cd ..

cd EASTL
git checkout master
git pull origin master
cmake -Bbuild_linux_x86-64 -H. -DEASTL_BUILD_TESTS:BOOL=TRUE
make -C build_linux_x86-64
cd ..

cd ..
cmake -Bbuild_linux_x86-64 -H.
make -C build_linux_x86-64

```

## How to build (Windows, PS + cmake + MSBuild): 

### PowerShell:
```
cd <vulkan directory>/shaderc
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 -H. -DSHADERC_SKIP_TESTS:BOOL=TRUE -DSHADERC_ENABLE_SHARED_CRT:BOOL=TRUE

cd build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' libshaderc\libshaderc.sln /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' libshaderc\libshaderc.sln /p:Configuration=Release

cd <dev directory>
git clone git@github.com:VladSerhiienko/Viewer.git
cd Viewer
git submodule init
git submodule update --recursive

cd third_party
Expand-Archive SDL2-2.0.7.zip .

cd SDL2-2.0.7
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 "-H."
cd build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' SDL2.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' SDL2.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

cd flatbuffers
git checkout master
git pull origin master
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 "-H."
cd build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' FlatBuffers.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' FlatBuffers.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

cd EASTL
git checkout master
git pull origin master
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 "-H." -DEASTL_BUILD_TESTS:BOOL=TRUE
cd build_windows_x86-64
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' EASTL.sln /target:ALL_BUILD /p:Configuration=Debug
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' EASTL.sln /target:ALL_BUILD /p:Configuration=Release
cd ../../

cd ..
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_x86-64 "-H."


```
