// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::Log;

bool Data::Log::IsValidKtlLoggerMode(__in KtlLoggerMode const & ktlLoggerMode)
{
    return ktlLoggerMode == KtlLoggerMode::Default || ktlLoggerMode == KtlLoggerMode::InProc || ktlLoggerMode == KtlLoggerMode::OutOfProc;
}
