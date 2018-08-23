/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KTrace.cpp

    Description:
      Kernel Tempate Library (KTL) system instance base class

      Implement trace support functions

    History:
      alanwar          28-July-2017         Initial version.

--*/

#include <ktl.h>
#include <ktrace.h>


#if KTL_USER_MODE

KtlTraceCallback g_KtlTraceCallback;

VOID RegisterKtlTraceCallback(
    __in KtlTraceCallback Callback
    )
{
    g_KtlTraceCallback = Callback;
}

#endif
