

set(mecabsrc "${CMAKE_CURRENT_LIST_DIR}/mecab/mecab/src")
set(mecabsrcnew "${CMAKE_CURRENT_BINARY_DIR}/mecab")



if(NOT EXISTS "${mecabsrcnew}/mecab.h")
    
    file(GLOB SOURCES "${mecabsrc}/*.cpp")
    file(GLOB SOURCESH "${mecabsrc}/*.h")

    list(REMOVE_ITEM SOURCES "${mecabsrc}/mecab-cost-train.cpp")
    list(REMOVE_ITEM SOURCES "${mecabsrc}/mecab-dict-gen.cpp")
    list(REMOVE_ITEM SOURCES "${mecabsrc}/mecab-dict-index.cpp")
    list(REMOVE_ITEM SOURCES "${mecabsrc}/mecab-system-eval.cpp")
    list(REMOVE_ITEM SOURCES "${mecabsrc}/mecab-test-gen.cpp")

    list(REMOVE_ITEM SOURCESH "${mecabsrc}/mecab.h")

    file(COPY ${SOURCES} DESTINATION "${mecabsrcnew}")
    file(COPY ${SOURCESH} DESTINATION "${mecabsrcnew}")

    if(NOT WINXP)
        file(READ "${mecabsrc}/dictionary.cpp" dictionary)
        set(dictionary "namespace std{\n    template<class Argv1, class Argv2, class Result>\n      struct binary_function\n      {\n          typedef Argv1 first_argument_type;\n          typedef Argv2 second_argument_type;\n          typedef Result result_type;\n      };\n  }\n"${dictionary})
        file(WRITE "${mecabsrcnew}/dictionary.cpp" "${dictionary}")
        list(REMOVE_ITEM SOURCES "${mecabsrc}/dictionary.cpp")
        list(APPEND SOURCES "${mecabsrcnew}/dictionary.cpp")
    endif()

    file(READ "${mecabsrc}/mecab.h" mecabh)
    string(REPLACE "#  ifdef DLL_EXPORT\n#    define MECAB_DLL_EXTERN  __declspec(dllexport)\n#    define MECAB_DLL_CLASS_EXTERN  __declspec(dllexport)\n#  else\n#    define MECAB_DLL_EXTERN  __declspec(dllimport)\n#  endif" "" mecabh "${mecabh}")
    file(WRITE "${mecabsrcnew}/mecab.h" "${mecabh}")
endif()

file(GLOB SOURCES "${mecabsrcnew}/*.cpp") 


add_library(libmecab ${SOURCES})

target_include_directories(libmecab INTERFACE "${mecabsrc}")
target_compile_options(libmecab PRIVATE /EHsc
                                        /std:c++14
                                        /GL         
                                        /GA       
                                        /Ob2        
                                        /W3         
                                        /nologo    
                                        /Zi        
                                        /wd4800    
                                        /wd4305
                                        /wd4267
                                        /wd4244)
target_compile_definitions(
    libmecab PRIVATE
    -D_CRT_SECURE_NO_DEPRECATE
    -DMECAB_USE_THREAD
    -DDLL_EXPORT
    -DHAVE_GETENV
    -DHAVE_WINDOWS_H
    -DDIC_VERSION=102
    -DVERSION="0.996"
    -DPACKAGE="mecab"
    -DUNICODE
    -D_UNICODE
    -DMECAB_DEFAULT_RC="c:\\\\Program Files\\\\mecab\\\\etc\\\\mecabrc"
)