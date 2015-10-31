#pragma once

#include <filesystem>

HRESULT StartClrHost();

HRESULT CreateManagedDelegate(
    _In_ const std::wstring& szAssemblyName,          // Name of the assembly containing the method
    _In_ const std::wstring& szClassName,             // Name of the class containing the method
    _In_ const std::wstring& szMethodName,            // Name of the method
    _Out_ void **pfnDelegate              // Output Managed function pointer
    );
