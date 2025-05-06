
include(FetchContent)

FetchContent_Declare(webview2 
    URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2535.41
    URL_HASH SHA1=5991fa16cdbd1e7da9ac8226fd04470da4918337
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(webview2)

set(webview2_include ${webview2_SOURCE_DIR}/build/native/include)

add_library(webview2 INTERFACE)
target_include_directories(webview2 INTERFACE ${webview2_include})
target_link_libraries(webview2 INTERFACE ${webview2_SOURCE_DIR}/build/native/${platform}/WebView2LoaderStatic.lib)

if(WINXP)
  file(READ "${webview2_include}/WebView2EnvironmentOptions.h" WebView2EnvironmentOptions)
  string(REPLACE  "COREWEBVIEW2_RELEASE_CHANNELS_STABLE |\n      COREWEBVIEW2_RELEASE_CHANNELS_BETA | COREWEBVIEW2_RELEASE_CHANNELS_DEV |\n      COREWEBVIEW2_RELEASE_CHANNELS_CANARY | kInternalChannel" "(COREWEBVIEW2_RELEASE_CHANNELS)(0x1f)" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  string(REPLACE "#include <wrl/implements.h>" "" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  string(REPLACE "Microsoft::WRL::RuntimeClass<\n          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,\n          CoreWebView2EnvironmentOptionsBase<" "CoreWebView2EnvironmentOptionsBase<" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  string(REPLACE ">>" ">" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  string(REPLACE "Microsoft::WRL::RuntimeClass<\n          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>," "ComImpl<" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  string(REPLACE "Microsoft::WRL::Implements<\n          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>," "ComImpl<" WebView2EnvironmentOptions "${WebView2EnvironmentOptions}")
  file(WRITE "${webview2_include}/WebView2EnvironmentOptions.h" "${WebView2EnvironmentOptions}")

  file(READ "${webview2_include}/WebView2.h" WebView2H)
  string(REPLACE [=[#include "EventToken.h"]=] "typedef struct EventRegistrationToken\n    {\n    __int64 value;\n    } 	EventRegistrationToken;" WebView2H "${WebView2H}")
  file(WRITE "${webview2_include}/WebView2.h" "${WebView2H}")

endif()