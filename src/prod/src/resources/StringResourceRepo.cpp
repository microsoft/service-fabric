// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <windows.h>
#include <system_error>

#if defined(PLATFORM_UNIX)
#include <map>
#include <string>
#include <sal.h>
#endif

#include "resources/Resources.h"


#if defined(PLATFORM_UNIX)
#include "StringResourceRepo.h"
using namespace std;
StringResourceRepo* StringResourceRepo::Get_Singleton()
{
    static StringResourceRepo *repo = new StringResourceRepo();
    return repo;
}

void StringResourceRepo::AddResource(int id, wstring const& str)
{
    resourceMap_.insert(make_pair(id, str));
}

wstring StringResourceRepo::GetResource(int id)
{
    return resourceMap_[id];
}

StringResourceRepo::StringResourceRepo()
{
#ifndef FABRIC_RESOURCES_STATIC_LIB
    #define DEFINE_STRING_RESOURCE(id,str) AddResource(id, str);
    #include "stringtbl.inc"
#endif
}


int LoadStringResource(UINT id, __out_ecount(bufferMax) LPWSTR buffer, int bufferMax)
{
    wstring str = StringResourceRepo::Get_Singleton()->GetResource(id);
    int size = str.size();
    if(size > bufferMax -1)
       size = bufferMax - 1;
    memcpy(buffer, str.c_str(), size*sizeof(wchar_t));
    buffer[size] = 0;
    return size;
}

#else

static HMODULE g_currentModule = nullptr;

void EnsureCurrentModule()
{
    if (g_currentModule != nullptr)
        return;

    // See http://msdn.microsoft.com/en-us/library/ms683200(v=VS.85).aspx
    // The second parameter of GetModuleHandleEx can be a pointer to any
    // address mapped to the module.  We define a static to use as that
    // pointer.
    static int staticInThisModule = 0;

    BOOL success = ::GetModuleHandleEx(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCTSTR>(&staticInThisModule),
    &g_currentModule);

    if (!success)
    {
        DWORD dwErrVal = GetLastError();
        std::error_code ec (dwErrVal, std::system_category());
        throw std::system_error(ec, "GetModuleHandleEx");
    }
}

int LoadStringResource(UINT id, __out_ecount(bufferMax) LPWSTR buffer, int bufferMax)
{
   EnsureCurrentModule();
   return LoadString(g_currentModule, id, buffer, bufferMax);
}

#endif
