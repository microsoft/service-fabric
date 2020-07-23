// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

#if !DotNetCoreClrIOT

using namespace System;

void CopyFromManaged(wchar_t * dest, int destSize, String^ managedSrc)
{
    pin_ptr<const wchar_t> src = PtrToStringChars(managedSrc);

    wcsncpy_s(dest, destSize, src, _TRUNCATE);
}

#endif