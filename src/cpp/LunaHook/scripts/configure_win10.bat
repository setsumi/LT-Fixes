set https_proxy=http://127.0.0.1:7897
cmake -DWIN10ABOVE=ON -DBUILD_HOST=OFF ..\CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ..\build\x86_win10_1
cmake -DWIN10ABOVE=ON -DBUILD_HOOK=OFF ..\CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T host=x86 -B ..\build\x86_win10_2
cmake -DWIN10ABOVE=ON -DBUILD_HOST=OFF ..\CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ..\build\x64_win10_1
cmake -DWIN10ABOVE=ON -DBUILD_HOOK=OFF ..\CMakeLists.txt -G "Visual Studio 17 2022" -A x64 -T host=x64 -B ..\build\x64_win10_2