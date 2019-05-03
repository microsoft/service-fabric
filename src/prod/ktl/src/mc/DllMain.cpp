/*++

    (c) 2014 by Microsoft Corp. All Rights Reserved.

    DllMain.cpp

    Description:
        A dummy file just to create an import library so that
        MSBuild doesn't complain when we use the mc project as
        a project reference in other vcxproj files.
        This is just a short term fix and will hopefully go away
        once this MSBuild limitation is gone.

--*/

#include <windows.h>
#include <process.h>

BOOL APIENTRY DllMain(
    HMODULE,
    DWORD,
    LPVOID)
{
    __security_init_cookie();
    return TRUE;
}

extern "C" __declspec(dllexport) void Noop()
{
}
