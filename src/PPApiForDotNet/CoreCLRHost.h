#pragma once

#include <filesystem>

HRESULT StartClrHost();

HRESULT CreateManagedDelegate(
    _In_ std::wstring szAssemblyName,          // Name of the assembly containing the method
    _In_ std::wstring szClassName,             // Name of the class containing the method
    _In_ std::wstring szMethodName,            // Name of the method
    _In_ std::wstring szAppDomainName,         // Name of the Appdomain to be created 
    _Out_ void **pfnDelegate              // Output Managed function pointer
    );
