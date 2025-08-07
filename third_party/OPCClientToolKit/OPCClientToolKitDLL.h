#pragma once

// 兼容静态和动态库的导出宏定义

#if defined(OPCCLIENTTOOLKIT_STATIC)
    #define OPCDACLIENT_API
#elif defined(OPCDACLIENT_LIBRARY)
    #define OPCDACLIENT_API __declspec(dllexport)
#elif defined(_MSC_VER)
    #define OPCDACLIENT_API __declspec(dllimport)
#else
    #define OPCDACLIENT_API
#endif
