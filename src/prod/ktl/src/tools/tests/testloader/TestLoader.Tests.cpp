#ifdef PLATFORM_UNIX

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestLoader
#include <boost/test/unit_test.hpp>
#include "TestLoader.h"
#include <codecvt>
#include <dlfcn.h>
#include <map>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <iostream>

#define FIELD_OFFSET(type, field)  ((LONG)(LONG_PTR)&(((type *)0)->field))

#define String std::string

#else // !PLATFORM_UNIX

#include <specstrings.h>
#include "TestLoader.h"
#include "WexString.h"
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <process.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace WEX::Logging;

#endif // !PLATFORM_UNIX

using namespace std;

#ifndef PLATFORM_UNIX
//
// Enum of command IDs for this control application.
//

enum CmdId {
    CmdList,
    CmdRunSingleTest,
    CmdRunAllTests
};

//
// Test Settings definition
//
typedef struct _TEST_SETTINGS {
    WCHAR       CoInstallerVersion[MAX_VERSION_SIZE];
    PCWSTR      DriverName;
    BOOL        LoopTest;
    CmdId       CommandId;
    ULONG       TestCaseId;
} TEST_SETTINGS, *PTEST_SETTINGS;

//
// Default Test Settings
//
static const TEST_SETTINGS gs_defaultSettings =
{
    L"01009",   // The current WDF version
    L"KTLTests",
    FALSE,
    CmdList,
    (ULONG)-1
};

//
// Bit flag indicating the target modes to execute tests under.
//
enum TargetMode
{
    None = 0x00,
    User = 0x01,            // User-mode unit test DLL
    Kernel = 0x02,          // Kernel unit test driver
    Both = (User | Kernel),
};
#endif // !PLATFORM_UNIX

//
// Functions that will be executed from user mode dll
//
typedef DWORD (__cdecl * QueryNumberOfTests)(ULONG&);
typedef DWORD (__cdecl * QueryTests) (PKU_TEST_ID_ARRAY testIdArray, ULONG& testIdArraySize);
typedef DWORD (__cdecl * RunTest) (PKU_TEST_ID testId, PVOID testParameters, ULONG& testParametersSize, int argc, WCHAR *args[]);

//
// Helper functions
//

#ifdef PLATFORM_UNIX
//
// Convert wchar_t* to string
//
static string utf16to8(const wchar_t *wstr)
{
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.to_bytes((const char16_t *) wstr);
}


static std::wstring Utf8To16(const char *str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    std::u16string u16str = conv.from_bytes(str);
    std::basic_string<wchar_t> result;
    for(int index = 0; index < u16str.length(); index++)
    {
        result.push_back((wchar_t)u16str[index]);
    }
    return result;
}

#endif

//
// Print function for test arguments
//
void
#ifndef PLATFORM_UNIX
TestLoader::
#endif
PrintTestArguments(const TEST_PARAMS& parameters)
{
#ifdef PLATFORM_UNIX
    BOOST_TEST_MESSAGE("        argc = " << parameters.Count);
#else
    Log::Comment(String().Format(L"        argc = %d", parameters.Count));
#endif
    for (int i = 0; i < parameters.Count; i++)
    {
#ifdef PLATFORM_UNIX
        BOOST_TEST_MESSAGE("        args[" << i << "]: " << utf16to8(parameters.Parameter[i]));
#else
        Log::Comment(String().Format(L"        args[%d]: %s", i, parameters.Parameter[i]));
#endif
    }
}

#ifndef PLATFORM_UNIX
//
// Print function for test cases
//
void TestLoader::PrintTestCases(const ULONG& numberOfTests, PKU_TEST_ID_ARRAY testIdArray, const String& label)
{
    Log::Comment(label);
    for (ULONG i = 0; i < numberOfTests; i++)
    {
        Log::Comment(String().Format(L"Test %d: ", testIdArray->TestIdArray[i].Id) + testIdArray->TestIdArray[i].Name +
            L"\n    Categories: " + testIdArray->TestIdArray[i].Categories +
            L"\n    Help: " + testIdArray->TestIdArray[i].Help);
    }
}
#endif

//
// Print function for test start signalling
//
void
#ifndef PLATFORM_UNIX
TestLoader::
#endif
PrintTestStart(const KU_TEST_ID& test)
{
    #ifdef PLATFORM_UNIX
    BOOST_TEST_MESSAGE("Test[" << test.Id << "] " << utf16to8(test.Name) << "...");
    BOOST_TEST_MESSAGE("    Starting Test ["<< test.Id << "]:" << utf16to8(test.Name));
    #else
    Log::Comment(String().Format(L"Test[%d] ", test.Id) + test.Name + L"...");
    Log::Comment(String().Format(L"    Starting Test [%d]:", test.Id) + test.Name);
    #endif
}

//
// Print function for test results after execution
//
DWORD
#ifndef PLATFORM_UNIX
TestLoader::
#endif
PrintTestResult(const DWORD& error, const KU_TEST_ID& test)
{
    if (error != ERROR_SUCCESS)
    {
        #ifdef PLATFORM_UNIX
        BOOST_TEST_MESSAGE("    Completing Test " << test.Id << ": " << utf16to8(test.Name) << ". Status = FAILURE (" << error << ")");
        #else
        Log::Error(String().Format(L"    Completing Test %d: ", test.Id) +
                                   test.Name +
                                   String().Format(L". Status = FAILURE (%d)", error));
        #endif
        return error;
    }
    else
    {
        #ifdef PLATFORM_UNIX
        BOOST_TEST_MESSAGE("    Completing Test " << test.Id << ": " << utf16to8(test.Name) << ". Status = SUCCESS");
        #else
        Log::Comment(String().Format(L"    Completing Test %d: ", test.Id) +
                                    test.Name +
                                    ". Status = SUCCESS");
        #endif
    }
    return ERROR_SUCCESS;
}

//
// Funtion that tries to match tokens in one string against tokens
// in a different string
//
BOOLEAN
#ifndef PLATFORM_UNIX
TestLoader::
#endif
ContainsString(const String& stringsToLookFor, const String& stringToLookIn)
{
    BOOLEAN exists = TRUE;
    #ifdef PLATFORM_UNIX
    if (!stringsToLookFor.empty())
    #else
    if (!WEX::Common::String::IsNullOrEmpty(stringsToLookFor))
    #endif
    {
        exists = FALSE;
        BOOLEAN searching = TRUE;
        String tempCategory = stringsToLookFor;

        // search until out of string
        while (searching)
        {
            String category;
            #ifdef PLATFORM_UNIX
            int comma = tempCategory.find(',', 0);
            #else
            int comma = tempCategory.Find(L',', 0);
            #endif

            // found a comma
            if (comma >= 0)
            {
                #ifdef PLATFORM_UNIX
                category.assign(tempCategory.c_str(), comma);
                tempCategory.erase(0, category.length() + 1);
                #else
                category.Copy(tempCategory, comma);
                tempCategory.Delete(0, category.GetLength() + 1);
                #endif
            }
            else
            {
                category = tempCategory;
                searching = FALSE;
            }

            // i know have the token
            BOOLEAN internalSearch = TRUE;
            String testCategories = stringToLookIn;
            while (internalSearch)
            {
                String internalCategory;
                #ifdef PLATFORM_UNIX
                int internalComma = testCategories.find(',', 0);
                #else
                int internalComma = testCategories.Find(L',', 0);
                #endif
                if (internalComma >= 0)
                {
                    #ifdef PLATFORM_UNIX
                    internalCategory.assign(testCategories.c_str(), internalComma);
                    testCategories.erase(0, internalCategory.length()+1);
                    #else
                    internalCategory.Copy(testCategories, internalComma);
                    testCategories.Delete(0, internalCategory.GetLength()+1);
                    #endif
                }
                else
                {
                    internalCategory = testCategories;
                    internalSearch = FALSE;
                }

                // i have internal token
                #ifdef PLATFORM_UNIX
                if (!category.compare(internalCategory))
                #else
                if (!category.CompareNoCase(internalCategory))
                #endif
                {
                    searching = FALSE;
                    internalSearch = FALSE;
                    exists = TRUE;
                    break;
                }
            }
        }
    }

    return exists;
}


#ifndef PLATFORM_UNIX
//
// This function will print the appropriate success or failure message
//
void ProcessErrors(const DWORD& userMode, const DWORD& kernelMode)
{
    Log::Comment(L"");
    if (ERROR_SUCCESS!=userMode)
    {
        Log::Error(L"Error(s) in user mode");
    }
    if (ERROR_SUCCESS!=kernelMode)
    {
        Log::Error(L"Error(s) in kernel mode");
    }
    Log::Comment(L"");
}
#endif

#ifndef PLATFORM_UNIX
//
// Funtion that determines the execution mode of the current run
// it will look for the parameter target and set a value to execute
// in KERNEL, USER or BOTH
//
static TargetMode GetTargetMode()
{
    String value;
    RuntimeParameters::TryGetValue(L"Target", value);
    if (value.CompareNoCase(L"User") == 0)
    {
        return TargetMode::User;
    }
    else if (value.CompareNoCase(L"Kernel") == 0)
    {
        return TargetMode::Kernel;
    }
    else if (value.CompareNoCase(L"Both") == 0)
    {
        Log::Comment(L"Executing available USER and KERNEL mode tests.");
        Log::Comment(L"");
        return TargetMode::Both;
    }
    else
    {
        if (!WEX::Common::String::IsNullOrEmpty(value))
        {
            Log::Warning(L"Invalid parameter. Valid values 'User' for user, 'Kernel' for kernel, 'Both' for both.");
            Log::Warning(L"If no parameter is passed will execute both user and kernel mode tests.");
            return TargetMode::None;
        }
        else
        {
            Log::Comment(L"Executing both USER and KERNEL mode tests.");
            return TargetMode::Both;
        }
    }
}
#endif

#ifndef PLATFORM_UNIX
// Parse, spawn, and return completion result from the command line passed in CmdLine
LONGLONG TestLoader::SpawnCommandLine(WCHAR* CmdLine)
{
    WCHAR       cmdLineCopy[KU_MAX_POST_OP_LENGTH];
    WCHAR*      tokens[(KU_MAX_POST_OP_LENGTH / 2) + 1];
    ULONG       numberOfTokens = 0;
    WCHAR*      currentToken;
    WCHAR*      context;

    if ((wcslen(CmdLine) + 1) > (sizeof(cmdLineCopy) / sizeof(WCHAR)))
    {
        Log::Error(L"SpawnCommandLine: CmdLine too long");
        return -1;
    }
    wcscpy_s(cmdLineCopy, sizeof(cmdLineCopy), CmdLine);

    currentToken = wcstok_s(&cmdLineCopy[0], L" \t", &context);
    while (currentToken != NULL)
    {
        if (numberOfTokens == (KU_MAX_POST_OP_LENGTH / 2))
        {
            Log::Error(L"SpawnCommandLine: too many tokens");
            return -1;
        }

        tokens[numberOfTokens] = currentToken;
        numberOfTokens++;

        currentToken = wcstok_s(NULL, L" \t", &context);
    }

    if (numberOfTokens == 0)
    {
        Log::Error(L"SpawnCommandLine: no tokens found");
        return -1;
    }

    tokens[numberOfTokens] = NULL;

    intptr_t spawnStatus = _wspawnv(_P_WAIT, tokens[0], tokens);

    if (spawnStatus == -1)
    {
        Log::Error(L"SpawnCommandLine: _wspawnv failed");
        return -1;
    }

    if (spawnStatus != 0)
    {
        Log::Error(L"SpawnCommandLine: spawned command failed");
    }

    return spawnStatus;
}
#endif


#ifndef PLATFORM_UNIX
//
// KERNEL mode helper functions
//
DWORD TestLoader::ListTests(__in HANDLE DeviceHandle)
/*++

Routine Description:

    Handler for CmdList.

Arguments:

    DeviceHandle - Supplies a handle to the control device.

Return Value:

    Win32 error code.

--*/
{
    BOOL                    b;
    ULONG                   numberOfTests;
    PKU_TEST_ID_ARRAY       testIdArray = NULL;
    ULONG                   testIdArraySize;
    DWORD                   bytesReturned;

    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS,
            NULL, 0, &numberOfTests, sizeof(numberOfTests), &bytesReturned, NULL);

    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query number of teststests.");
        return GetLastError();
    }
    testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
    testIdArray = (PKU_TEST_ID_ARRAY) malloc(testIdArraySize);

    if (!testIdArray)
    {
        Log::Error(L"Unable to allocate memory for TestIdArray.");
        return GetLastError();
    }

    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_TESTS,
            NULL, 0, testIdArray, testIdArraySize, &bytesReturned, NULL);
    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query tests.");
        free(testIdArray);
        return GetLastError();
    }
    PrintTestCases(numberOfTests, testIdArray, L"Available Kernel mode tests:");
    free(testIdArray);
    return ERROR_SUCCESS;
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::RunSingleTest(__in HANDLE DeviceHandle, __in ULONG TestId, __in BOOLEAN BreakOnStart, __in_bcount_opt(TestParameterBytes) PBYTE TestParameter, __in ULONG TestParameterBytes, __in BOOL Verbose)

/*++

Routine Description:

    Handler for CmdRunSingleTest.

Arguments:

    DeviceHandle - Supplies a handle to the control device.

Return Value:

    Win32 error code.

--*/

{
    UNREFERENCED_PARAMETER(Verbose);

    DWORD               error = ERROR_SUCCESS;
    PKU_TEST_START      startCmd = NULL;
    DWORD               bytesReturned = 0;
    ULONG               bufferSize;

    bufferSize = FIELD_OFFSET(KU_TEST_START, Parameter) + TestParameterBytes;
    startCmd = (PKU_TEST_START)malloc(bufferSize);
    if (startCmd == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    RtlZeroMemory(startCmd, bufferSize);
    startCmd->TestId = TestId;
    startCmd->BreakOnStart = BreakOnStart;
    startCmd->ParameterBytes = TestParameterBytes;
    if (TestParameter)
    {
        if (memcpy_s(startCmd->Parameter, TestParameterBytes, TestParameter, TestParameterBytes) != 0)
        {
            Log::Error(L"memcpy_s failed");
            return ERROR_BAD_LENGTH;
        }
    }

    BOOL success = DeviceIoControl(
        DeviceHandle,
        (DWORD) IOCTL_KU_CONTROL_START_TEST,
        startCmd, bufferSize,
        NULL, 0,
        &bytesReturned,
        NULL);

    if (success != TRUE)
    {
        error = GetLastError();
        Log::Error(String().Format(L"DeviceIoControl failed: %d \n", error));
    }

    free(startCmd);
    return error;
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::RunSingleKernelTest(__in HANDLE DeviceHandle, const String& ids, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters)
{
    BOOL                b;
    DWORD               error = ERROR_SUCCESS;
    ULONG               numberOfTests;
    DWORD               bytesReturned;
    DWORD               result = ERROR_SUCCESS;
    PKU_TEST_ID_ARRAY   testIdArray = NULL;
    ULONG               testIdArraySize;

    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS,
            NULL, 0, &numberOfTests, sizeof(numberOfTests), &bytesReturned, NULL);
    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query number of tests");
        return GetLastError();
    }

    testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
    testIdArray = (PKU_TEST_ID_ARRAY)malloc(testIdArraySize);
    if (!testIdArray)
    {
        Log::Error(L"Unable to allocate memory for TestIdArray.");
        return GetLastError();
    }

    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_TESTS,
            NULL, 0, testIdArray, testIdArraySize, &bytesReturned, NULL);
    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query tests.");
        free(testIdArray);
        return GetLastError();
    }

    for (ULONG i = 0; i < numberOfTests; i++)
    {
        if (ContainsString(testIdArray->TestIdArray[i].Name, ids))
        {
            PrintTestStart(testIdArray->TestIdArray[i]);
            BOOLEAN ran = FALSE;
            for (ULONG k = 0; k < parameters.NumberOfTests; k++)
            {
                // if the ids match
                if (String().Format(testIdArray->TestIdArray[i].Name).CompareNoCase(parameters.TestParamsArray[k].Id) == 0)
                {
                    // run test
                    PrintTestArguments(parameters.TestParamsArray[k]);
                    error = RunSingleTest(DeviceHandle, testIdArray->TestIdArray[i].Id, BreakOnStart, (PBYTE)&(parameters.TestParamsArray[k]), sizeof(TEST_PARAMS), FALSE);
                    if ((error == ERROR_SUCCESS) && (parameters.TestParamsArray[k].PostOpCmd[0] != 0))
                    {
                        LONGLONG status = SpawnCommandLine(parameters.TestParamsArray[k].PostOpCmd);
                        if (status != 0)
                        {
                            Log::Error(L"    Post operation spawned command failed");
                            error = (DWORD)status;
                        }
                    }

                    result = PrintTestResult(error, testIdArray->TestIdArray[i]);
                    ran = TRUE;
                }
            }
            if (!ran)
            {
                Log::Comment(L"    Test did not run. No entry in configuration file found.");
            }
        }
    }

    // free the testidarray that was allocated
    free(testIdArray);
    // return agregated result
    return result;
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::RunAllTests(__in HANDLE DeviceHandle, const String& categories, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters)
/*++

Routine Description:

    Handler for CmdRunAllTests.

Arguments:

    DeviceHandle - Supplies a handle to the control device.

Return Value:

    Win32 error code.

--*/

{
    BOOL                b;
    DWORD               error = ERROR_SUCCESS;
    ULONG               numberOfTests;
    DWORD               bytesReturned;
    PKU_TEST_ID_ARRAY   testIdArray = NULL;
    ULONG               testIdArraySize;

    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS,
            NULL, 0, &numberOfTests, sizeof(numberOfTests), &bytesReturned, NULL);
    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query number of tests.");
        return GetLastError();
    }
    testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
    testIdArray = (PKU_TEST_ID_ARRAY)malloc(testIdArraySize);
    if (!testIdArray)
    {
        Log::Error(L"Unable to allocate memory for TestIdArray.");
        return GetLastError();
    }
    b = DeviceIoControl(DeviceHandle, (DWORD) IOCTL_KU_CONTROL_QUERY_TESTS,
            NULL, 0, testIdArray, testIdArraySize, &bytesReturned, NULL);
    if (!b)
    {
        Log::Error(L"DeviceIoControl failed for query tests");
        free(testIdArray);
        return GetLastError();
    }
    DWORD testPass = ERROR_SUCCESS;
    for (ULONG i = 0; i < numberOfTests; i++)
    {
        BOOLEAN run = TRUE;
        run = ContainsString(categories, testIdArray->TestIdArray[i].Categories);
        if (run)
        {
            PrintTestStart(testIdArray->TestIdArray[i]);

            BOOLEAN ran = FALSE;
            for (ULONG k = 0; k < parameters.NumberOfTests; k++)
            {
                // if the ids match
                if (String().Format(testIdArray->TestIdArray[i].Name).CompareNoCase(parameters.TestParamsArray[k].Id) == 0)
                {
                    // run test
                    PrintTestArguments(parameters.TestParamsArray[k]);
                    error = RunSingleTest(DeviceHandle, testIdArray->TestIdArray[i].Id, BreakOnStart,(PBYTE)&(parameters.TestParamsArray[k]), sizeof(TEST_PARAMS), FALSE);

                    if ((error == ERROR_SUCCESS) && (parameters.TestParamsArray[k].PostOpCmd[0] != 0))
                    {
                        LONGLONG status = SpawnCommandLine(parameters.TestParamsArray[k].PostOpCmd);
                        if (status != 0)
                        {
                            Log::Error(L"    Post operation spawned command failed");
                            error = (DWORD)status;
                        }
                    }

                    testPass = PrintTestResult(error, testIdArray->TestIdArray[i]);
                    ran = TRUE;
                }
            }

            if (!ran)
            {
                Log::Comment(L"    Test did not run. No entry in configuration file found.");
            }
        }
    }

    free(testIdArray);
    return testPass;
}
#endif

#ifndef PLATFORM_UNIX
HANDLE GetDeviceHandle(TEST_SETTINGS testSettings)
{
    HANDLE                  hDevice = INVALID_HANDLE_VALUE;
    DWORD                   error = ERROR_SUCCESS;
    HRESULT                 hr = S_OK;
    WCHAR                   infFileName[MAX_PATH] = {0};
    WCHAR                   wdfSectionName[MAX_PATH] = {0};
    WCHAR                   deviceObjectName[MAX_PATH] = {0};

    StringCchPrintfW(testSettings.CoInstallerVersion,
        ARRAYSIZE(testSettings.CoInstallerVersion), L"%.02d%.03d",
        KMDF_VERSION_MAJOR, KMDF_VERSION_MINOR);

    Log::Comment(String().Format(L"KMDF CoInstaller version: %s", testSettings.CoInstallerVersion));

    hr = StringCchPrintfW(infFileName, ARRAYSIZE(infFileName), L"%s.inf",
        testSettings.DriverName);
    VERIFY_SUCCEEDED(hr);

    hr = StringCchPrintfW(wdfSectionName, ARRAYSIZE(wdfSectionName), L"%s.NT.Wdf",
        testSettings.DriverName);
    VERIFY_SUCCEEDED(hr);

    //
    // Remove any existing driver because the existing driver
    // may be a stale binary from a different test build.
    // Ignore any error.
    //

    KmdfNonPnpInstaller installer(
        testSettings.CoInstallerVersion, testSettings.DriverName,
        infFileName, wdfSectionName, TRUE);
    installer.StopDriver();
    installer.RemoveDriver();
    installer.RemoveCertificate();

    //
    // Reinstall the test driver with the current binary.
    //

    error = installer.InstallCertificate();
    VERIFY_WIN32_SUCCEEDED(error);

    error = installer.InstallDriver();
    VERIFY_WIN32_SUCCEEDED(error);

    error = installer.StartDriver();
    VERIFY_WIN32_SUCCEEDED(error);

    //
    // Open the driver control device.
    //
    StringCchPrintfW(deviceObjectName,
        ARRAYSIZE(deviceObjectName), L"%ws%ws",
        KU_CONTROL_NAME, testSettings.DriverName);

    printf("Opening Device: %ws %s %ws %ws\n", deviceObjectName, TESTNAME, KU_NT_CONTROL_NAME, KU_DOS_CONTROL_NAME);
    hDevice = CreateFileW(deviceObjectName,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    VERIFY_ARE_NOT_EQUAL(INVALID_HANDLE_VALUE, hDevice);
    return hDevice;
}
#endif

#ifndef PLATFORM_UNIX
//
// KERNEL mode functions used by TAEF functions
//
void TestLoader::ListAvailableKernelTests(const String& driverName)
{
    //
    // List the tests.
    //
    TEST_SETTINGS testSettings = gs_defaultSettings;
    testSettings.DriverName = driverName;
    HANDLE hDevice = GetDeviceHandle(testSettings);
    ListTests(hDevice);
    CloseHandle(hDevice);
    hDevice = INVALID_HANDLE_VALUE;
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::ExecuteSingleKernelTest(const String& ids, BOOLEAN BreakOnStart, const String& driverName, TEST_PARAMS_ARRAY& parameters)
{
    //
    // Execute single KERNEL test
    //
    TEST_SETTINGS testSettings = gs_defaultSettings;
    testSettings.DriverName = driverName;
    testSettings.CommandId = CmdRunAllTests;
    HANDLE hDevice = GetDeviceHandle(testSettings);
    DWORD error = RunSingleKernelTest(hDevice, ids, BreakOnStart, parameters);
    CloseHandle(hDevice);
    hDevice = INVALID_HANDLE_VALUE;
    return error;
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::ExecuteAllKernelTests(const String& driverName, const String& categories, BOOLEAN BreakOnStart, TEST_PARAMS_ARRAY& parameters)
{
    //
    // Execute all KERNEL tests
    //
    TEST_SETTINGS testSettings = gs_defaultSettings;
    testSettings.DriverName = driverName;
    testSettings.CommandId = CmdRunAllTests;
    HANDLE hDevice = GetDeviceHandle(testSettings);
    DWORD error = RunAllTests(hDevice, categories, BreakOnStart, parameters);
    CloseHandle(hDevice);
    hDevice = INVALID_HANDLE_VALUE;
    return error;
}
#endif

#ifndef PLATFORM_UNIX
//
// USER mode functions used by TAEF functions
//
void TestLoader::ListAvailableUserTests(const String& name)
{
    HINSTANCE hinstLib;
    FARPROC ProcAdd = NULL;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
    ULONG numberOfTests = 0;
    hinstLib = LoadLibrary(String().Format(name) + L".dll");

    // check to see if the dll could be loaded
    if (hinstLib != NULL)
    {
        ProcAdd = GetProcAddress(hinstLib, "QueryNumberOfTests");

        // check to see if the funtion was located within the dll that was loaded
        if (NULL != ProcAdd)
        {
            QueryNumberOfTests MyFunction;
            MyFunction = QueryNumberOfTests(ProcAdd);
            fRunTimeLinkSuccess = TRUE;
            MyFunction(numberOfTests);
            ProcAdd = NULL;
            ProcAdd = GetProcAddress(hinstLib, "QueryTests");

            // check to see if the function was located within the dll that was loaded
            if (NULL != ProcAdd)
            {
                PKU_TEST_ID_ARRAY       testIdArray = NULL;
                ULONG                   testIdArraySize;
                testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
                testIdArray = (PKU_TEST_ID_ARRAY) malloc(testIdArraySize);

                // try to allocate memory for the number of tests that are present
                if (!testIdArray)
                {
                    Log::Error(L"ERROR allocating memory");
                }

                QueryTests MyFunctionQuery;
                MyFunctionQuery = QueryTests(ProcAdd);
                MyFunctionQuery(testIdArray, numberOfTests);
                // print tests available in user mode
                PrintTestCases(numberOfTests, testIdArray, L"Available User mode tests:");
                // free allocated testIdArray
                free(testIdArray);
            }
            else
            {
                // log error and bail querytest
                Log::Error(L"Function not present in library module");
            }
        }
        else
        {
            // log error and bail listtest
            Log::Error(L"Function not present in library module");
        }

        // free the library that was loaded
        fFreeResult = FreeLibrary(hinstLib);
    }
    else
    {
        // log error and exit
        Log::Error(L"Could not load library");
    }
}
#endif

#ifndef PLATFORM_UNIX
DWORD TestLoader::ExecuteSingleUserTest(const String& ids, const String& name, TEST_PARAMS_ARRAY& parameters)
{
    // initialize variable
    DWORD result = ERROR_SUCCESS;
    HINSTANCE hinstLib;
    FARPROC ProcAdd = NULL;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
    ULONG numberOfTests = 0;
    hinstLib = LoadLibrary(String().Format(name) + L".dll");

    // check to see if the library is present
    if (hinstLib != NULL)
    {
        ProcAdd = GetProcAddress(hinstLib, "QueryNumberOfTests");

        // check to see if the funtion was located
        if (NULL != ProcAdd)
        {
            QueryNumberOfTests MyFunction;
            MyFunction = QueryNumberOfTests(ProcAdd);
            fRunTimeLinkSuccess = TRUE;
            MyFunction(numberOfTests);
            PKU_TEST_ID_ARRAY       testIdArray = NULL;
            ULONG                   testIdArraySize;
            testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
            testIdArray = (PKU_TEST_ID_ARRAY) malloc(testIdArraySize);

            // try to allocate memory for the test cases
            if (!testIdArray)
            {
                Log::Warning(L"ERROR allocating memory");
            }

            ProcAdd = NULL;
            ProcAdd = GetProcAddress(hinstLib, "QueryTests");

            // check to see if the function was located
            if (NULL != ProcAdd)
            {
                QueryTests MyFunctionQuery;
                MyFunctionQuery = QueryTests(ProcAdd);
                MyFunctionQuery(testIdArray, numberOfTests);

                // for every test that was returned
                for (ULONG i = 0; i < numberOfTests; i++)
                {
                    // check to see if the id is in the list provided
                    if (ContainsString(testIdArray->TestIdArray[i].Name, ids))
                    {
                        DWORD error;
                        ProcAdd = GetProcAddress(hinstLib, "RunTest");

                        // check to see if the funciton could be loaded
                        if (ProcAdd != NULL)
                        {
                            PrintTestStart(testIdArray->TestIdArray[i]);
                            ULONG testParametersSize = 0;
                            RunTest MyFunctionRunTest;
                            MyFunctionRunTest = RunTest(ProcAdd);

                            // for every entry read from the file
                            BOOLEAN ran = FALSE;
                            for (ULONG k = 0; k < parameters.NumberOfTests; k++)
                            {
                                // if the ids match agains the parameters provided in configuration file
                                if (String().Format(testIdArray->TestIdArray[i].Name).CompareNoCase(parameters.TestParamsArray[k].Id) == 0)
                                {
                                    // create list of pointers to the parameters
                                    LPWSTR params[KU_MAX_PARAMETERS];
                                    for (int j = 0; j < KU_MAX_PARAMETERS; j++)
                                    {
                                        params[j] = parameters.TestParamsArray[k].Parameter[j];
                                    }

                                    // run test
                                    PrintTestArguments(parameters.TestParamsArray[k]);
                                    error = MyFunctionRunTest(&(testIdArray->TestIdArray[i]), (PVOID)NULL, testParametersSize, parameters.TestParamsArray[k].Count, params);
                                    if ((error == ERROR_SUCCESS) && (parameters.TestParamsArray[k].PostOpCmd[0] != 0))
                                    {
                                        LONGLONG status = SpawnCommandLine(parameters.TestParamsArray[k].PostOpCmd);
                                        if (status != 0)
                                        {
                                            Log::Error(L"    Post operation spawned command failed");
                                            error = (DWORD)status;
                                        }
                                    }

                                    // update general result to signal failure
                                    result = PrintTestResult(error, testIdArray->TestIdArray[i]);
                                    // set ran as true
                                    ran = TRUE;
                                }
                            }

                            // check to see if the particular test ran as it was listed
                            if (!ran)
                            {
                                // if the test did not run log a comment saying configuration info not found
                                Log::Comment(L"    Test did not run. No entry in configuration file found.");
                            }
                        }
                        else
                        {
                            // log error and bail RunTest function is not in the dll
                            Log::Error(L"Could not locate function");
                            return (DWORD) -1;
                        }
                    }
                }
            }
            else
            {
                // log error and bail QueryTest function is not in the dll
                Log::Error(L"Function not present in library module");
                return (DWORD) -1;
            }

            // free the testidarray that was allocated
            free(testIdArray);
        }
        else
        {
            // log error and bail ListTest function is not in the dll
            Log::Error(L"Function not present in library module");
        }

        // free the library module
        fFreeResult = FreeLibrary(hinstLib);
        // return agregated result
        return result;
    }
    else
    {
        // the dll could not be loaded, bail
        Log::Error(L"Could not load library");
        return (DWORD) -1;
    }
}
#endif



DWORD
#ifndef PLATFORM_UNIX
TestLoader::
#endif
ExecuteAllUserTests(const String& name, const String& categories, TEST_PARAMS_ARRAY& parameters)
{
    // initialize test settings
    DWORD testPass = ERROR_SUCCESS;
    HINSTANCE hinstLib;
    FARPROC ProcAdd = NULL;
    BOOL fFreeResult, fRunTimeLinkSuccess = FALSE;
    ULONG numberOfTests = 0;

    #ifdef PLATFORM_UNIX
    hinstLib = dlopen(name.c_str(), RTLD_NOW | RTLD_DEEPBIND);
    #else
    hinstLib = LoadLibrary(String().Format(name) + L".dll");
    #endif

    // check to see if the dll could be loaded
    if (hinstLib != NULL)
    {
        #ifdef PLATFORM_UNIX
        ProcAdd = (FARPROC)dlsym(hinstLib, "QueryNumberOfTests");
        #else
        ProcAdd = GetProcAddress(hinstLib, "QueryNumberOfTests");
        #endif

        // check to see if the function could be loaded
        if (NULL != ProcAdd)
        {
            QueryNumberOfTests MyFunction;
            MyFunction = QueryNumberOfTests(ProcAdd);
            fRunTimeLinkSuccess = TRUE;
            MyFunction(numberOfTests);
            PKU_TEST_ID_ARRAY       testIdArray = NULL;
            ULONG                   testIdArraySize;
            testIdArraySize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + sizeof(KU_TEST_ID) * numberOfTests;
            testIdArray = (PKU_TEST_ID_ARRAY) malloc(testIdArraySize);

            // allocate memory for incoming tests from dll
            if (!testIdArray)
            {
                #ifdef PLATFORM_UNIX
                BOOST_TEST_MESSAGE("ERROR allocating memory.");
                #else
                Log::Warning(L"ERROR allocating memory.");
                #endif
            }

            ProcAdd = NULL;
            #ifdef PLATFORM_UNIX
            ProcAdd = (FARPROC)dlsym(hinstLib, "QueryTests");
            #else
            ProcAdd = GetProcAddress(hinstLib, "QueryTests");
            #endif

            // check to see if the function could be loaded
            if (NULL != ProcAdd)
            {
                QueryTests MyFunctionQuery;
                MyFunctionQuery = QueryTests(ProcAdd);
                MyFunctionQuery(testIdArray, numberOfTests);
                DWORD error;
                ProcAdd = NULL;
                #ifdef PLATFORM_UNIX
                ProcAdd = (FARPROC)dlsym(hinstLib, "RunTest");
                #else
                ProcAdd = GetProcAddress(hinstLib, "RunTest");
                #endif

                // check to see if the function could be loaded
                if (NULL != ProcAdd)
                {
                    // for every tests
                    for (ULONG i = 0; i < numberOfTests; i++)
                    {
                        BOOLEAN run = TRUE;
                        #ifdef PLATFORM_UNIX
                        string tmp = utf16to8(testIdArray->TestIdArray[i].Categories);
                        run = ContainsString(categories, tmp);
                        #else
                        run = ContainsString(categories, testIdArray->TestIdArray[i].Categories);
                        #endif

                        // check to see if it is included in the categories to be ran
                        if (run)
                        {
                            // log beginning of test
                            PrintTestStart(testIdArray->TestIdArray[i]);
                            ULONG testParametersSize = 0;
                            RunTest MyFunctionRunTest;
                            MyFunctionRunTest = RunTest(ProcAdd);
                            LPWSTR params[KU_MAX_PARAMETERS];
                            BOOLEAN ran = FALSE;
                            for (ULONG k = 0; k < parameters.NumberOfTests; k++)
                            {
                                // if the ids match
                                #ifdef PLATFORM_UNIX
                                if (wcscasecmp(testIdArray->TestIdArray[i].Name, parameters.TestParamsArray[k].Id) == 0)
                                #else
                                if (String().Format(testIdArray->TestIdArray[i].Name).CompareNoCase(parameters.TestParamsArray[k].Id) == 0)
                                #endif
                                {
                                    // create list of pointers to the parameters
                                    for (int j = 0; j < KU_MAX_PARAMETERS; j++)
                                    {
                                        params[j] = parameters.TestParamsArray[k].Parameter[j];
                                    }

                                    // run test
                                    PrintTestArguments(parameters.TestParamsArray[k]);
                                    error = MyFunctionRunTest(&(testIdArray->TestIdArray[i]), (PVOID)NULL, testParametersSize, parameters.TestParamsArray[k].Count, params);
                                    #ifndef PLATFORM_UNIX
                                    if ((error == ERROR_SUCCESS) && (parameters.TestParamsArray[k].PostOpCmd[0] != 0))
                                    {
                                        LONGLONG status = SpawnCommandLine(parameters.TestParamsArray[k].PostOpCmd);
                                        if (status != 0)
                                        {
                                            Log::Error(L"    Post operation spawned command failed");
                                            error = (DWORD)status;
                                        }
                                    }
                                    #endif

                                    // agregate result
                                    testPass = PrintTestResult(error, testIdArray->TestIdArray[i]);
                                    // set test as ran
                                    ran = TRUE;
                                }
                            }

                            // check to see if the test ran
                            if (!ran)
                            {
                                // log comment saying that no information was found for the test
                                #ifdef PLATFORM_UNIX
                                BOOST_TEST_MESSAGE("    Test did not run. No entry in configuration file found.");
                                #else
                                Log::Comment(L"    Test did not run. No entry in configuration file found.");
                                #endif
                            }
                        }
                    }
                }
                else
                {
                    // RunTest function is not in the dll
                    #ifdef PLATFORM_UNIX
                    BOOST_TEST_MESSAGE("Function not present in library module");
                    #else
                    Log::Error(L"Function not present in library module");
                    #endif
                    return (DWORD) -1;
                }
            }
            else
            {
                // QueryTest function is not in the dll
                #ifdef PLATFORM_UNIX
                BOOST_TEST_MESSAGE("Function not present in library module");
                #else
                Log::Error(L"Function not present in library module");
                #endif
                return (DWORD) -1;
            }

            // free allocated testidarray
            free(testIdArray);
        }
        else
        {
            // ListTest function is not in the dll
            #ifdef PLATFORM_UNIX
            BOOST_TEST_MESSAGE("Function not present in library module");
            #else
            Log::Error(L"Function not present in library module");
            #endif
            return (DWORD) -1;
        }

        // free loaded library
        #ifdef PLATFORM_UNIX
        dlclose(hinstLib);
        #else
        fFreeResult = FreeLibrary(hinstLib);
        #endif
        // return agregate result
        return testPass;
    }
    else
    {
        // the library could not be loaded
        #ifdef PLATFORM_UNIX
        BOOST_TEST_MESSAGE("Could not load library. Error: " << dlerror());
        #else
        Log::Error(L"Could not load library");
        #endif
        return (DWORD) -1;
    }
}

#ifndef PLATFORM_UNIX
//
// Entry points for the TAEF dll
//
void TestLoader::ListAvailableTests()
{
    // read parameters and set verify to only log on failure
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    String value;
    String driverName;
    RuntimeParameters::TryGetValue(L"Driver", driverName);
    String dllName;
    RuntimeParameters::TryGetValue(L"Dll", dllName);

    if (WEX::Common::String::IsNullOrEmpty(driverName))
    {
        Log::Warning(L"No driver name supplied. Will only list USER mode tests.");
    }
    if (WEX::Common::String::IsNullOrEmpty(dllName))
    {
        Log::Warning(L"No dll name supplied. Will only list KERNEL mode tests.");
    }

    // initialize and read target parameter
    TargetMode targetMode = GetTargetMode();

    // switch between modes
    Log::Comment(L"");
    if (targetMode & TargetMode::User)
    {
        if (!WEX::Common::String::IsNullOrEmpty(dllName))
        {
            Log::Comment(L"----USER----");
            Log::Comment(L"Listing available USER mode tests.");

            // execute USER mode
            TestLoader::ListAvailableUserTests(dllName);
            Log::Comment(L"");
        }
    }
    if (targetMode & TargetMode::Kernel)
    {
        if (!WEX::Common::String::IsNullOrEmpty(driverName))
        {
            Log::Comment(L"---KERNEL---");
            Log::Comment(L"Listing available KERNEL mode tests.");

            // execute KERNEL mode
            TestLoader::ListAvailableKernelTests(driverName);
        }
    }
    Log::Comment(L"");
}
#endif

#ifndef PLATFORM_UNIX
void TestLoader::ExecuteSingleTest(TEST_PARAMS_ARRAY& parameters)
{
    // initialize and set verify to log on failure only
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);
    String ids;
    RuntimeParameters::TryGetValue(L"ids", ids);

    if (!WEX::Common::String::IsNullOrEmpty(ids))
    {
        // initialize and process target parameter
        TargetMode targetMode = GetTargetMode();
        String driverName;
        RuntimeParameters::TryGetValue(L"Driver", driverName);
        String dllName;
        RuntimeParameters::TryGetValue(L"Dll", dllName);

        if (WEX::Common::String::IsNullOrEmpty(driverName))
        {
            Log::Warning(L"No driver name supplied. Will only list USER mode tests.");
        }
        if (WEX::Common::String::IsNullOrEmpty(dllName))
        {
            Log::Warning(L"No dll name supplied. Will only list KERNEL mode tests.");
        }

        Log::Comment(L"");
        DWORD userResult = ERROR_SUCCESS;
        DWORD kernelResult = ERROR_SUCCESS;

        // switch modes
        if (targetMode & TargetMode::User)
        {
            if (!WEX::Common::String::IsNullOrEmpty(dllName))
            {
                Log::Comment(L"----USER----");
                Log::Comment(L"Executing USER mode test.");

                userResult = TestLoader::ExecuteSingleUserTest(ids, dllName, parameters);
                Log::Comment(L"");
            }
        }
        if (targetMode & TargetMode::Kernel)
        {
            if (!WEX::Common::String::IsNullOrEmpty(driverName))
            {
                Log::Comment(L"---KERNEL---");
                Log::Comment(L"Executing KERNEL mode test.");

                String BreakOnStart;
                BOOLEAN breakOnStart = FALSE;
                RuntimeParameters::TryGetValue(L"BreakOnStart", BreakOnStart);
                if (!WEX::Common::String::IsNullOrEmpty(BreakOnStart))
                {
                    if (!BreakOnStart.CompareNoCase(L"TRUE"))
                    {
                        Log::Comment(L"TESTS WILL BREAK INTO THE DEBUGGER BEFORE STARTING");
                        breakOnStart = TRUE;
                    }
                }
                kernelResult = TestLoader::ExecuteSingleKernelTest(ids, breakOnStart, driverName, parameters);
            }
        }

        // log appropriate error or success
        ProcessErrors(userResult, kernelResult);
    }
    else
    {
        // log error and bail as no ids where specified
        Log::Error(L"Please specify one or more ids.");
    }
}
#endif

#ifndef PLATFORM_UNIX
void TestLoader::ExecuteTests(TEST_PARAMS_ARRAY& parameters)
{
    // initialize and set verify to log on failure only
    SetVerifyOutput verifySettings(VerifyOutputSettings::LogOnlyFailures);

    // initialize and process target parameter
    TargetMode targetMode = GetTargetMode();
    String driverName;
    RuntimeParameters::TryGetValue(L"Driver", driverName);
    String dllName;
    RuntimeParameters::TryGetValue(L"Dll", dllName);
    String categories;
    RuntimeParameters::TryGetValue(L"Categories", categories);

    if (WEX::Common::String::IsNullOrEmpty(driverName))
    {
        Log::Warning(L"No driver name supplied. Will only list USER mode tests.");
    }
    if (WEX::Common::String::IsNullOrEmpty(dllName))
    {
        Log::Warning(L"No dll name supplied. Will only list KERNEL mode tests.");
    }

    Log::Comment(L"");
    DWORD userResult = ERROR_SUCCESS;
    DWORD kernelResult = ERROR_SUCCESS;

    // switch between modes
    if (targetMode & TargetMode::User)
    {
        if (!WEX::Common::String::IsNullOrEmpty(dllName))
        {
            Log::Comment(L"Executing available USER mode tests.");
            Log::Comment(L"----USER----");
            Log::Comment(L"Executing USER mode tests...");

            // execute USER mode
            userResult = TestLoader::ExecuteAllUserTests(dllName, categories, parameters);
            Log::Comment(L"");
        }
    }
    if (targetMode & TargetMode::Kernel)
    {
        if (!WEX::Common::String::IsNullOrEmpty(driverName))
        {
            Log::Comment(L"Executing available KERNEL mode tests.");
            Log::Comment(L"---KERNEL---");
            Log::Comment(L"Executing KERNEL mode tests...");

            // execute KERNEL mode
            String BreakOnStart;
            BOOLEAN breakOnStart = FALSE;
            RuntimeParameters::TryGetValue(L"BreakOnStart", BreakOnStart);
            if (!WEX::Common::String::IsNullOrEmpty(BreakOnStart))
            {
                if (!BreakOnStart.CompareNoCase(L"TRUE"))
                {
                    Log::Comment(L"TESTS WILL BREAK INTO THE DEBUGGER BEFORE STARTING");
                    breakOnStart = TRUE;
                }
            }
            kernelResult = TestLoader::ExecuteAllKernelTests(driverName, categories, breakOnStart, parameters);
        }
    }
    // process USER and KERNEL mode results
    ProcessErrors(userResult, kernelResult);
}
#endif

#ifndef PLATFORM_UNIX
void TestLoader::Usage()
{
    // print the usage
    Log::Comment(L"");
    Log::Comment(L"-----Usage-----");
    Log::Comment(L"te.exe testloader.dll /name:\"TestLoader::Usage\"");
    Log::Comment(L"");
    Log::Comment(L"---------------");
    Log::Comment(L"Modifiers");
    Log::Comment(L"");
    Log::Comment(L"    /p:target=[user|kernel|both]");
    Log::Comment(L"    /p:driver=[drivername] -> the name of the driver to test");
    Log::Comment(L"    /p:dll=[dllname] -> the name of the dll to test");
    Log::Comment(L"");
    Log::Comment(L"---------------");
    Log::Comment(L"To list available tests.");
    Log::Comment(L"");
    Log::Comment(L"    te.exe testloader.dll /name:\"TestLoader::ListAvailableTests\"");
    Log::Comment(L"");
    Log::Comment(L"---------------");
    Log::Comment(L"To execute a single test.");
    Log::Comment(L"");
    Log::Comment(L"    te.exe testloader.dll /name:\"TestLoader::ExecuteSingleTest\"");
    Log::Comment(L"    /p:id=[int] -> the id of the test");
    Log::Comment(L"");
    Log::Comment(L"---------------");
    Log::Comment(L"To execute all tests.");
    Log::Comment(L"");
    Log::Comment(L"    te.exe testloader.dll /name:\"TestLoader::ExecuteTests\"");
    Log::Comment(L"");
    Log::Comment(L"---------------");
    Log::Comment(L"");
    Log::Comment(L"To generate codecoverage report.");
    Log::Comment(L"");
    Log::Comment(L"    After any of the above commands add:");
    Log::Comment(L"        /testmode:localcodecoverage");
    Log::Comment(L"        /binaryundertest:[list of filenames under test with extensions separated by ;]");
    Log::Comment(L"    e.g.: te.exe testloader.dll /name:\"TestLoader::ExecuteTests\" /p:driver=test");
    Log::Comment(L"              /testmode:localcodecoverage /binaryundertest:test.sys");
    Log::Comment(L"");
}
#endif

#ifndef PLATFORM_UNIX
void TestLoader::Execute()
{
    // read configuration file
    PTEST_PARAMS_ARRAY testParamsArray = NULL;
    TestDataArray<String> params;
    if (FAILED(TestData::TryGetValue(L"Parameters", params)))
    {
        // log an error if file is malformed, bail afterwards
        Log::Error(L"Could not retrieve parameter nodes, malformed configuration file.");
        return;
    }

    size_t count = params.GetSize();
    size_t testParamsArraySize;
    testParamsArraySize = FIELD_OFFSET(TEST_PARAMS_ARRAY, TestParamsArray) + sizeof(TEST_PARAMS) * count;
    testParamsArray = (PTEST_PARAMS_ARRAY) malloc(testParamsArraySize);

    // try to allocate memory for all the parameters found
    if (!testParamsArray)
    {
            Log::Warning(L"ERROR allocating memory for test parameters.");
    }
    testParamsArray->NumberOfTests = count;

    // for each set of id:parameter
    for (size_t i = 0; i < count; ++i)
    {
        HRESULT hr = ERROR_SUCCESS;

        // find and extract id
        int colonIndex = params[i].Find(L':');

        // extract id
        String testId;
        testId.Copy(params[i], colonIndex);
        hr = StringCchCopyW(testParamsArray->TestParamsArray[i].Id, KU_TEST_NAME_LENGTH, testId );
        if (FAILED(hr))
        {
            Log::Error(L"Could not copy id.");
            break;
        }

        params[i].Delete(0,colonIndex+1);

        // set as not run yet
        testParamsArray->TestParamsArray[i].HasRun = FALSE;
        testParamsArray->TestParamsArray[i].PostOpCmd[0] = 0;

        int index = 0;
        if (!WEX::Common::String::IsNullOrEmpty(params[i]))
        {
            BOOLEAN searching = TRUE;
            BOOLEAN storeParameter = TRUE;
            String tempParameter = params[i];

            // search until out of string
            while (searching)
            {
                storeParameter = TRUE;
                String parameter;
                String postOpCmd;
                int comma = tempParameter.Find(L',', 0);

                // found a comma
                if (comma >= 0)
                {
                    parameter.Copy(tempParameter, comma);
                    tempParameter.Delete(0, parameter.GetLength() + 1);
                }
                else
                {
                    searching = FALSE;

                    // See if the terminator is a '{' - meaning a post test step
                    int brace = tempParameter.Find(L'{');

                    if (brace < 0)
                    {
                        parameter = tempParameter;
                    }
                    else
                    {
                        // post operation command line is provided
                        parameter.Copy(tempParameter, brace);
                        tempParameter.Delete(0, parameter.GetLength() + 1);

                        int endBrace = tempParameter.Find(L'}');
                        if (endBrace == 0)
                        {
                            Log::Error(L"Invalid post operation declarition syntax.");
                            break;
                        }

                        postOpCmd.Copy(tempParameter, endBrace);
                        storeParameter = (brace != 0);
                    }
                }

                hr = StringCchCopyW(testParamsArray->TestParamsArray[i].PostOpCmd, KU_MAX_POST_OP_LENGTH, postOpCmd);
                if (FAILED(hr))
                {
                    Log::Error(L"Could not copy postOpCmd.");
                    break;
                }

                if (storeParameter)
                {
                    hr = StringCchCopyW(testParamsArray->TestParamsArray[i].Parameter[index], KU_MAX_PATH, parameter);
                    if (FAILED(hr))
                    {
                        Log::Error(L"Could not copy parameter.");
                        break;
                    }
                    index++;
                }
            }
        }

        // set the parameter count as the index
        testParamsArray->TestParamsArray[i].Count = index;
    }

    // switch functionality
    String value;
    RuntimeParameters::TryGetValue(L"C", value);
    if (value.CompareNoCase(L"List") == 0)
    {
        ListAvailableTests();
    }
    else if (value.CompareNoCase(L"RunTest") == 0)
    {
        ExecuteSingleTest(*testParamsArray);
    }
    else if (value.CompareNoCase(L"RunTests") == 0)
    {
        ExecuteTests(*testParamsArray);
    }
    else
    {
        Usage();
    }

    // free the test parameter array that was allocated
    free(testParamsArray);
}
#endif

#ifdef PLATFORM_UNIX
const string TEST_CMD = "C";
const string TEST_FILE = "so";
const string TEST_CONFIG_FILE = "configfile";
const string TEST_CATEGORIES = "categories";

BOOST_AUTO_TEST_CASE(Execute)
{
    using namespace boost::unit_test::framework;
    using boost::property_tree::ptree;

    int argc    = master_test_suite().argc;
    char **argv = master_test_suite().argv;

    map<string,string> params;
    for (int i = 1; i < argc; ++i) {
        string param(argv[i]);
        int pos = param.find("=");
        params.insert(make_pair(param.substr(0, pos), param.substr(pos+1)));
    }
    BOOST_REQUIRE(params.count(TEST_CMD) != 0);
    BOOST_REQUIRE(params.count(TEST_FILE) != 0);
    BOOST_REQUIRE(params.count(TEST_CONFIG_FILE) != 0);

    fstream fs;
    fs.open(params[TEST_CONFIG_FILE].c_str());
    BOOST_REQUIRE(fs.is_open());

    ptree pt;
    read_xml(fs, pt);

    fs.close();

    const ptree& parameters = pt.get_child("Data.Table.Row.Parameter");

    TEST_PARAMS_ARRAY* testParamsArray = nullptr;
    size_t count = parameters.size();
    size_t testParamsArraySize = FIELD_OFFSET(TEST_PARAMS_ARRAY, TestParamsArray) + sizeof(TEST_PARAMS) * count;
    testParamsArray = (PTEST_PARAMS_ARRAY) malloc(testParamsArraySize);
    testParamsArray->NumberOfTests = 0;
    testParamsArray->Reserved1 = 0;

    BOOST_FOREACH(const ptree::value_type& param, parameters) {
        if (param.first == "Value") {
            HRESULT hr = ERROR_SUCCESS;

            wstring s = Utf8To16(param.second.data().c_str());
            size_t pos = s.find(L":");

            TEST_PARAMS& testParams = testParamsArray->TestParamsArray[testParamsArray->NumberOfTests];
            hr = StringCchCopyW(testParams.Id, KU_TEST_NAME_LENGTH, s.substr(0, pos).c_str());

            s = s.substr(pos+1);
            testParams.Count = 0;
            testParams.HasRun = false;

            int index = 0;
            if (!s.empty()) {
                BOOLEAN searching = TRUE;
                BOOLEAN storeParameter = TRUE;
                wstring tempParameter = s;

                // search until out of string
                while (searching)
                {
                    storeParameter = TRUE;
                    wstring parameter;
                    wstring postOpCmd;
                    int comma = tempParameter.find(L',', 0);

                    // found a comma
                    if (comma >= 0)
                    {
                        parameter = tempParameter.substr(0, comma);
                        tempParameter = tempParameter.substr(comma + 1);
                    }
                    else
                    {
                        searching = FALSE;

                        // See if the terminator is a '{' - meaning a post test step
                        int brace = tempParameter.find(L'{');

                        if (brace < 0)
                        {
                            parameter = tempParameter;
                        }
                        else
                        {
                            // post operation command line is provided
                            parameter = tempParameter.substr(0, brace);
                            tempParameter = tempParameter.substr(brace+1);

                            int endBrace = tempParameter.find(L'}');
                            if (endBrace == 0)
                            {
                                //Log::Error(L"Invalid post operation declarition syntax.");
                                break;
                            }

                            postOpCmd = tempParameter.substr(0, endBrace);
                            storeParameter = (brace != 0);
                        }
                    }

                    hr = StringCchCopyW(testParams.PostOpCmd, KU_MAX_POST_OP_LENGTH, postOpCmd.c_str());
                    if (FAILED(hr))
                    {
                        //Log::Error(L"Could not copy postOpCmd.");
                        break;
                    }

                    if (storeParameter)
                    {
                        hr = StringCchCopyW(testParams.Parameter[index], KU_MAX_PATH, parameter.c_str());
                        if (FAILED(hr))
                        {
                            //Log::Error(L"Could not copy parameter.");
                            break;
                        }
                        index++;
                    }
                }
            }

            testParams.Count = index;

            testParamsArray->NumberOfTests++;
        }
    }

    // switch functionality
    auto iter = params.find("C");
    if (iter != params.end() && iter->second == "List")
    {
        //ListAvailableTests();
    }
    else if (iter != params.end() && iter->second == "RunTest")
    {
        //ExecuteSingleTest(*testParamsArray);
    }
    else if (iter != params.end() && iter->second == "RunTests")
    {
        //ExecuteTests(*testParamsArray);
    }
    else
    {
        //Usage();
    }

    DWORD userResult = ERROR_SUCCESS;
    userResult = ExecuteAllUserTests(params[TEST_FILE], params[TEST_CATEGORIES], *testParamsArray);
    BOOST_CHECK(userResult == 0);

    free(testParamsArray);
}
#endif

