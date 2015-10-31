#pragma once

typedef int(__stdcall *Fn_coreclr_initialize)(
	const char* exePath,
	const char* appDomainFriendlyName,
	int propertyCount,
	const char** propertyKeys,
	const char** propertyValues,
	void** hostHandle,
	unsigned int* domainId);

typedef int(__stdcall *Fn_coreclr_create_delegate)(
	void* hostHandle,
	unsigned int domainId,
	const char* entryPointAssemblyName,
	const char* entryPointTypeName,
	const char* entryPointMethodName,
	void** delegate);
