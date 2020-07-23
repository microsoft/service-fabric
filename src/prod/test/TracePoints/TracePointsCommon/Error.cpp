// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

runtime_error Error::LastError(string const & what)
{
    return Error::WindowsError(GetLastError(), what);
}

runtime_error Error::WindowsError(DWORD errorCode, string const & what)
{
    ostringstream oss;
    oss << what << " (Error " << errorCode << ".)" << endl;
    return runtime_error(oss.str());
}
