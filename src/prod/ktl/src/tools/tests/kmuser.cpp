#include "KmUser.h"
#ifndef KM_LIBRARY
#include "KmUnitTestCases.h"
#endif
#include <ktl.h>
#include <ktrace.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLATFORM_UNIX
extern "C" {
#endif

#ifndef PLATFORM_UNIX
namespace KmUser
{
#endif
    DWORD
#ifndef PLATFORM_UNIX
    KmUserFuncs::
#endif
    QueryNumberOfTests(ULONG& numberOfTests)
    {
        numberOfTests = ARRAY_SIZE(gs_KuTestCases);
        return ERROR_SUCCESS;
    }

    DWORD
#ifndef PLATFORM_UNIX
    KmUserFuncs::
#endif
    QueryTests(PKU_TEST_ID_ARRAY testIdArray, ULONG& testIdArraySize)
    {
        HRESULT hr = S_OK;
        PKU_TEST_ID         testId;

        ZeroMemory((PVOID)testIdArray, testIdArraySize);

        testIdArray->NumberOfTests = ARRAY_SIZE(gs_KuTestCases);

        for (unsigned int i = 0; i < ARRAY_SIZE(gs_KuTestCases); ++i) {
            testId = &testIdArray->TestIdArray[i];
            testId->Id = i;
            hr = StringCchCopyW(testId->Name, KU_TEST_NAME_LENGTH, gs_KuTestCases[i].TestCaseName);
            if(FAILED(hr))
            {
                break;
            }
            hr = StringCchCopyW(testId->Categories, KU_MAX_PATH, gs_KuTestCases[i].Categories);
            if(FAILED(hr))
            {
                break;
            }
            hr = StringCchCopyW(testId->Help, KU_MAX_PATH, gs_KuTestCases[i].Help);
            if(FAILED(hr))
            {
                break;
            }
        }

        if(FAILED(hr))
        {
            return WIN32_FROM_HRESULT(hr);
        }
        return ERROR_SUCCESS;
    }

    NTSTATUS
#ifndef PLATFORM_UNIX
    KmUserFuncs::
#endif
    RunTest(PKU_TEST_ID testId, PVOID testParameters, ULONG& testParametersSize, int argc, WCHAR *args[])
    {
        UNREFERENCED_PARAMETER(testParameters);
        UNREFERENCED_PARAMETER(testParametersSize);

        PCKU_TEST_ENTRY     testEntry;
        testEntry = &gs_KuTestCases[testId->Id];
        return testEntry->TestRoutine(argc, args);
    }
#ifndef PLATFORM_UNIX
}
#endif

#ifdef PLATFORM_UNIX
}
#endif

void KTestPrintf(const char *Fmt, ...)
{
    va_list args;
    va_start(args, Fmt);

    vprintf(Fmt, args);

#if !defined(PLATFORM_UNIX)
    KDbgPrintfVarArgs(Fmt, args);
#endif

    va_end(args);
}

