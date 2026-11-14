# HTTPS through what the system provides (WinHTTP, NSURLSession on iOS, or the
# system's libcurl loaded at run time): the launcher's updates and competitive play.
include_guard(GLOBAL)
find_package(Threads REQUIRED)
add_library(charged_http STATIC src/platform/http_client.cpp)
target_include_directories(charged_http PUBLIC src)
target_compile_features(charged_http PUBLIC cxx_std_17)
target_link_libraries(charged_http PUBLIC Threads::Threads)
if(WIN32)
    target_link_libraries(charged_http PRIVATE winhttp)
elseif(MSCHARGED_IOS)
    # No libcurl on iOS: NSURLSession, so the file is Objective-C++ there.
    enable_language(OBJCXX)
    set_source_files_properties(src/platform/http_client.cpp PROPERTIES LANGUAGE OBJCXX COMPILE_OPTIONS -fobjc-arc)
    target_link_libraries(charged_http PRIVATE "-framework Foundation")
else()
    target_link_libraries(charged_http PRIVATE ${CMAKE_DL_LIBS})
endif()
