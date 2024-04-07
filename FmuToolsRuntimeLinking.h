#ifndef FMUTOOLS_RUNTIMELINKING_H
#define FMUTOOLS_RUNTIMELINKING_H

#include <string>
#include <stdexcept>
#include <iostream>
#include "filesystem.hpp"

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
#    include <limits.h>
#    define DYNLIB_HANDLE void*
#    define get_function_ptr dlsym
#endif

#include <iostream>


/// Runtime/Dynamic linking of shared library.
/// @param dynlib_dir The directory where the shared library is located.
/// @param dynlib_name The name of the shared library.
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

/// Gets the location of the shared library.
std::string GetLibraryLocation(){
    fs::path library_path;
#if WIN32
    HMODULE hModule = nullptr;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&GetLibraryLocation, &hModule))
    {
        char library_pathc[MAX_PATH];
        DWORD result = GetModuleFileNameA(hModule, library_pathc, MAX_PATH);
        if (result != 0) {
            //std::cout << "The path of the FMU shared library is: " << library_pathc << std::endl;
            library_path = std::string(library_pathc);
        } else {
            std::cerr << "Error getting the FMU shared library name. Error code: " << GetLastError() << std::endl;
        }
    } else {
        std::cerr << "Error getting the FMU shared library handle. Error code: " << GetLastError() << std::endl;
    }
#else
    void* sym_ptr = dlsym(RTLD_DEFAULT, "fmi2DoStep");
    if (sym_ptr == NULL) {
        std::cout << "Warning: FmuToolsRuntimeLinking: sym_ptr to fmi2DoStep is Null" << std::endl;
    }
    Dl_info dl_info;
    if (dladdr(sym_ptr, &dl_info)) {
        char library_pathc[PATH_MAX];
        library_path = realpath(dl_info.dli_fname, library_pathc);
        //std::cout << "The path of the FMU shared library is: " << library_pathc << std::endl;
        library_path = std::string(library_pathc);
    } else {
        std::cerr << "Error getting the FMU shared library information." << std::endl;
    }
#endif

    fs::path library_folder = library_path.parent_path();
    return library_folder.string();
}
    
#endif // end FMUTOOLSRUNTIMELINKING_H
