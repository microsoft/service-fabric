#pragma warning(disable: 4838)

#ifndef KTL_MIXED_STL_ATL
#define KTL_MIXED_STL_ATL
#endif
#include <ktl.h>

#include <atlbase.h>

#include <stdio.h>
#include <iostream>
#include <varargs.h>

#include <objbase.h>
#include <msxml6.h>

// Platform layer

#include <string>
#include <vector>
#include <memory>

using namespace std;


int
wmain(int argc, wchar_t** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);
    
    NTSTATUS Result;
    Result = KtlSystem::Initialize();
    if (!NT_SUCCESS(Result))
    {
        printf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    KAllocator& g_Allocator = KtlSystem::GlobalNonPagedAllocator();

    wstring s = L"STL String";

    KArray<wstring> StringArray1(g_Allocator);
    wstring x = L"string";

    StringArray1.Append(x);

    vector<wstring> StringArray2;

    CComVariant v;
    vector<CComBSTR> ArrayOfBSTR;  // ATL/STL mix

    KArray<CComVariant> varArray(g_Allocator);
    varArray.Append(v);

    CComVariant cv = varArray[0];

    wcout << L"This is a test of KT/STL/ATL simultaneously" << endl;

    return 0;
}
