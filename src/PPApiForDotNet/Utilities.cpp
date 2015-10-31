#include "Stdafx.h"
#include <stdlib.h>
#include "Utilities.h"

using namespace std::experimental::filesystem::v1;

path GetModuleDir()
{
    // Discover the path to this exe's module. All other files are expected to be in the same directory.
    wchar_t thisModulePath[MAX_PATH];
    DWORD thisModuleLength = ::GetModuleFileNameW(::GetModuleHandleW(L"PPApiForDotNet"), thisModulePath, MAX_PATH);
    path modulePath(thisModulePath);
    path moduleDir = modulePath.remove_filename();
    return moduleDir;
}
