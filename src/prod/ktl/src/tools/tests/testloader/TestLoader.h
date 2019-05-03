#pragma once

#ifdef PLATFORM_UNIX

#include <ktlpal.h>
#include <strsafe.h>
#include "../../inc/KmUnitDefs.h"

#else // !PLATFORM_UNIX

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

typedef unsigned long ULONG;

#include "WexTestClass.h"
#include "WexString.h"
#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wdfinstaller.h>

#include "Installer.h"

#include "../../inc/KmUnitDefs.h"

#define MAX_VERSION_SIZE        (6)


#define CONVERT_VERSION_TO_STRING_WORKER(major, minor)  #major ## #minor
#define CONVERT_VERSION_TO_STRING(major, minor)         CONVERT_VERSION_TO_STRING_WORKER(major, minor)


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)       (sizeof(array) / sizeof(array[0]))
#endif

#ifndef WIN32_FROM_HRESULT
#define WIN32_FROM_HRESULT(hr)           \
    (SUCCEEDED(hr) ? ERROR_SUCCESS :    \
        (HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr)))
#endif

using namespace WEX::Common;

#endif // !PLATFORM_UNIX

#ifndef PLATFORM_UNIX
class TestLoader : public WEX::TestClass<TestLoader>
{

//
// Settings of the tests.
//

public:
	TestLoader ()
	{
	}

	~TestLoader ()
	{
	}

	//
	// Helper functions
	//
	void PrintTestArguments(const TEST_PARAMS& parameters);
	void PrintTestCases(const ULONG& numberOfTests, PKU_TEST_ID_ARRAY testIdArray, const String& label);
	void PrintTestStart(const KU_TEST_ID& test);
	DWORD PrintTestResult(const DWORD& error, const KU_TEST_ID& test);
	BOOLEAN ContainsString(const String& stringsToLookFor, const String& stringToLookIn);
	LONGLONG SpawnCommandLine(WCHAR* CmdLine);

	//
	// KERNEL helper functions
	//
	DWORD ListTests(__in HANDLE DeviceHandle);
	DWORD RunSingleTest(__in HANDLE DeviceHandle, __in ULONG TestId, __in BOOLEAN BreakOnStart, __in_bcount_opt(TestParameterBytes) PBYTE TestParameter, __in ULONG TestParameterBytes, __in BOOL Verbose);
	DWORD RunSingleKernelTest(__in HANDLE DeviceHandle, const String& ids, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters);
	DWORD RunAllTests(__in HANDLE DeviceHandle, const String& categories, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters);

	//
	// Execution functions
	//
	DWORD ExecuteSingleUserTest(const String& ids, const String& name, TEST_PARAMS_ARRAY& parameters);
	DWORD ExecuteSingleKernelTest(const String& ids, BOOLEAN BreakOnStart, const String& driverName, TEST_PARAMS_ARRAY& parameters);
	DWORD ExecuteAllUserTests(const String& name, const String& categories, TEST_PARAMS_ARRAY& parameters);
	DWORD ExecuteAllKernelTests(const String& driverName, const String& categories, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters);
	void ListAvailableKernelTests(const String& driverName);
	void ListAvailableUserTests(const String& name);

	//
	// Usage functions
	//
	void ListAvailableTests();
	void ExecuteTests(TEST_PARAMS_ARRAY& parameters);
	void ExecuteSingleTest(TEST_PARAMS_ARRAY& parameters);
	void Usage();

	TEST_CLASS(TestLoader)
	BEGIN_TEST_METHOD(Execute)
		TEST_METHOD_PROPERTY(L"DataSource", L"p:ConfigFile")
	END_TEST_METHOD()
};
#endif

