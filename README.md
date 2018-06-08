
``` _    ___                       
| |  / (_)__ _      _____  _____
| | / / / _ \ | /| / / _ \/ ___/
| |/ / /  __/ |/ |/ /  __/ /    
|___/_/\___/|__/|__/\___/_/     
```   

[![Build status](https://ci.appveyor.com/api/projects/status/b6e2hy037a1d4bmv?svg=true)](https://ci.appveyor.com/project/VladSerhiienko/viewer)
[![Build Status](https://travis-ci.org/VladSerhiienko/Viewer.svg?branch=master)](https://travis-ci.org/VladSerhiienko/Viewer)

[FbxPipeline](https://github.com/VladSerhiienko/FbxPipeline) accompaning project.

## Demo:
![alt text](https://github.com/VladSerhiienko/FbxPipeline/blob/master/docs/AK.PNG "AK")
![alt text](https://github.com/VladSerhiienko/FbxPipeline/blob/master/docs/Cerberus.PNG "Cerberus")
![alt text](https://github.com/VladSerhiienko/FbxPipeline/blob/master/docs/9mm.PNG "9mm")

## How to build (Linux, bash + cmake + make):

### Bash
```
cd <dev directory>
git clone git@github.com:VladSerhiienko/Viewer.git
cd Viewer
cmake -Bbuild_linux_x86_64 -H.
make -C build_linux_x86_64
```

## How to build (Windows, PS + cmake + MSBuild): 

### PowerShell:
```
cd <dev directory>
git clone git@github.com:VladSerhiienko/Viewer.git
cd Viewer
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_amd64_msvc "-H."
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' build_windows_amd64_msvc\viewer.sln /p:Configuration=Debug
```
