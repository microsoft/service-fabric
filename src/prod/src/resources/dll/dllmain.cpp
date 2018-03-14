// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <windows.h>

#if defined(PLATFORM_UNIX)
#include <map>
#include <string>
#include <sal.h>
#include "PAL.h"
#endif

#include "targetver.h"
#include "resources/Resources.h"

#if defined(PLATFORM_UNIX)
using namespace std;
class StringResourceRepo
{
private:
    map<int, wstring> resourceMap_;

public:
    static StringResourceRepo *Get_Singleton()
    {
        static StringResourceRepo *repo = new StringResourceRepo();
        return repo;
    }

    void AddResource(int id, wstring const& str)
    {
        resourceMap_.insert(make_pair(id, str));
    }

    wstring GetResource(int id)
    {
        return resourceMap_[id];
    }

    StringResourceRepo()
    {
        #define DEFINE_STRING_RESOURCE(id,str) AddResource(id, str);
        #include "stringtbl.inc"
    }
};


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

static HMODULE ThisModule = 0;

BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        ThisModule = module;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

int LoadStringResource(UINT id, __out_ecount(bufferMax) LPWSTR buffer, int bufferMax)
{
    return LoadString(ThisModule, id, buffer, bufferMax);
}

#endif
