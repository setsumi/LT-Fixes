set https_proxy=http://127.0.0.1:7897
cmake -DWINXP=ON ../CMakeLists.txt -G "Visual Studio 17 2022" -A win32 -T v141_xp -B ../build/x86_winxp
