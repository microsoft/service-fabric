#pragma once
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif

NTSTATUS
KStreamTest(
    __in int argc, 
    __in_ecount(argc) WCHAR* args[]
    );
