#ifndef FMUTOOLSRUNTIMELINKING_H
#define FMUTOOLSRUNTIMELINKING_H

#include <string>
#include <stdexcept>
#include <iostream>

#if defined(_MSC_VER) || defined(WIN32) || defined(__MINGW32__)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <Errhandlingapi.h>
#    define DYNLIB_HANDLE HINSTANCE
#    define get_function_ptr GetProcAddress
#    include <system_error>
#    include <memory>
#    include <string>
#else
#    include <dlfcn.h>
#    define DYNLIB_HANDLE void*
#    define get_function_ptr dlsym
#endif



DYNLIB_HANDLE RuntimeLinkLibrary(const std::string& dynlib_dir, const std::string& dynlib_name){
#if WIN32
        if (!SetDllDirectory(dynlib_dir.c_str())){
            std::cerr << "ERROR: Could not locate the DLL directory." << std::endl;
            throw std::runtime_error("Could not locate the DLL directory.");
        }

        DYNLIB_HANDLE dynlib_handle = LoadLibrary(dynlib_name.c_str());

        if (!dynlib_handle){
            DWORD error = ::GetLastError();
            std::string message = std::system_category().message(error);

            std::cerr << "ERROR: DLL directory is found, but cannot load the library: " << dynlib_name << std::endl;
            std::cerr << "GetLastError returned: " << message << std::endl;

            throw std::runtime_error("Could not load the library.");
        }

        return dynlib_handle;
#else
        return dlopen(dynlib_name.c_str(), RTLD_LAZY);
#endif
}
    
#endif
