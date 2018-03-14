// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

extern "C" void CancellationTokenSource_Cancel(
    CancellationTokenSourceHandle cts)
{
    ((ktl::CancellationTokenSource*)cts)->Cancel();
}

extern "C" void CancellationTokenSource_Release(
    CancellationTokenSourceHandle cts)
{
    ktl::CancellationTokenSource::SPtr ctsSPtr;
    ctsSPtr.Attach((ktl::CancellationTokenSource*)cts);
}
