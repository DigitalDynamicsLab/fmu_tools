#ifndef FMUTOOLSRUNTIMELINKING_H
#define FMUTOOLSRUNTIMELINKING_H

#include <string>
#include <stdexcept>


#if defined(_MSC_VER) || defined(WIN32) || defined(__MINGW32__)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    define DYNLIB_HANDLE HINSTANCE
#    define get_function_ptr GetProcAddress
#else
#    include <dlfcn.h>
#    define DYNLIB_HANDLE void*
#    define get_function_ptr dlsym
#endif



DYNLIB_HANDLE RuntimeLinkLibrary(const std::string& dynlib_dir, const std::string& dynlib_name){
#if WIN32
        if (!SetDllDirectory(dynlib_dir.c_str()))
            throw std::runtime_error("Could not locate the binaries directory.");

        return LoadLibrary(dynlib_name.c_str());
#else
        return = dlopen(dynlib_name.c_str(), RTLD_LAZY);
#endif
}
    
#endif