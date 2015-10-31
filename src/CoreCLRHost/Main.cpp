// CoreCLRHost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "C:\git\PPApiForDotNet\src\PPApiForDotNet\CoreCLRHost.h"

typedef int(__stdcall *Fn_entrypoint)(
    const char* arg);

int main()
{
    HRESULT hr = StartClrHost();
    if (FAILED(hr)) return 1;
    Fn_entrypoint del;
    hr = CreateManagedDelegate(
        L"PPApiInCSharp, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null",
        L"Program",
        L"Main",
        (void**)&del);
    if (FAILED(hr)) return 2;
    int result = del("Hello world!");

    return 0;
}

