
``` _    ___                       
| |  / (_)__ _      _____  _____
| | / / / _ \ | /| / / _ \/ ___/
| |/ / /  __/ |/ |/ /  __/ /    
|___/_/\___/|__/|__/\___/_/     
```   
[FbxPipeline](https://github.com/VladSerhiienko/FbxPipeline) accompaning project.

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
cmake -G "Visual Studio 14 2015 Win64" -Bbuild_windows_amd64 "-H."
& 'C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe' build_windows_amd64\viewer.sln /p:Configuration=Debug
```
