
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KStreams.h

    Description:
      KTL interface definitions stream abstractions.

    History:
      richhas          12-Jan-2012         Initial version.

--*/

#pragma once

#include <ktl.h>


//* Interface abstraction for outputing wellknown types into an information stream
class KIOutputStream
{
public:
    virtual
    NTSTATUS Put(__in WCHAR Value) = 0;

    virtual
    NTSTATUS Put(__in LONG Value) = 0;

    virtual
    NTSTATUS Put(__in ULONG Value) = 0;

    virtual
    NTSTATUS Put(__in LONGLONG Value) = 0;

    virtual
    NTSTATUS Put(__in ULONGLONG Value) = 0;

    virtual
    NTSTATUS Put(__in GUID& Value) = 0;

    virtual
    NTSTATUS Put(__in_z WCHAR const * Value) = 0;

    virtual
    NTSTATUS Put(__in KUri::SPtr Value) = 0;

    virtual
    NTSTATUS Put(__in KString::SPtr Value) = 0;

    virtual
    NTSTATUS Put(__in KDateTime Value) = 0;

    virtual
    NTSTATUS Put(__in KDuration Value) = 0;

    virtual
    NTSTATUS Put(__in BOOLEAN Value) = 0;
};


