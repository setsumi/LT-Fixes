set https_proxy=http://127.0.0.1:7897
cmake --build ..\build\x86_winxp --config Release --target ALL_BUILD -j 14
copy ..\builds\_x86_winxp\shareddllproxy32.exe ..\..\files
robocopy ..\builds\_x86_winxp ..\..\files\DLL32 *.dll