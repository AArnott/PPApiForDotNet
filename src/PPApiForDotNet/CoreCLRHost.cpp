#include "Stdafx.h"
#include <stdlib.h>
#include <atlstr.h>
#include "CoreCLR_unixinterface.h"

#include <memory>
#include <filesystem>
#include "CoreClrHost.h"

using namespace std;
using namespace std::experimental::filesystem::v1;

// The name of the CoreCLR native runtime DLL.
#ifdef __unix__
#include <linux/limits.h>
#include <unistd.h>
static const WCHAR *coreCLRDll = _T("libcoreclr.so");
extern LPCWSTR APPLICATION_NAME;
#elif defined(__APPLE__)
static const WCHAR *coreCLRDll = _T("libcoreclr.dylib");
extern LPCWSTR APPLICATION_NAME;
#else
static const path coreCLRDll = _T("coreclr.dll");
static const wchar_t PATH_LIST_SEPARATOR_STR = L';';
#endif

static bool s_clrStartAttempted = false;
static void* s_clrHostHandle = nullptr;
static unsigned int s_clrDomainId = 0;
static HMODULE s_coreclrModule = nullptr;

// Encapsulates the environment that CoreCLR will run in, including the TPALIST (trusted platform assembly list)
class HostEnvironment
{
private:
    // The list of paths to the assemblies that will be trusted by CoreCLR.
    // This is delimited by PATH_LIST_SEPARATOR_STR
    std::wstring m_tpaList;
    path p;

    // Attempts to load CoreCLR.dll from the given directory.
    // On success pins the dll, sets m_coreCLRDirectoryPath and returns the HMODULE.
    // On failure returns nullptr.
    HMODULE TryLoadCoreCLR(const path &clrDir)
    {
        path coreClrPath = clrDir / coreCLRDll;

        printf("Attempting to load coreclr\n");

        HMODULE hModule = ::LoadLibraryEx(coreClrPath.c_str(), nullptr, 0);
        if (!hModule) {
            printf("Failed to load: Error code : %d\n", GetLastError());
            return nullptr;
        }

        printf("Loaded\n");

        return hModule;
    }

public:
    // The path to the directory that CoreCLR is in
    path m_coreCLRDirectoryPath;

    static path GetModuleDir()
    {
        // Discover the path to this exe's module. All other files are expected to be in the same directory.
        wchar_t thisModulePath[MAX_PATH];
        DWORD thisModuleLength = ::GetModuleFileNameW(::GetModuleHandleW(L"PPApiForDotNet"), thisModulePath, MAX_PATH);
        path modulePath(thisModulePath);
        path moduleDir = modulePath.remove_filename();
        return moduleDir;
    }

    HostEnvironment()
    {
        // Check for %CORE_ROOT% and try to load CoreCLR.dll from it if it is set
        s_coreclrModule = nullptr; // Initialize this here since we don't call TryLoadCoreCLR if CORE_ROOT is unset.

        // Try to load CoreCLR from the directory that coreRun is in
        if (s_coreclrModule == nullptr)
        {
            path appDir = GetModuleDir();

            s_coreclrModule = TryLoadCoreCLR(appDir);
            if (s_coreclrModule)
            {
                m_coreCLRDirectoryPath = appDir;
            }
        }
    }

    ~HostEnvironment() {
    }

    bool TPAListContainsFile(const wstring& fileNameWithoutExtension, const list<wstring>& rgTPAExtensions)
    {
        if (!m_tpaList.size())
        {
            return false;
        }

        for (auto& ext : rgTPAExtensions)
        {
            wstring fileName;
            fileName += path::preferred_separator;
            fileName += fileNameWithoutExtension;
            fileName += ext;
            fileName += PATH_LIST_SEPARATOR_STR;

            if (m_tpaList.find(fileName, 0) != string::npos)
            {
                return true;
            }
        }

        return false;
    }

    void AddFilesFromDirectoryToTPAList(const path& targetPath, const list<wstring>& rgTPAExtensions)
    {
        for (auto& p : directory_iterator(targetPath))
        {
            for (auto& ext : rgTPAExtensions)
            {
                if (p.path().extension().compare(ext) == 0)
                {
                    // Add to the list if not already on it
                    path fileNameWithoutExtension = p.path().filename();
                    fileNameWithoutExtension.replace_extension(L"");
                    if (!TPAListContainsFile(fileNameWithoutExtension, rgTPAExtensions))
                    {
                        wprintf(L"%s added to the TPA list\n", p.path().filename().c_str());
                        m_tpaList.append(p.path());
                        m_tpaList.append(&PATH_LIST_SEPARATOR_STR, 1);
                    }
                    else
                    {
                        wprintf(L"%s not added to the TPA list because another file with the same name is already present on the list\n", p.path().filename().c_str());
                    }
                }
            }
        }
    }

    // Returns the semicolon-separated list of paths to runtime dlls that are considered trusted.
    // On first call, scans the coreclr directory for dlls and adds them all to the list.
    const wstring GetTpaList() {
        if (!m_tpaList.length()) {
            list<wstring> rgTPAExtensions;
            rgTPAExtensions.push_back(L".ni.dll");// Probe for .ni.dll first so that it's preferred if ni and il coexist in the same dir
            rgTPAExtensions.push_back(L".dll");
            rgTPAExtensions.push_back(L".ni.exe");
            rgTPAExtensions.push_back(L".exe");
            rgTPAExtensions.push_back(L".ni.winmd");
            rgTPAExtensions.push_back(L".winmd");

            AddFilesFromDirectoryToTPAList(m_coreCLRDirectoryPath, rgTPAExtensions);
        }

        return m_tpaList;
    }
};

HRESULT StartClrHost()
{
    HostEnvironment hostEnvironment;

    //-------------------------------------------------------------

    path appDir(HostEnvironment::GetModuleDir());
    path appLocalWinmetadata = appDir / _T("WinMetadata");

    path appNiPath(appDir);
    appNiPath /= _T("NI");

    // Construct native search directory paths
    wstring nativeDllSearchDirs(appDir);

    nativeDllSearchDirs.append(&PATH_LIST_SEPARATOR_STR, 1);
    nativeDllSearchDirs.append(hostEnvironment.m_coreCLRDirectoryPath);

    //-------------------------------------------------------------

    // Allowed property names:
    // APPBASE
    // - The base path of the application from which the exe and other assemblies will be loaded
    //
    // TRUSTED_PLATFORM_ASSEMBLIES
    // - The list of complete paths to each of the fully trusted assemblies
    //
    // APP_PATHS
    // - The list of paths which will be probed by the assembly loader
    //
    // APP_NI_PATHS
    // - The list of additional paths that the assembly loader will probe for ngen images
    //
    // NATIVE_DLL_SEARCH_DIRECTORIES
    // - The list of paths that will be probed for native DLLs called by PInvoke
    //
    const char *property_keys[] = {
        "TRUSTED_PLATFORM_ASSEMBLIES",
        "APP_PATHS",
        "APP_NI_PATHS",
        "NATIVE_DLL_SEARCH_DIRECTORIES",
        "AppDomainCompatSwitch",
        "APP_LOCAL_WINMETADATA"
    };

    CW2A tpaList(hostEnvironment.GetTpaList().c_str(), CP_UTF8);
    CW2A nativeDllSearchDirsA(nativeDllSearchDirs.c_str(), CP_UTF8);
    string pzAppDir = appDir.string();
    string pzAppNiPath = appNiPath.string();
    string pzAppLocalWinmetadata = appLocalWinmetadata.u8string();

    const char *property_values[] = {
        // TRUSTED_PLATFORM_ASSEMBLIES
        tpaList,
        // APP_PATHS
        pzAppDir.c_str(),
        // APP_NI_PATHS
        pzAppNiPath.c_str(),
        // NATIVE_DLL_SEARCH_DIRECTORIES
        nativeDllSearchDirsA,
        // AppDomainCompatSwitch
        "UseLatestBehaviorWhenTFMNotSpecified",
        // APP_LOCAL_WINMETADATA
        pzAppLocalWinmetadata.c_str(),
    };

    for (int idx = 0; idx < sizeof(property_keys) / sizeof(char*); idx++)
    {
        printf("%s = %s\n", property_keys[idx], property_values[idx]);
    }

    Fn_coreclr_initialize coreclr_initialize = (Fn_coreclr_initialize)::GetProcAddress(s_coreclrModule, "coreclr_initialize");
    if (coreclr_initialize == nullptr)
    {
        printf("Failed to get coreclr_initialize\n");
        return E_FAIL;
    }

    if (coreclr_initialize(appDir.u8string().c_str(), "PPAPIForDotNet", sizeof(property_keys) / sizeof(char*), property_keys, property_values, &s_clrHostHandle, &s_clrDomainId))
    {
        printf("Failed to initialize coreclr\n");
        return E_FAIL;
    }

    printf("CoreClr initialized\n");
    return S_OK;
}

HRESULT CreateManagedDelegate(
    _In_ wstring szAssemblyName,          // Name of the assembly containing the method
    _In_ wstring szClassName,             // Name of the class containing the method
    _In_ wstring szMethodName,            // Name of the method
    _Out_ void **pfnDelegate              // Output Managed function pointer
    )
{
    if (!s_clrStartAttempted)
    {
        return E_UNEXPECTED;
    }

    CW2A assemblyName(szAssemblyName.c_str(), CP_UTF8);
    CW2A className(szClassName.c_str(), CP_UTF8);
    CW2A methodName(szMethodName.c_str(), CP_UTF8);

    Fn_coreclr_create_delegate coreclr_create_delegate = (Fn_coreclr_create_delegate)::GetProcAddress(s_coreclrModule, "coreclr_create_delegate");
    if (coreclr_create_delegate == nullptr)
    {
        printf("Failed to get coreclr_initialize\n");
        return E_FAIL;
    }

    if (coreclr_create_delegate(s_clrHostHandle, s_clrDomainId, assemblyName, className, methodName, pfnDelegate))
    {
        printf("Failed call to coreclr_create_delegate\n");
        return E_FAIL;
    }

    return S_OK;
}
