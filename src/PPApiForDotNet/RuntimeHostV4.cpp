/****************************** Module Header ******************************\
 * Module Name:  RuntimeHostV4.cpp
 * Project:      CppHostCLR
 * Copyright (c) Microsoft Corporation.
 * 
 * The code in this file demonstrates using .NET Framework 4.0 Hosting 
 * Interfaces (http://msdn.microsoft.com/en-us/library/dd380851.aspx) to host 
 * .NET runtime 4.0, load a .NET assebmly, and invoke a type in the assembly.
 * 
 * This source is subject to the Microsoft Public License.
 * See http://www.microsoft.com/en-us/openness/licenses.aspx#MPL.
 * All other rights reserved.
 * 
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma region Includes and Imports
#include "Stdafx.h"
#include <windows.h>

#include <metahost.h>
#pragma comment(lib, "mscoree.lib")

// Import mscorlib.tlb (Microsoft Common Language Runtime Class Library).
#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;
#pragma endregion

//
//   FUNCTION: RuntimeHostV4Demo2(PCWSTR, PCWSTR)
//
//   PURPOSE: The function demonstrates using .NET Framework 4.0 Hosting 
//   Interfaces to host a .NET runtime, and use the ICLRRuntimeHost interface
//   that was provided in .NET v2.0 to load a .NET assembly and invoke its 
//   type. Because ICLRRuntimeHost is not compatible with .NET runtime v1.x, 
//   the requested runtime must not be v1.x.
//   
//   If the .NET runtime specified by the pszVersion parameter cannot be 
//   loaded into the current process, the function prints ".NET runtime <the 
//   runtime version> cannot be loaded", and return.
//   
//   If the .NET runtime is successfully loaded, the function loads the 
//   assembly identified by the pszAssemblyName parameter. Next, the function 
//   invokes the public static function 'int GetStringLength(string str)' of 
//   the class and print the result.
//
//   PARAMETERS:
//   * pszAssemblyPath - The path to the Assembly to be loaded.
//   * pszClassName - The name of the Type that defines the method to invoke.
//   * pszStaticMethodName - The name of the static method in the .NET class to invoke.
//   * pszStringArg - The argument to pass to the static method.
//
//   RETURN VALUE: HRESULT of the demo.
//
HRESULT RuntimeHostV4Demo2(
	PCWSTR pszAssemblyPath, 
    PCWSTR pszClassName,
	PCWSTR pszStaticMethodName,
	PCWSTR pszStringArg,
	PDWORD pdwResult)
{
	PCWSTR pszVersion = L"v4.0.30319";
    HRESULT hr;

    ICLRMetaHost *pMetaHost = NULL;
    ICLRRuntimeInfo *pRuntimeInfo = NULL;

    // ICorRuntimeHost and ICLRRuntimeHost are the two CLR hosting interfaces
    // supported by CLR 4.0. Here we demo the ICLRRuntimeHost interface that 
    // was provided in .NET v2.0 to support CLR 2.0 new features. 
    // ICLRRuntimeHost does not support loading the .NET v1.x runtimes.
    ICLRRuntimeHost *pClrRuntimeHost = NULL;

    // 
    // Load and start the .NET runtime.
    // 

    wprintf(L"Load and start the .NET runtime %s \n", pszVersion);

    hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&pMetaHost));
    if (FAILED(hr))
    {
        wprintf(L"CLRCreateInstance failed w/hr 0x%08lx\n", hr);
        goto Cleanup;
    }

    // Get the ICLRRuntimeInfo corresponding to a particular CLR version. It 
    // supersedes CorBindToRuntimeEx with STARTUP_LOADER_SAFEMODE.
    hr = pMetaHost->GetRuntime(pszVersion, IID_PPV_ARGS(&pRuntimeInfo));
    if (FAILED(hr))
    {
        wprintf(L"ICLRMetaHost::GetRuntime failed w/hr 0x%08lx\n", hr);
        goto Cleanup;
    }

    // Check if the specified runtime can be loaded into the process. This 
    // method will take into account other runtimes that may already be 
    // loaded into the process and set pbLoadable to TRUE if this runtime can 
    // be loaded in an in-process side-by-side fashion. 
    BOOL fLoadable;
    hr = pRuntimeInfo->IsLoadable(&fLoadable);
    if (FAILED(hr))
    {
        wprintf(L"ICLRRuntimeInfo::IsLoadable failed w/hr 0x%08lx\n", hr);
        goto Cleanup;
    }

    if (!fLoadable)
    {
        wprintf(L".NET runtime %s cannot be loaded\n", pszVersion);
        goto Cleanup;
    }

    // Load the CLR into the current process and return a runtime interface 
    // pointer. ICorRuntimeHost and ICLRRuntimeHost are the two CLR hosting  
    // interfaces supported by CLR 4.0. Here we demo the ICLRRuntimeHost 
    // interface that was provided in .NET v2.0 to support CLR 2.0 new 
    // features. ICLRRuntimeHost does not support loading the .NET v1.x 
    // runtimes.
    hr = pRuntimeInfo->GetInterface(CLSID_CLRRuntimeHost, 
        IID_PPV_ARGS(&pClrRuntimeHost));
    if (FAILED(hr))
    {
        wprintf(L"ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
        goto Cleanup;
    }

    // Start the CLR.
    hr = pClrRuntimeHost->Start();
    if (FAILED(hr))
    {
        wprintf(L"CLR failed to start w/hr 0x%08lx\n", hr);
        goto Cleanup;
    }

    // 
    // Load the NET assembly and call the static method GetStringLength of 
    // the type CSSimpleObject in the assembly.
    // 

    wprintf(L"Load the assembly %s\n", pszAssemblyPath);

    // The invoked method of ExecuteInDefaultAppDomain must have the 
    // following signature: static int pwzMethodName (String pwzArgument)
    // where pwzMethodName represents the name of the invoked method, and 
    // pwzArgument represents the string value passed as a parameter to that 
    // method. If the HRESULT return value of ExecuteInDefaultAppDomain is 
    // set to S_OK, pReturnValue is set to the integer value returned by the 
    // invoked method. Otherwise, pReturnValue is not set.
    hr = pClrRuntimeHost->ExecuteInDefaultAppDomain(pszAssemblyPath, 
        pszClassName, pszStaticMethodName, pszStringArg, pdwResult);
    if (FAILED(hr))
    {
        wprintf(L"Failed to call %s.%s with HR 0x%08lx\n", pszClassName, pszStaticMethodName, hr);
        goto Cleanup;
    }

    // Print the call result of the static method.
    wprintf(L"Call %s.%s(\"%s\") => %d\n", pszClassName, pszStaticMethodName, 
        pszStringArg, *pdwResult);

Cleanup:

    if (pMetaHost)
    {
        pMetaHost->Release();
        pMetaHost = NULL;
    }
    if (pRuntimeInfo)
    {
        pRuntimeInfo->Release();
        pRuntimeInfo = NULL;
    }
    if (pClrRuntimeHost)
    {
        // Please note that after a call to Stop, the CLR cannot be 
        // reinitialized into the same process. This step is usually not 
        // necessary. You can leave the .NET runtime loaded in your process.
        //wprintf(L"Stop the .NET runtime\n");
        //pClrRuntimeHost->Stop();

        pClrRuntimeHost->Release();
        pClrRuntimeHost = NULL;
    }

    return hr;
}
