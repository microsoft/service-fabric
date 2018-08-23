#pragma once

#include "KmCommon.h"
#include <KmUnitDefs.h>

#ifndef WIN32_FROM_HRESULT
#define WIN32_FROM_HRESULT(hr)           \
    (SUCCEEDED(hr) ? ERROR_SUCCESS :    \
        (HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr)))
#endif

#ifdef PLATFORM_UNIX
extern "C" {
DWORD QueryNumberOfTests(ULONG& numberOfTests);

DWORD QueryTests(PKU_TEST_ID_ARRAY testIdArray, ULONG& testIdArraySize);

NTSTATUS RunTest(PKU_TEST_ID testId, PVOID testParameters, ULONG& testParametersSize, int argc, __in_ecount(argc) WCHAR *args[]);
}
#else
namespace KmUser
{
    class __declspec(dllexport) KmUserFuncs
    {
    public:
      static DWORD QueryNumberOfTests(ULONG& numberOfTests);

      static DWORD QueryTests(PKU_TEST_ID_ARRAY testIdArray, ULONG& testIdArraySize);

      static NTSTATUS RunTest(PKU_TEST_ID testId, PVOID testParameters, ULONG& testParametersSize, int argc, __in_ecount(argc) WCHAR *args[]);
    };
}
#endif

