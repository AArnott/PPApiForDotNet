// CoreCLRHost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "C:\git\PPApiForDotNet\src\PPApiForDotNet\CoreCLRHost.h"

int main()
{
    HRESULT hr = StartClrHost();
    if (FAILED(hr)) {
        return 1;
    }

    return 0;
}

